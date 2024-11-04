#include "TemperatureAnalysisParallel.h"

using namespace std;

TemperatureAnalysisParallel::TemperatureAnalysisParallel(const string &filename) : inputFile(filename) {}

// Set the months designated for heating
void TemperatureAnalysisParallel::setHeatingMonths(const vector<int> &months)
{
    heatingMonths = months;
}

// Set the months designated for cooling
void TemperatureAnalysisParallel::setCoolingMonths(const vector<int> &months)
{
    coolingMonths = months;
}

// Partitioning & Scheduling: Each pipeline stage (file reading, parsing, anomaly detection, and writing) is
// divided into separate tasks, running concurrently. Scheduling is done by launching dedicated threads.
void TemperatureAnalysisParallel::startPipeline(const string &outputFile)
{
    // Create threads for each stage of the pipeline to achieve task parallelism
    thread readerThread(&TemperatureAnalysisParallel::fileReader, this);
    thread parserThread(&TemperatureAnalysisParallel::parser, this);
    thread processorThread(&TemperatureAnalysisParallel::anomalyDetector, this);
    thread writerThread(&TemperatureAnalysisParallel::fileWriter, this, outputFile);

    // Join all threads to ensure they finish before exiting the main thread
    readerThread.join();
    parserThread.join();
    processorThread.join();
    writerThread.join();
}

// Stage 1: Reads data from the file and pushes to readQueue
// Coordination & Synchronization: Protects access to readQueue with readMutex and notifies the parser when new data is available.
void TemperatureAnalysisParallel::fileReader()
{
    string line;
    while (getline(inputFile, line))
    {
        // Synchronization: Locking the readMutex ensures thread-safe access to readQueue
        unique_lock<mutex> lock(readMutex);
        readQueue.push(line);
        readCond.notify_one(); // Notify parser thread that new data is available
    }
    {
        // Send a sentinel value to signal completion of reading
        unique_lock<mutex> lock(readMutex);
        readQueue.push("-1");
        readCond.notify_one();
        printf("finished reading... (STEP 1)\n");
    }
    inputFile.close();
}

// Stage 2: Parses each line into TemperatureData and pushes to parseQueue
// Coordination & Synchronization: Waits for data in readQueue, signaling the anomaly detector after parsing each line.
void TemperatureAnalysisParallel::parser()
{
    while (true)
    {
        // Coordination: Wait until there is data available to parse
        unique_lock<mutex> lock(readMutex);
        readCond.wait(lock, [this]
                      { return !readQueue.empty(); });

        // Check for sentinel value before processing
        if (readQueue.empty())
        {
            continue;
        }

        auto line = readQueue.front();
        readQueue.pop();
        lock.unlock(); // Unlock for other threads to access

        // Check for sentinel after finished reading file
        if (line == "-1")
        {
            break; // Exit loop if sentinel is found
        }

        TemperatureData data = parseLine(line);
        if (data.month == INT_MAX)
        {
            continue; // Skip invalid month
        }

        // Synchronization: Locking the parseMutex ensures thread-safe access to parseQueue
        unique_lock<mutex> parseLock(parseMutex);
        parseQueue.push(data);
        parseCond.notify_one(); // Notify anomaly detector that new data is available
    }

    // Push sentinel value to indicate completion
    {
        unique_lock<mutex> lock(parseMutex);
        parseQueue.push(TemperatureData()); // Push an invalid TemperatureData as sentinel
        parseCond.notify_one();
    }

    printf("ALL DONE PARSE QUEUE METHOD (STEP 2)\n");
}

// Stage 3: Processes each TemperatureData for anomalies and pushes to processQueue
// Partitioning, Load Balancing, & Synchronization: Each month’s data is processed in separate threads, ensuring balanced load.
// Protects processQueue with processMutex.
void TemperatureAnalysisParallel::anomalyDetector()
{
    vector<thread> threads; // Container for threads evaluating monthly data
    unordered_map<Month, unordered_map<Hour, vector<double>>> monthlyData;
    unordered_map<Hour, double> lastTemperature; // Tracks the last temperature per Hour

    Month currentMonth(-1, -1); // Initialize to an invalid month

    while (true)
    {
        // Coordination: Wait until there is data available to process
        unique_lock<mutex> lock(parseMutex);
        parseCond.wait(lock, [this]
                       { return !parseQueue.empty(); });

        auto data = parseQueue.front();
        parseQueue.pop();
        lock.unlock();

        // Check for sentinel value to terminate processing
        if (data.month == 0 && data.day == 0 && data.year == 0)
        {
            break; // Exit the loop if sentinel is found
        }

        Month monthKey(data.year, data.month);
        Hour hourKey(data.day, data.hour);

        // Detect anomaly based on the last temperature
        if (lastTemperature.find(hourKey) != lastTemperature.end())
        {
            double previousTemp = lastTemperature[hourKey];
            if (isAnomaly(data.temperature, previousTemp))
            {
                continue; // Skip this entry as it's an anomaly
            }
        }

        // Store the current temperature
        lastTemperature[hourKey] = data.temperature;                // Track the last temperature for this Hour
        monthlyData[monthKey][hourKey].push_back(data.temperature); // Store temperature by month and hour

        // Check if the month has changed
        if (currentMonth.month != data.month || currentMonth.year != data.year)
        {
            // Partitioning & Load Balancing: Each month’s data is evaluated in a new thread to ensure balanced processing
            if (currentMonth.month != -1)
            {
                threads.push_back(thread(&TemperatureAnalysisParallel::evaluateMonthlyTemperatures,
                                         this, currentMonth, monthlyData[currentMonth]));
            }

            // Reset for the new month
            lastTemperature.clear(); // Clear last temperature data
            currentMonth = monthKey;
        }
    }

    // Join all threads before finishing
    for (auto &t : threads)
    {
        if (t.joinable())
        {
            t.join(); // Ensure all threads complete before proceeding
        }
    }

    // Push sentinel value to indicate completion of processing
    unique_lock<mutex> lock(processMutex);
    processQueue.push(TemperatureDataOut()); // Push an invalid TemperatureDataOut as sentinel
    processCond.notify_all();

    printf("ALL DONE ANOMALY DETECT METHOD (STEP 3)\n");
}

// Function to calculate mean and standard deviation and evaluate temperatures
// Partitioning: Data is processed month by month, reducing contention across threads.
// Synchronization: Uses processMutex to ensure safe access to processQueue.
void TemperatureAnalysisParallel::evaluateMonthlyTemperatures(Month month, const std::unordered_map<Hour, std::vector<double>> &temperatures)
{
    // Partitioning: Ensures that data for each month is evaluated separately
    if (temperatures.empty())
    {
        return; // No data to process
    }

    // Calculate mean and standard deviation
    double mean = calculateMean(temperatures);
    double stddev = calculateStdDev(temperatures, mean);

    // Process temperatures for heating/cooling issues
    for (const auto &hourEntry : temperatures)
    {
        const Hour &hourKey = hourEntry.first; // Extract hour key
        int currentHour = hourKey.hour;        // Access the hour
        int currentDay = hourKey.day;          // Extract day

        for (double temp : hourEntry.second)
        {
            // Synchronization: Use processMutex to ensure safe writing to the processQueue
            if ((isHeatingMonth(month.month) && temp > mean + stddev) || (isCoolingMonth(month.month) && temp < mean - stddev))
            {
                TemperatureDataOut data(month.month, currentDay, month.year, currentHour, 0, 0, temp, mean, stddev);

                unique_lock<mutex> processLock(processMutex);
                processQueue.push(data);  // Push detected issue to the queue
                processCond.notify_one(); // Notify writer thread that new data is available
                break;                    // Stop processing after first issue in this hour
            }
        }
    }
}

// Stage 4: Writes results to the output file
// Coordination & Synchronization: Waits for data in processQueue and synchronizes access with processMutex to safely write to the file.
void TemperatureAnalysisParallel::fileWriter(const string &outputFile)
{
    ofstream outFile(outputFile);
    while (true)
    {
        // Coordination: Wait until there are results to write
        unique_lock<mutex> lock(processMutex);
        processCond.wait(lock, [this]
                         { return !processQueue.empty(); });

        TemperatureDataOut result = processQueue.front();
        processQueue.pop();
        lock.unlock();

        // Check for sentinel value to terminate
        if (result.month == 0 && result.day == 0 && result.year == 0)
        {
            break; // Exit the loop if sentinel is found
        }

        // Format and write the result to the output file
        if (isHeatingMonth(result.month))
        {
            outFile << "Heating issue detected: "
                    << result.month << "/" << result.day << "/" << result.year
                    << " At Hour: " << result.hour << " | Temp: " << result.temperature
                    << " | Mean: " << result.mean << " | Stddev: " << result.stddev << endl;
        }
        else if (isCoolingMonth(result.month))
        {
            outFile << "Cooling issue detected: "
                    << result.month << "/" << result.day << "/" << result.year
                    << " At Hour: " << result.hour << " | Temp: " << result.temperature
                    << " | Mean: " << result.mean << " | Stddev: " << result.stddev << endl;
        }

        // Flush the output to ensure it's written immediately
        outFile.flush();
    }
    printf("ALL DONE FILE WRITE METHOD (STEP 4)\n");
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
        return TemperatureData(INT_MAX, 0, 0, 0, 0, 0, 0.0); // Return an invalid TemperatureData
    }

    // Parse the line
    ss >> month >> delim >> day >> delim >> year >> hour >> delim >> minute >> delim >> second >> temperature;

    // Return structured data
    return TemperatureData(month, day, year, hour, minute, second, temperature);
}

// Helper function to calculate mean temperature
double TemperatureAnalysisParallel::calculateMean(const unordered_map<Hour, vector<double>> &temperatures)
{
    double total = 0;
    int count = 0;

    for (const auto &hourEntry : temperatures)
    {
        for (double temp : hourEntry.second)
        {
            total += temp;
            count++;
        }
    }

    return (count > 0) ? (total / count) : 0;
}

// Helper function to calculate standard deviation
double TemperatureAnalysisParallel::calculateStdDev(const unordered_map<Hour, vector<double>> &temperatures, double mean)
{
    double sumSquaredDiffs = 0;
    int count = 0;

    for (const auto &hourEntry : temperatures)
    {
        for (double temp : hourEntry.second)
        {
            sumSquaredDiffs += (temp - mean) * (temp - mean);
            count++;
        }
    }

    return (count > 1) ? sqrt(sumSquaredDiffs / (count - 1)) : 0; // Using sample std dev
}

// Helper function to determine if temperature is an anomaly
bool TemperatureAnalysisParallel::isAnomaly(double current, double previous)
{
    return fabs(current - previous) > 2.0; // Custom threshold for anomalies
}

// Check if month is designated for heating
bool TemperatureAnalysisParallel::isHeatingMonth(int month)
{
    return find(heatingMonths.begin(), heatingMonths.end(), month) != heatingMonths.end();
}

// Check if month is designated for cooling
bool TemperatureAnalysisParallel::isCoolingMonth(int month)
{
    return find(coolingMonths.begin(), coolingMonths.end(), month) != coolingMonths.end();
}
