#include "TemperatureAnalysisParallel.h"

using namespace std;

TemperatureAnalysisParallel::TemperatureAnalysisParallel(const string &filename) : inputFile(filename) {}

void TemperatureAnalysisParallel::setHeatingMonths(const vector<int> &months) {
    heatingMonths = months;
}

void TemperatureAnalysisParallel::setCoolingMonths(const vector<int> &months) {
    coolingMonths = months;
}

// Starts the task-parallel pipeline
void TemperatureAnalysisParallel::startPipeline(const string &outputFile) {
    thread readerThread(&TemperatureAnalysisParallel::fileReader, this);
    thread parserThread(&TemperatureAnalysisParallel::parser, this);
    thread processorThread(&TemperatureAnalysisParallel::anomalyDetector, this);
    thread writerThread(&TemperatureAnalysisParallel::fileWriter, this, outputFile);

    readerThread.join();
    parserThread.join();
    processorThread.join();
    writerThread.join();
}

// Stage 1: Reads data from the file and pushes to readQueue
void TemperatureAnalysisParallel::fileReader() {
    string line;
    while (getline(inputFile, line)) {
        unique_lock<mutex> lock(readMutex);
        readQueue.push(line);
        readCond.notify_one();
    }
    inputFile.close();
}

// Stage 2: Parses each line into TemperatureData and pushes to parseQueue
void TemperatureAnalysisParallel::parser() {
    while (true) {
        unique_lock<mutex> lock(readMutex);
        readCond.wait(lock, [this] { return !readQueue.empty(); });

        auto line = readQueue.front();
        readQueue.pop();

        lock.unlock();

        TemperatureData data = parseLine(line);
        if(data.month == INT_MAX) {
            continue;
        }

        unique_lock<mutex> parseLock(parseMutex);
        parseQueue.push(data);
        parseCond.notify_one();
    }

    // Push sentinel value to indicate completion
    {
        unique_lock<mutex> lock(parseMutex);
        parseQueue.push(TemperatureData()); // Push an invalid TemperatureData as sentinel
        parseCond.notify_one();
    }
}


// Stage 3: Processes each TemperatureData for anomalies and pushes to processQueue
void TemperatureAnalysisParallel::anomalyDetector() {
    vector<thread> threads; // Container to hold thread objects
    unordered_map<Hour, vector<double>, HourHash> hourlyTemperatures; // store temps by hour
    unordered_map<TimeKey, double, TimeKeyHash> lastTemperature; // To track the last temperature per TimeKey
    int currentMonth = -1; // Initialize to an invalid month

    while (true) {
        unique_lock<mutex> lock(parseMutex);
        parseCond.wait(lock, [this] { return !parseQueue.empty(); });

        auto data = parseQueue.front();
        parseQueue.pop();
        lock.unlock();

        // Create composite key for the current data
        TimeKey key(data.year, data.month, data.day, data.hour);

        // Detect anomaly based on the last temperature
        if (lastTemperature.find(key) != lastTemperature.end()) {
            double previousTemp = lastTemperature[key];
            if (isAnomaly(data.temperature, previousTemp)) {
                // Skip this entry as it's an anomaly
                continue;
            }
        }

        // Store the current temperature
        lastTemperature[key] = data.temperature; // Track the last temperature for this TimeKey
        Hour hourKey(data.day, data.hour); // Create Hour struct or object
        hourlyTemperatures[hourKey].push_back(data.temperature); // Store the temperature by hour

        // Check if the month has changed
        if (currentMonth != data.month) {
            // Raise a flag for the completion of the previous month
            if (currentMonth != -1) { // Make sure this isn't the first month
                // Launch a new thread for evaluating the monthly temperatures
                threads.push_back(thread(&TemperatureAnalysisParallel::evaluateMonthlyTemperatures,
                    this, key, hourlyTemperatures));
            }

            // Reset for the new month
            hourlyTemperatures.clear(); // Clear previous month data
            lastTemperature.clear(); // Clear last temperature data

            // Update currentMonth to the new month
            currentMonth = data.month;
        }
    }

    // Join all threads before finishing
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}



// Function to calculate mean and standard deviation and evaluate temperatures
void TemperatureAnalysisParallel::evaluateMonthlyTemperatures(
    const TimeKey& key, const unordered_map<Hour, vector<double>, HourHash> temperatures) {
    
    // Check if there are any temperatures to evaluate
    if (temperatures.empty()) {
        return; // No data to process
    }

    // Calculate mean and standard deviation
    double mean = calculateMean(temperatures);
    double stddev = calculateStdDev(temperatures, mean);
    
    printf("Month: %d Mean: %f, stdev: %f, count: %d\n", key.month, mean, stddev, temperatures.size());

    // Process temperatures for heating/cooling issues
    for (const auto& hourEntry : temperatures) {
        for(double temp : hourEntry.second) {
            if ((isHeatingMonth(key.month) && temp > mean + stddev) || (isCoolingMonth(key.month) && temp < mean - stddev)) {
                // Create a TemperatureData instance to populate with details
                TemperatureDataOut data(key.month, key.day, key.year, key.hour, 0, 0, temp, mean, stddev);
                unique_lock<mutex> processLock(processMutex);
                printf("Mean: %f, stdev: %f, temp: %f, size: %d\n", mean, stddev, temp, temperatures.size());
                processQueue.push(data); // Push the detected issue to the queue
                processCond.notify_one();
                break; // Stop processing after the first issue in this hour
            }
        }
    }
}

// Stage 4: Writes results to the output file
void TemperatureAnalysisParallel::fileWriter(const string &outputFile) {
    ofstream outFile(outputFile);
    while (true) {
        unique_lock<mutex> lock(processMutex);
        processCond.wait(lock, [this] { return !processQueue.empty(); });

        auto result = processQueue.front();
        processQueue.pop();

        lock.unlock();

        // Format and write the result to the output file
        if (isHeatingMonth(result.month)) {
            outFile << "Heating issue detected: " 
                    << result.month << "/" << result.day << "/" << result.year
                    << " At Hour: " << result.hour << " | Temp: " << result.temperature 
                    << " | Mean: " << result.mean << " | Stddev: " << result.stddev << endl;
        } else if (isCoolingMonth(result.month)) {
            outFile << "Cooling issue detected: " 
                    << result.month << "/" << result.day << "/" << result.year
                    << " At Hour: " << result.hour << " | Temp: " << result.temperature 
                    << " | Mean: " << result.mean << " | Stddev: " << result.stddev << endl;
        }
    }
    outFile.close();
}




// Helper function to parse a line of data
TemperatureData TemperatureAnalysisParallel::parseLine(const string &line)
{
    stringstream ss(line);
    char delim{};

    int month, day, year, hour, minute, second;
    float temperature;

    if (line.empty())
    {
        // Return an empty object for an empty line
        return TemperatureData(INT_MAX, 0, 0, 0, 0, 0, 0.0);
    }

    // store date into temporary fields
    ss >> month >> delim >> day >> delim >> year;
    // store time into temporary fields
    ss >> hour >> delim >> minute >> delim >> second;
    // store temperature
    ss >> temperature >> delim;

    return TemperatureData(month, day, year, hour, minute, second, temperature);
}

// Helper function to detect anomalies based on temperature
bool TemperatureAnalysisParallel::isAnomaly(double currentTemp, double previousTemp) {
    return abs(currentTemp - previousTemp) > 2.0;
}

// Helper function to calculate mean
double TemperatureAnalysisParallel::calculateMean(const unordered_map<Hour, vector<double>, HourHash>& temperatures) {
    double totalSum = 0.0;
    int totalCount = 0;

    for (const auto& entry : temperatures) {
        const vector<double>& tempData = entry.second;
        totalCount += tempData.size();
        for (double temp : tempData) {
            totalSum += temp;
        }
    }

    return totalCount > 0 ? totalSum / totalCount : 0.0; // Return 0 if no temperatures
}


// Helper function to calculate standard deviation
double TemperatureAnalysisParallel::calculateStdDev(const unordered_map<Hour, vector<double>, HourHash>& temperatures, double mean) {
    double sumSquaredDiffs = 0.0;
    int totalCount = 0;

    for (const auto& entry : temperatures) {
        const vector<double>& tempData = entry.second;
        totalCount += tempData.size();
        for (double temp : tempData) {
            sumSquaredDiffs += (temp - mean) * (temp - mean);
        }
    }

    return totalCount > 1 ? sqrt(sumSquaredDiffs / (totalCount - 1)) : 0.0; // Return 0 if no valid stddev can be calculated
}


// Check if a month is a heating month
bool TemperatureAnalysisParallel::isHeatingMonth(int month) {
    return find(heatingMonths.begin(), heatingMonths.end(), month) != heatingMonths.end();
}

// Check if a month is a cooling month
bool TemperatureAnalysisParallel::isCoolingMonth(int month) {
    return find(coolingMonths.begin(), coolingMonths.end(), month) != coolingMonths.end();
}
