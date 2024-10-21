#include "TemperatureAnalysis.h"

using namespace std;

TemperatureAnalysis::TemperatureAnalysis(const string &filename)
{
    this->numThreads = 12;
    initializeFile(filename); // Ensure file is opened successfully

    this->fileSize = inputFile.tellg(); // Get file size
    if (fileSize <= 0)
    {
        cerr << "File is empty or couldn't determine file size." << endl;
        exit(EXIT_FAILURE); // Handle file size error
    }

    inputFile.seekg(0, ios::beg); // Reset file position to beginning
    this->segmentSize = fileSize / numThreads;

    // Optional: Print out the initialized values for debugging
    cout << "File Size: " << fileSize << ", Segment Size: " << segmentSize << endl;

    pthread_mutex_init(&reportMutex, NULL); // Initialize the mutex
}

TemperatureAnalysis::~TemperatureAnalysis()
{
    if (inputFile.is_open())
    {
        inputFile.close();
    }

    pthread_mutex_destroy(&reportMutex); // Destroy the mutex
}

/**
 * Used to initialize and open the file
 * @arg filename - name of data file
 * @retval True if the file exists, false otherwise
 */
void TemperatureAnalysis::initializeFile(const string &filename)
{
    inputFile.open(filename);
    if (!inputFile.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE); // Handle file open error appropriately
    }
    // Seek to the end to determine file size
    inputFile.seekg(0, ios::end);
}

/**
 * Processes temperature data from a log file in parallel using multiple threads.
 * Each thread handles a segment of the file, parsing temperature records and
 * updating the shared dataset with average temperature values for each hour.
 */
void TemperatureAnalysis::processTemperatureData(void)
{
    pthread_t threads[numThreads];
    ThreadArgs *threadArgs[numThreads]; // Declare an array of ThreadArgs pointers

    // Create threads to process the file
    for (int i = 0; i < numThreads; ++i)
    {
        threadArgs[i] = new ThreadArgs(); // Dynamically allocate new ThreadArgs for each thread
        threadArgs[i]->startPosition = i * segmentSize;
        threadArgs[i]->endPosition = (i == numThreads - 1) ? fileSize : (i + 1) * segmentSize;
        threadArgs[i]->threadId = i;
        threadArgs[i]->analysis = this; // Assign this to the analysis member

        int ret = pthread_create(&threads[i], NULL, &TemperatureAnalysis::threadFunction, threadArgs[i]); // Use static member function

        if (ret)
        {
            cerr << "Error creating thread " << i << ": " << ret << endl;
            delete threadArgs[i]; // Cleanup if thread creation fails
            return;               // Handle thread creation error
        }
    }

    // Join threads
    for (int i = 0; i < numThreads; ++i)
    {
        pthread_join(threads[i], NULL);
        delete threadArgs[i]; // Clean up allocated memory for each threadArgs
    }

    // close file
    inputFile.close();
}

/**
 * Static thread function to process a segment of the temperature data from the input file.
 * @param args Pointer to ThreadArgs struct containing the start and end positions for processing.
 * @return NULL
 */
void *TemperatureAnalysis::threadFunction(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    return threadArgs->analysis->processSegment(args);
}

/**
 * Processes a segment of the temperature data from the input file.
 * @param args Pointer to ThreadArgs struct containing the start and end positions for processing.
 * @return NULL
 */
void *TemperatureAnalysis::processSegment(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;

    long startPos = threadArgs->startPosition;
    long endPos = threadArgs->endPosition;

    // Seek to the start position
    inputFile.seekg(startPos);

    // Ensure we start at the beginning of a line
    if (startPos != 0)
    {
        string temp;
        getline(inputFile, temp);
    }

    string line;
    while (inputFile.tellg() < endPos && getline(inputFile, line))
    {
        TemperatureData data = parseLine(line);

        if (data.hour == INT_MAX)
        {
            continue; // Skip invalid data lines
        }

        if (find(coolingMonths.begin(), coolingMonths.end(), data.month) == coolingMonths.end() && find(heatingMonths.begin(), heatingMonths.end(), data.month) == heatingMonths.end())
        {
            continue; // skip months we dont care about
        }

        hourlyData current_hour(data.year, data.month, data.day, data.hour);

        // populate the dataset by hour
        datasetMutex.lock();
        if (dataset.find(current_hour) != dataset.end() && !dataset[current_hour].empty()) {
            // If the current hour exists and the vector is not empty, check for anomaly
            if (!isAnomaly(dataset[current_hour].back(), data.temperature)) {
                dataset[current_hour].push_back(data.temperature);
            } else {
                datasetMutex.unlock();
                continue;  // Skip if there's no anomaly
            }
        } else {
            // If current hour doesn't exist or the vector is empty, add the temperature directly
            dataset[current_hour].push_back(data.temperature);
        }
        datasetMutex.unlock();

        // Lock mutex to safely update the shared dataset
        hourlyAvgMutex.lock();
        // Update hourly average dataset
        if (hourlyAvg.find(current_hour) == hourlyAvg.end())
        {
            hourlyAvg[current_hour] = make_tuple(data.temperature, 1);
        }
        else
        {
            hourlyAvg[current_hour] = make_tuple(
                get<0>(hourlyAvg[current_hour]) + data.temperature,
                get<1>(hourlyAvg[current_hour]) + 1);
        }
        hourlyAvgMutex.unlock();
    }
    return NULL;
}

/**
 * Generates a report of mean temperatures and standard deviations for each month.
 * @param reportName The name of the file where the report will be written.
 */
void TemperatureAnalysis::generateReport(const string &reportName)
{
    ofstream reportFile(reportName.c_str());
    if (!reportFile.is_open())
    {
        cerr << "Error opening report file!" << endl;
        return;
    }

    // Initialize mutex
    pthread_mutex_init(&reportMutex, NULL);

    pthread_t heatingThreads[heatingMonths.size()];
    pthread_t coolingThreads[coolingMonths.size()];
    ReportArgs heatingArgs[heatingMonths.size()];
    ReportArgs coolingArgs[coolingMonths.size()];

    // Create heating threads
    for (size_t i = 0; i < heatingMonths.size(); ++i)
    {
        heatingArgs[i] = {this, heatingMonths[i], &reportFile};
        pthread_create(&heatingThreads[i], NULL, processHeatingMonth, (void *)&heatingArgs[i]);
    }

    // Create cooling threads
    for (size_t i = 0; i < coolingMonths.size(); ++i)
    {
        coolingArgs[i] = {this, coolingMonths[i], &reportFile};
        pthread_create(&coolingThreads[i], NULL, processCoolingMonth, (void *)&coolingArgs[i]);
    }

    // Join heating threads
    for (size_t i = 0; i < heatingMonths.size(); ++i)
    {
        pthread_join(heatingThreads[i], NULL);
    }

    // Join cooling threads
    for (size_t i = 0; i < coolingMonths.size(); ++i)
    {
        pthread_join(coolingThreads[i], NULL);
    }

    // Destroy mutex
    pthread_mutex_destroy(&reportMutex);
    reportFile.close();
}

/**
 * Parses string that is read in from the temperature log file. It stores the date, time,
 * and temperature into separate fields and wraps them in a TemperatureData struct object
 * which is then returned such that data from each reading can be easily accessed.
 * @arg line - string from the log file
 * @retval struct defined in TemperatureAnalysis.h which holds date, time, temperature
 */
TemperatureData TemperatureAnalysis::parseLine(const string &line)
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

/**
 * Determines if the current temperature is an anomaly by comparing it to the previous temperature.
 * If there is a difference of two or more degrees, then the data point will be thrown out
 * @arg currentTemp - temperature which is being checked as an anomaly
 * @arg previousTemp - average temperature of the hourly bucket
 * @retval true if currentTemp is an anomaly, false otherwise
 */
bool TemperatureAnalysis::isAnomaly(double currentTemp, double previousTemp)
{
    return abs(currentTemp - previousTemp) > 2.0;
}

void TemperatureAnalysis::setHeatingMonths(const vector<int> &months)
{
    heatingMonths = months;
}

void TemperatureAnalysis::setCoolingMonths(const vector<int> &months)
{
    coolingMonths = months;
}

// Process Cooling Month: Detect temperatures below 1 standard deviation (for cooling)
void *TemperatureAnalysis::processCoolingMonth(void *args)
{
    ReportArgs *reportArgs = (ReportArgs *)args;
    TemperatureAnalysis *analysis = reportArgs->analysis;
    int month = reportArgs->month;
    ofstream &reportFile = *reportArgs->reportFile;

    // Loop through each hour in hourlyAvg for the given month
    for (const auto &hourEntry : analysis->hourlyAvg)
    {
        const hourlyData &hData = hourEntry.first;
        if (hData.month == month)
        {
            double sumTemps = get<0>(hourEntry.second);
            int countTemps = get<1>(hourEntry.second);

            if (countTemps > 0)
            {
                // Calculate mean temperature
                double mean = sumTemps / countTemps;
                double variance = 0.0;

                // Calculate variance for the current hour
                for (const auto &tempEntry : analysis->dataset[hData])
                {
                    variance += (tempEntry - mean) * (tempEntry - mean);
                }
                variance /= countTemps;
                double stddev = sqrt(variance);

                // Check for cooling issues: temp < (mean - stddev)
                for (const auto &tempEntry : analysis->dataset[hData])
                {
                    if (tempEntry < (mean - stddev))
                    {
                        pthread_mutex_lock(&analysis->reportMutex); // Lock mutex
                        reportFile << "Cooling issue detected: " << month << "/" << hData.day << "/" << hData.year
                                   << " At Hour: " << hData.hour << " | Temp: " << tempEntry << endl;
                        pthread_mutex_unlock(&analysis->reportMutex); // Unlock mutex
                        break;                                        // Stop further checks for this hour if an issue is found
                    }
                }
            }
        }
    }

    return nullptr;
}

// Process Heating Month: Detect temperatures above 1 standard deviation (for heating)
void *TemperatureAnalysis::processHeatingMonth(void *args)
{
    ReportArgs *reportArgs = (ReportArgs *)args;
    TemperatureAnalysis *analysis = reportArgs->analysis;
    int month = reportArgs->month;
    ofstream &reportFile = *reportArgs->reportFile;

    // Loop through each hour in hourlyAvg for the given month
    for (const auto &hourEntry : analysis->hourlyAvg)
    {
        const hourlyData &hData = hourEntry.first;
        if (hData.month == month)
        {
            double sumTemps = get<0>(hourEntry.second);
            int countTemps = get<1>(hourEntry.second);

            if (countTemps > 0)
            {
                // Calculate mean temperature
                double mean = sumTemps / countTemps;
                double variance = 0.0;

                // Calculate variance for the current hour
                for (const auto &tempEntry : analysis->dataset[hData])
                {
                    variance += (tempEntry - mean) * (tempEntry - mean);
                }
                variance /= countTemps;
                double stddev = sqrt(variance);

                // Check for heating issues: temp > (mean + stddev)
                for (const auto &tempEntry : analysis->dataset[hData])
                {
                    if (tempEntry > (mean + stddev))
                    {
                        pthread_mutex_lock(&analysis->reportMutex); // Lock mutex
                        reportFile << "Heating issue detected: " << month << "/" << hData.day << "/" << hData.year
                                   << " At Hour: " << hData.hour << " | Temp: " << tempEntry << endl;
                        pthread_mutex_unlock(&analysis->reportMutex); // Unlock mutex
                        break;                                        // Stop further checks for this hour if an issue is found
                    }
                }
            }
        }
    }

    return nullptr;
}
