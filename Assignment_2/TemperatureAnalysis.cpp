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

    // Create threads to process the file
    for (int i = 0; i < numThreads; ++i) {
        ThreadArgs* threadArgs = new ThreadArgs(); // Dynamically allocate new ThreadArgs
        threadArgs->startPosition = i * segmentSize;
        threadArgs->endPosition = (i == numThreads - 1) ? fileSize : (i + 1) * segmentSize;
        threadArgs->threadId = i;
        threadArgs->analysis = this; // Assign this to the analysis member

        int ret = pthread_create(&threads[i], NULL, [](void* args) -> void* {
            ThreadArgs* threadArgs = static_cast<ThreadArgs*>(args);
            return threadArgs->analysis->processSegment(args);
        }, threadArgs); // Pass the dynamically allocated threadArgs

        if (ret) {
            std::cerr << "Error creating thread " << i << ": " << ret << std::endl;
            delete threadArgs; // Cleanup if thread creation fails
            return;  // Handle thread creation error
        }
    }

    // Join threads
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL);
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
    ThreadArgs* threadArgs = static_cast<ThreadArgs*>(args);
    int month = threadArgs->threadId + 1; 
    double sum = 0.0;
    int count = 0;

    // Lock the mutex to access the dataset safely
    datasetMutex.lock();
    for (const auto& entry : dataset) {
        const hourlyData& hData = entry.first;
        const std::tuple<double, int>& tempData = entry.second;

        // Check if the month matches
        if (hData.month == month) {
            sum += std::get<0>(tempData);
            count += std::get<1>(tempData);
        }
    }
    datasetMutex.unlock();

    double mean = (count > 0) ? sum / count : 0.0;

    // Calculate the standard deviation
    double varianceSum = 0.0;
    datasetMutex.lock();
    for (const auto& entry : dataset) {
        const hourlyData& hData = entry.first;
        const std::tuple<double, int>& tempData = entry.second;

        // Check if the month matches
        if (hData.month == month) {
            double temp = std::get<0>(tempData) / std::get<1>(tempData); // Average temperature for that hour
            varianceSum += std::pow(temp - mean, 2);
        }
    }
    datasetMutex.unlock();

    double stddev = (count > 0) ? std::sqrt(varianceSum / count) : 0.0;

    // Lock the report mutex to safely write to the report file
    reportMutex.lock();
    std::cout << "Month: " << month << " | Mean Temperature: " << mean
              << " | Standard Deviation: " << stddev << std::endl;
    reportMutex.unlock();

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
