#include "TemperatureAnalysis.h"

using namespace std;

TemperatureAnalysis::TemperatureAnalysis(const std::string &filename) {
    this->numThreads = 12;
    initializeFile(filename); // Ensure file is opened successfully

    this->fileSize = inputFile.tellg();  // Get file size
    if (fileSize <= 0) {
        std::cerr << "File is empty or couldn't determine file size." << std::endl;
        exit(EXIT_FAILURE); // Handle file size error
    }
    
    inputFile.seekg(0, std::ios::beg);  // Reset file position to beginning
    this->segmentSize = fileSize / numThreads;

    // Optional: Print out the initialized values for debugging
    std::cout << "File Size: " << fileSize << ", Segment Size: " << segmentSize << std::endl;
}


TemperatureAnalysis::~TemperatureAnalysis()
{
    if (inputFile.is_open())
    {
        inputFile.close();
    }
}

/**
 * Used to initialize and open the file
 * @arg filename - name of data file
 * @retval True if the file exists, false otherwise
 */
void TemperatureAnalysis::initializeFile(const std::string &filename) {
    inputFile.open(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        exit(EXIT_FAILURE); // Handle file open error appropriately
    }
    // Seek to the end to determine file size
    inputFile.seekg(0, std::ios::end);
}

/**
 * Processes temperature data from a log file in parallel using multiple threads.
 * Each thread handles a segment of the file, parsing temperature records and
 * updating the shared dataset with average temperature values for each hour.
 */
void TemperatureAnalysis::processTemperatureData(void) {
    pthread_t threads[numThreads];
    ThreadArgs* threadArgs[numThreads];  // Declare an array of ThreadArgs pointers

    // Create threads to process the file
    for (int i = 0; i < numThreads; ++i) {
        threadArgs[i] = new ThreadArgs(); // Dynamically allocate new ThreadArgs for each thread
        threadArgs[i]->startPosition = i * segmentSize;
        threadArgs[i]->endPosition = (i == numThreads - 1) ? fileSize : (i + 1) * segmentSize;
        threadArgs[i]->threadId = i;
        threadArgs[i]->analysis = this; // Assign this to the analysis member

        int ret = pthread_create(&threads[i], NULL, [](void* args) -> void* {
            ThreadArgs* threadArgs = static_cast<ThreadArgs*>(args);
            return threadArgs->analysis->processSegment(args);
        }, threadArgs[i]); // Pass the dynamically allocated threadArgs

        if (ret) {
            std::cerr << "Error creating thread " << i << ": " << ret << std::endl;
            delete threadArgs[i]; // Cleanup if thread creation fails
            return;  // Handle thread creation error
        }
    }

    // Join threads
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL);
        delete threadArgs[i]; // Clean up allocated memory for each threadArgs
    }

    // close file
    inputFile.close();
}



/**
 * Processes a segment of the temperature data from the input file.
 * This method is intended to be executed by a thread and reads from the
 * specified start and end positions of the file. It parses each line of
 * data, updates the shared dataset, and ensures thread safety using a mutex.
 *
 * @param args Pointer to ThreadArgs struct containing the start and end 
 *             positions for processing, as well as the thread's ID.
 * @return NULL
 */
void* TemperatureAnalysis::processSegment(void* args)
{
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    
    long startPos = threadArgs->startPosition;
    long endPos = threadArgs->endPosition;
    
    // Seek to the start position
    inputFile.seekg(startPos);

    // Ensure we start at the beginning of a line
    if (startPos != 0) {
        std::string temp;
        std::getline(inputFile, temp);
    }

    std::string line;
    while (inputFile.tellg() < endPos && getline(inputFile, line)) {
        TemperatureData data = parseLine(line);

        if (data.hour == INT_MAX) {
            continue;  // Skip invalid data lines
        }

        hourlyData current_hour(data.month, data.day, data.hour);

        // Lock mutex to safely update the shared dataset
        datasetMutex.lock();
        if (dataset.find(current_hour) == dataset.end()) {
            dataset[current_hour] = std::make_tuple(data.temperature, 1);
        } else {
            double previous_temp = std::get<0>(dataset[current_hour]) / std::get<1>(dataset[current_hour]);
            if (!isAnomaly(data.temperature, previous_temp)) {
                dataset[current_hour] = std::make_tuple(
                    std::get<0>(dataset[current_hour]) + data.temperature,
                    std::get<1>(dataset[current_hour]) + 1
                );
            }

        }
        datasetMutex.unlock();
    }
    return NULL;
}

/**
 * Generates a report of mean temperatures and standard deviations for each month.
 * This method creates threads to calculate statistics for each month and
 * writes the results to a specified output file.
 *
 * @param reportName The name of the file where the report will be written.
 */
void TemperatureAnalysis::generateReport(const std::string &reportName)
{
    std::ofstream reportFile(reportName.c_str());
    if (!reportFile.is_open()) {
        std::cerr << "Error opening report file!" << std::endl;
        return;
    }

    pthread_t threads[12];
    ThreadArgs threadArgs[12];

    // Create threads to process each month
    for (int month = 1; month <= 12; ++month) {
        threadArgs[month - 1].threadId = month - 1; // Store the month in args
        threadArgs[month - 1].analysis = this; // Store the pointer to the TemperatureAnalysis instance

        int ret = pthread_create(&threads[month - 1], NULL, [](void* args) -> void* {
            ThreadArgs* threadArgs = static_cast<ThreadArgs*>(args);
            return threadArgs->analysis->reportMonth(args); // Call reportMonth with args
        }, (void*)&threadArgs[month - 1]);

        if (ret) {
            std::cerr << "Error creating thread for month " << month << ": " << ret << std::endl;
            reportFile.close(); // Close the file before returning
            return; // Handle thread creation error
        }
    }

    // Join threads
    for (int i = 0; i < 12; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Print out the monthly data
    for (int month = 1; month <= 12; ++month) {
        auto& [mean, stddev] = monthlyData[month - 1];
        std::cout << "Month: " << month << " | Mean: " << mean << " | Std Dev: " << stddev << std::endl;
        // Optionally, write to the report file as well
        reportFile << "Month: " << month << " | Mean: " << mean << " | Std Dev: " << stddev << std::endl;
    }

    reportFile.close();
}



/**
 * Calculates the mean temperature and standard deviation for a specified month.
 * This method is intended to be executed by a thread and retrieves data from
 * the shared dataset, computing the statistics while ensuring thread safety.
 *
 * @param args Pointer to an integer representing the month for which to 
 *             calculate the statistics.
 * @return NULL
 */
void* TemperatureAnalysis::reportMonth(void* args) {
    int month = static_cast<ThreadArgs*>(args)->threadId + 1; // Extract month
    double totalTemperature = 0.0;
    int count = 0;

    // Check if the dataset has data for the given month
    for (const auto& entry : dataset) {
        const auto& [hour, day, month_key] = entry.first;
        if (month_key == month) {
            totalTemperature += std::get<0>(entry.second);
            count += std::get<1>(entry.second);
        }
    }

    reportMutex.lock(); // Lock the report mutex
    auto& [mean, stddev] = monthlyData[month - 1]; // Access the corresponding tuple

    if (count > 0) {
        mean = totalTemperature / count; // Calculate mean

        double variance = 0.0;

        // Calculate variance for standard deviation
        for (const auto& entry : dataset) {
            const auto& [hour, day, month_key] = entry.first;
            if (month_key == month) {
                double temp = std::get<0>(entry.second) / std::get<1>(entry.second);
                variance += (temp - mean) * (temp - mean);
            }
        }
        variance /= count;  // Variance calculation
        stddev = std::sqrt(variance); // Calculate standard deviation
    } else {
        mean = 0.0; // Set mean to 0 if no data
        stddev = 0.0; // Set stddev to 0 if no data
    }

    reportMutex.unlock(); // Unlock the report mutex
    return NULL;
}



/**
 * Parses string that is read in from the temperature log file. It stores the date, time,
 * and temperature into separate fields and wraps them in a TemperatureData struct object
 * which is then returned such that data from each reading can be easily accessed.
 * @arg line - string from the log file
 * @retval struct defined in TemperatureAnalysis.h which holds date, time, temperature
 */
TemperatureData TemperatureAnalysis::parseLine(const std::string &line)
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
    return std::abs(currentTemp - previousTemp) > 2.0;
}


void TemperatureAnalysis::calculateMonthlyMeans() {
        std::vector<double> monthlyMeans(12, 0.0); // 12 months
        std::vector<int> monthlyCounts(12, 0);     // Count for each month

        // Lock the datasetMutex for thread-safe access
        datasetMutex.lock();
        for (const auto& entry : dataset) {
            const hourlyData& hData = entry.first;
            const std::tuple<double, int>& tempData = entry.second;

            // Increment the sum and count for the corresponding month
            if (hData.month >= 1 && hData.month <= 12) {
                monthlyMeans[hData.month - 1] += std::get<0>(tempData); // Add the sum of temperatures
                monthlyCounts[hData.month - 1] += std::get<1>(tempData); // Add the count of readings
            }
        }
        datasetMutex.unlock();

        // Print the results
        for (int month = 1; month <= 12; ++month) {
            if (monthlyCounts[month - 1] > 0) {
                double mean = monthlyMeans[month - 1] / monthlyCounts[month - 1];
                std::cout << "Month: " << month << " | Mean Temperature: " << mean << std::endl;
            } else {
                std::cout << "Month: " << month << " | No Data" << std::endl;
            }
        }
    }