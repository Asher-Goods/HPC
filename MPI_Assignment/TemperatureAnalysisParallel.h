#ifndef TEMPERATURE_ANALYSIS_PARALLEL_H
#define TEMPERATURE_ANALYSIS_PARALLEL_H

#include <condition_variable>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <limits.h>

using namespace std;

struct TemperatureData {
    static const int dataSize = 7; // Number of fields in TemperatureData

    int month, day, year, hour, minute, second;
    double temperature;

    TemperatureData() : month(0), day(0), year(0), hour(0), minute(0), second(0), temperature(0.0) {}
    TemperatureData(int month, int day, int year, int hour, int minute, int second, double temperature)
        : month(month), day(day), year(year), hour(hour), minute(minute), second(second), temperature(temperature) {}

    // Converts the data to an array for MPI transfer
    void toArray(double* arr) const {
        arr[0] = month;
        arr[1] = day;
        arr[2] = year;
        arr[3] = hour;
        arr[4] = minute;
        arr[5] = second;
        arr[6] = temperature;
    }

    // Fills the data from an array
    void fromArray(const double* arr) {
        month = static_cast<int>(arr[0]);
        day = static_cast<int>(arr[1]);
        year = static_cast<int>(arr[2]);
        hour = static_cast<int>(arr[3]);
        minute = static_cast<int>(arr[4]);
        second = static_cast<int>(arr[5]);
        temperature = arr[6];
    }
};

// Constants for task roles
enum TaskRole { READER = 0, PARSER = 1, DETECTOR = 2, WRITER = 3 };

struct TemperatureDataOut
{
    int month;
    int day;
    int year;
    int hour;
    int minute;
    int second;
    double temperature;
    double mean;
    double stddev;

    // Regular constructor
    TemperatureDataOut(int m, int d, int y, int h, int mi, int s, double temp, double meanVal = 0.0, double stddevVal = 0.0)
        : month(m), day(d), year(y), hour(h), minute(mi), second(s), temperature(temp), mean(meanVal), stddev(stddevVal) {}

    // Sentinel constructor
    TemperatureDataOut()
        : month(0), day(0), year(0), hour(0), minute(0), second(0),
          temperature(0.0), mean(0.0), stddev(0.0) {}
};

struct Hour
{
    int day;
    int hour;

    // Constructor
    Hour(int d, int h) : day(d), hour(h) {}

    // Define equality operator for unordered_map comparisons
    bool operator==(const Hour &other) const
    {
        return (day == other.day && hour == other.hour);
    }
};

// Define a hash specialization for Hour
namespace std
{
    template <>
    struct hash<Hour>
    {
        std::size_t operator()(const Hour &h) const
        {
            // Combine the hashes of 'day' and 'hour' uniquely
            return std::hash<int>()(h.day) ^ (std::hash<int>()(h.hour) << 1);
        }
    };
}

struct Month
{
    int year;
    int month;

    // Constructor that initializes year and month
    Month(int year = 0, int month = 0) : year(year), month(month) {}

    // Define equality operator
    bool operator==(const Month &other) const
    {
        return (year == other.year && month == other.month);
    }
};

// Hash function for Month
namespace std
{
    template <>
    struct hash<Month>
    {
        size_t operator()(const Month &month) const
        {
            return hash<int>()(month.year) ^ (hash<int>()(month.month) << 1);
        }
    };
}

// Class to perform temperature analysis in a task-parallel pipeline
class TemperatureAnalysisParallel
{
public:
    TemperatureAnalysisParallel(const string &filename);
    void setHeatingMonths(const vector<int> &months);
    void setCoolingMonths(const vector<int> &months);
    void startPipeline(const string &outputFile);

private:
    // Queue to store data between stages
    queue<string> readQueue;
    queue<TemperatureData> parseQueue;
    queue<TemperatureDataOut> processQueue;

    // Mutexes and condition variables for each stage
    mutex readMutex, parseMutex, processMutex;
    condition_variable readCond, parseCond, processCond;

    // File handling and configuration variables
    ifstream inputFile;
    vector<int> heatingMonths, coolingMonths;

    // Stage functions to handle each part of the pipeline
    void fileReader();
    void parser();
    void anomalyDetector();
    void fileWriter(const string &outputFile);

    // Helper functions
    TemperatureData parseLine(const string &line);
    bool isAnomaly(double currentTemp, double previousTemp);
    double calculateMean(const vector<double>& temperatures);
    double calculateStdDev(const vector<double>& temperatures, double mean);
    void evaluateMonthlyTemperatures(Month month, const std::unordered_map<Hour, std::vector<double>> &temperatures);
    bool isCoolingMonth(int month);
    bool isHeatingMonth(int month);
};

#endif // TEMPERATURE_ANALYSIS_PARALLEL_H
