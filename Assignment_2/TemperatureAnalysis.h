#ifndef TEMPERATURE_ANALYSIS_H
#define TEMPERATURE_ANALYSIS_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <sstream>
#include <limits.h>
#include <pthread.h>
#include <map>
#include <tuple>
#include <mutex>
#include <iostream>

using namespace std;

// Public structure to hold temperature data
struct TemperatureData
{
    int month;
    int day;
    int year;
    int hour;
    int minute;
    int second;
    double temperature;

    TemperatureData(int month, int day, int year, int hour, int minute, int second, double temperature)
        : month(month), day(day), year(year), hour(hour), minute(minute), second(second), temperature(temperature) {}
};

struct hourlyData
{
    int month;
    int day;
    int hour;

    // Overload the < operator for comparison
    bool operator<(const hourlyData& other) const {
        if (month != other.month) return month < other.month;
        if (day != other.day) return day < other.day;
        return hour < other.hour;
    }

    hourlyData(int month, int day, int hour) : month(month), day(day), hour(hour) {}
};

// TemperatureAnalysis class to encapsulate functionality
class TemperatureAnalysis
{

// From chat GPT
struct ThreadArgs {
    long startPosition;         // Start position for this thread
    long endPosition;           // End position for this thread
    int threadId;              // ID for the thread
    TemperatureAnalysis* analysis;  // Pointer to TemperatureAnalysis instance
};

public:
    // Constructor
    TemperatureAnalysis(string filename);

    // Destructor (to close file if necessary)
    ~TemperatureAnalysis();

    /**
     * Processes temperature data from a log file in parallel using multiple threads.
     * Each thread handles a segment of the file, parsing temperature records and
     * updating the shared dataset with average temperature values for each hour.
     */
    void processTemperatureData(void);

    /**
     * Generates a report of mean temperatures and standard deviations for each month.
     * This method creates threads to calculate statistics for each month and
     * writes the results to a specified output file.
     *
     * @param reportName The name of the file where the report will be written.
     */
    void generateReport(const std::string &reportName);

private:
    /**
     * Used to initialize and open the file
     * @param filename - name of data file
     * @retval True if the file exists, false otherwise
     */    
    bool initializeFile(const string &filename);

    /**
     * Determines if the current temperature is an anomaly by comparing it to the previous temperature.
     * If there is a difference of two or more degrees, then the data point will be thrown out
     * @param currentTemp - temperature which is being checked as an anomaly
     * @param previousTemp - average temperature of the hourly bucket
     * @retval true if currentTemp is an anomaly, false otherwise
     */
    bool isAnomaly(double currentTemp, double previousTemp);

    /**
     * Parses string that is read in from the temperature log file. It stores the date, time,
     * and temperature into separate fields and wraps them in a TemperatureData struct object
     * which is then returned such that data from each reading can be easily accessed.
     * @arg line - string from the log file
     * @retval struct defined in TemperatureAnalysis.h which holds date, time, temperature
     */
    TemperatureData parseLine(const string &line);

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
    void* processSegment(void* args);

    /**
     * Calculates the mean temperature and standard deviation for a specified month.
     * This method is intended to be executed by a thread and retrieves data from
     * the shared dataset, computing the statistics while ensuring thread safety.
     *
     * @param args Pointer to an integer representing the month for which to 
     *             calculate the statistics.
     * @return NULL
     */
    void* reportMonth(void* args);
    

    // private instantiation of input file field
    ifstream inputFile;
    // data set which contains all the parsed file data
    map<hourlyData, tuple<double, int>> dataset;

    // mutexes to ensure no write errors
    mutex datasetMutex;
    mutex reportMutex;

    // file characteristics
    int numThreads;
    long fileSize;
    long segmentSize;
};

#endif // TEMPERATURE_ANALYSIS_H
