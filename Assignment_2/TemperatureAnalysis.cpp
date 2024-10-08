#include "TemperatureAnalysis.h"

using namespace std;

TemperatureAnalysis::TemperatureAnalysis() {}

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
bool TemperatureAnalysis::initializeFile(const string &filename)
{
    inputFile.open(filename.c_str()); // Use c_str() to convert string to const char*
    return inputFile.is_open();
}

/**
 * Data is parsed in from the log file containing temperature information. Each data point is
 * then synthesized to store the average value for each hour. Each hour is then bucketed into
 * the dataset field which is mapped by month.
 */
void TemperatureAnalysis::processTemperatureData(void)
{
    string line = "";
    int current_hour = -1;
    double count = 0;
    double sum = 0;
    int last_month = -1; // Variable to store the last month processed

    while (getline(inputFile, line))
    {
        TemperatureData data = parseLine(line);
        if (data.hour == INT_MAX)
        { // Skip invalid data lines
            continue;
        }

        // Insert the average temperature for the last hour if moving to a new hour
        if (current_hour != data.hour)
        {
            if (current_hour != -1 && count > 0 && last_month != -1)
            {
                dataset[last_month].push_back(sum / count);
            }
            current_hour = data.hour;
            count = 0;
            sum = 0;
        }

        // Update the last month processed
        last_month = data.month;

        // Check for anomalies
        double previousTemp = (count > 0) ? (sum / count) : data.temperature;
        if (count > 0 && isAnomaly(data.temperature, previousTemp))
        {
            continue;
        }

        // Accumulate temperature
        sum += data.temperature;
        count++;
    }

    // Handle the last hour in the file
    if (count > 0 && last_month != -1)
    {
        dataset[last_month].push_back(sum / count);
    }
}

/**
 * An output file is opened which reports the mean temperature value of each month
 * and the respective standard deviation. This is achieved by looping through each
 * month bucket and performing algebraic operations.
 * @arg reportName - name of file to output report to
 */
void TemperatureAnalysis::generateReport(const std::string &reportName)
{
    std::ofstream reportFile(reportName.c_str()); // Use c_str() to convert string to const char*
    if (!reportFile.is_open())
    {
        std::cerr << "Error opening report file!" << std::endl;
        return;
    }

    for (int month = 1; month <= 12; ++month)
    {
        if (dataset[month].empty())
        { // Check if there is data for this month
            reportFile << "Month: " << month << " - No data available\n";
            continue;
        }

        // Calculate the mean for the current month
        double sum = 0.0;
        for (double temp : dataset[month])
        {
            sum += temp;
        }
        double mean = sum / dataset[month].size();

        // Calculate the standard deviation for the current month
        double varianceSum = 0.0;
        for (double temp : dataset[month])
        {
            varianceSum += std::pow(temp - mean, 2);
        }
        double stddev = std::sqrt(varianceSum / dataset[month].size());

        // Write report for this month
        reportFile << "Month: " << month << " | Mean Temperature: " << mean
                   << " | Standard Deviation: " << stddev << endl;
    }

    reportFile.close();
    std::cout << "Report generated: " << reportName << std::endl;
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
