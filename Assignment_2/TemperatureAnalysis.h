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

// TemperatureAnalysis class to encapsulate functionality
class TemperatureAnalysis
{
public:
    // Constructor
    TemperatureAnalysis();

    // Destructor (to close file if necessary)
    ~TemperatureAnalysis();

    /**
     * Used to initialize and open the file
     * @arg filename - name of data file
     * @retval True if the file exists, false otherwise
     */    
    bool initializeFile(const string &filename);

    /**
     * Data is parsed in from the log file containing temperature information. Each data point is
     * then synthesized to store the average value for each hour. Each hour is then bucketed into
     * the dataset field which is mapped by month.
     */
    void processTemperatureData();

    /**
     * An output file is opened which reports the mean temperature value of each month
     * and the respective standard deviation. This is achieved by looping through each
     * month bucket and performing algebraic operations.
     * @arg reportName - name of file to output report to
     */
    void generateReport(const std::string &reportName);

private:
    /**
     * Determines if the current temperature is an anomaly by comparing it to the previous temperature.
     * If there is a difference of two or more degrees, then the data point will be thrown out
     * @arg currentTemp - temperature which is being checked as an anomaly
     * @arg previousTemp - average temperature of the hourly bucket
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

    // private instantiation of input file field
    ifstream inputFile;
    map<int, vector<double> > dataset;
};

#endif // TEMPERATURE_ANALYSIS_H
