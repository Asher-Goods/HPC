#ifndef TEMPERATURE_ANALYSIS_H
#define TEMPERATURE_ANALYSIS_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <sstream>

using namespace std;

// Public structure to hold temperature data
struct TemperatureData {
    string date;
    string time;
    double temperature;

    TemperatureData(const string& d, const string& t, double temp);
};

// TemperatureAnalysis class to encapsulate functionality
class TemperatureAnalysis {
public:
    // Constructor
    TemperatureAnalysis();

    // Destructor (to close file if necessary)
    ~TemperatureAnalysis();

    // Public interface to initialize the file
    bool initializeFile(const string& filename);

    // Public function to process the temperature data
    void processTemperatureData();

private:
    // Private helper methods
    TemperatureData parseLine(const string& line);
    bool isAnomaly(double currentTemp, double previousTemp);
    string getHourlyTimestamp(const string& date, const string& time);
    double calculateMean(const vector<double>& temps);
    double calculateStdDev(const vector<double>& temps, double mean);
    ifstream inputFile;
};

#endif  // TEMPERATURE_ANALYSIS_H
