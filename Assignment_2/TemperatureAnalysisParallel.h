#ifndef TEMPERATURE_ANALYSIS_PARALLEL_H
#define TEMPERATURE_ANALYSIS_PARALLEL_H

#include <condition_variable>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <tuple>
#include <functional>
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
    int month, day, year, hour, minute, second;
    double temperature;
    bool isValid; // Indicates if the data is valid

    TemperatureData(int m, int d, int y, int h, int mi, int s, double temp)
        : month(m), day(d), year(y), hour(h), minute(mi), second(s), temperature(temp), isValid(true) {}

    TemperatureData() : month(0), day(0), year(0), hour(0), minute(0), second(0), temperature(0), isValid(false) {} // Default constructor for sentinel
};

struct TemperatureDataOut {
    int month;
    int day;
    int year;
    int hour;
    int minute;
    int second;
    double temperature;
    double mean;      
    double stddev;    

    TemperatureDataOut(int m, int d, int y, int h, int mi, int s, double temp, double meanVal = 0.0, double stddevVal = 0.0)
        : month(m), day(d), year(y), hour(h), minute(mi), second(s), temperature(temp), mean(meanVal), stddev(stddevVal) {}
};

struct Hour {
    int day;
    int hour;
    
    Hour(int d, int h) : day(d), hour(h) {}

    // Overload the equality operator
    bool operator==(const Hour& other) const {
        return day == other.day && hour == other.hour;
    }
};

// Define a hash function for Hour
struct HourHash {
    std::size_t operator()(const Hour& h) const {
        return std::hash<int>()(h.day) ^ (std::hash<int>()(h.hour) << 1); // Combine hashes of day and hour
    }
};

struct TimeKey {
    int year;
    int month;
    int day;   // Not used for the hash
    int hour;  // Not used for the hash

    // Default constructor
    TimeKey() : year(0), month(0), day(0), hour(0) {}

    // Constructor with parameters
    TimeKey(int y, int m, int d, int h) : year(y), month(m), day(d), hour(h) {}

    // Equality operator
    bool operator==(const TimeKey& other) const {
        return year == other.year && month == other.month;
    }
};

// Custom hash function for TimeKey
struct TimeKeyHash {
    std::size_t operator()(const TimeKey& key) const {
        // Combine year and month for the hash value
        return std::hash<int>()(key.year) ^ (std::hash<int>()(key.month) << 1);
    }
};


// Class to perform temperature analysis in a task-parallel pipeline
class TemperatureAnalysisParallel {
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

    TemperatureData data;

    // Stage functions to handle each part of the pipeline
    void fileReader();
    void parser();
    void anomalyDetector();
    void fileWriter(const string &outputFile);

    // Helper functions
    TemperatureData parseLine(const string &line);
    bool isAnomaly(double currentTemp, double previousTemp);
    double calculateMean(const unordered_map<Hour, vector<double>, HourHash>& temperatures);
    double calculateStdDev(const unordered_map<Hour, vector<double>, HourHash>& temperatures, double mean);
    void evaluateMonthlyTemperatures(const TimeKey& key, const unordered_map<Hour, vector<double>, HourHash> temperatures);
    bool isCoolingMonth(int month);
    bool isHeatingMonth(int month);
};

#endif // TEMPERATURE_ANALYSIS_PARALLEL_H
