#include "TemperatureAnalysis.h"

using namespace std;


TemperatureAnalysis::TemperatureAnalysis() {}

TemperatureAnalysis::~TemperatureAnalysis() {
    if (inputFile.is_open()) {
        inputFile.close();
    }
}

// Used to initialize and open the file
bool TemperatureAnalysis::initializeFile(const string& filename) {
    inputFile.open(filename);
    return inputFile.is_open();
}

/**
 * Data is parsed and bucketed based on hours.
 */
void TemperatureAnalysis::processTemperatureData(void) {
    string line = "";
    int current_hour = -1;
    double count = 0;
    double sum = 0;
    int last_month = -1; // Variable to store the last month processed

    while (getline(inputFile, line)) {
        TemperatureData data = parseLine(line);
        if (data.hour == INT_MAX) {  // Skip invalid data lines
            continue;
        }

        // Insert the average temperature for the last hour if moving to a new hour
        if (current_hour != data.hour) {
            if (current_hour != -1 && count > 0 && last_month != -1) {
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
        if (count > 0 && isAnomaly(data.temperature, previousTemp)) {
            continue;
        }

        // Accumulate temperature
        sum += data.temperature;
        count++;
    }

    // Handle the last hour in the file
    if (count > 0 && last_month != -1) {
        dataset[last_month].push_back(sum / count);
    }
}


void TemperatureAnalysis::generateReport(const std::string& reportName) {
    std::ofstream reportFile(reportName);
    if (!reportFile.is_open()) {
        std::cerr << "Error opening report file!" << std::endl;
        return;
    }

    for (int month = 1; month <= 12; ++month) {
        if (dataset[month].empty()) {  // Check if there is data for this month
            reportFile << "Month: " << month << " - No data available\n";
            continue;
        }

        // Calculate the mean for the current month
        double sum = 0.0;
        for (double temp : dataset[month]) {
            sum += temp;
        }
        double mean = sum / dataset[month].size();

        // Calculate the standard deviation for the current month
        double varianceSum = 0.0;
        for (double temp : dataset[month]) {
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


// Parses a line of temperature data
TemperatureData TemperatureAnalysis::parseLine(const std::string& line) {
    stringstream ss(line);
    char delim{};

    int month, day, year, hour, minute, second;
    float temperature;


    if (line.empty()) {
        return TemperatureData(INT_MAX,0,0,0,0,0,0.0);  // Return an empty object for an empty line
    }

    // store date into temporary fields
    ss >> month >> delim >> day >> delim >> year; 
    // store time into temporary fields
    ss >> hour >> delim >> minute >> delim >> second;
    // store temperature
    ss >> temperature >> delim;

    // printf("Date = %d/%d/%d\t Time = %d:%d:%d\t Temp = %f\n", month, day, year, hour, minute, second, temperature);
    return TemperatureData(month, day, year, hour, minute, second, temperature);
}

// Determine if new data point is an anomaly (more than two degrees away from the previous temperature)
bool TemperatureAnalysis::isAnomaly(double currentTemp, double previousTemp) {
    return std::abs(currentTemp - previousTemp) > 2.0;
}
