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
    int current_hour = 0;
    double count = 0;
    double sum = 0;
    while(getline(inputFile, line)){
        // read in a line and parse it, storing it temporarily to "data"
        TemperatureData data = parseLine(line);
        if (data.hour == INT_MAX) {
            continue;
        }

        if (current_hour != data.hour && !dataset[data.month].empty()) {
            printf("is this ever executed?\n");
            dataset[data.month].push_back(sum/count);    
            current_hour = data.hour;
            count = 0;
            sum = 0;
        }

        // determine previous temperature within bucket
        double previousTemp;
        if (count > 0) {
            previousTemp = sum / count;
        } else {
            previousTemp = data.temperature;
        }
        // check for existence of other items in the bucket and anomalies
        if (count > 0 && isAnomaly(data.temperature, previousTemp)) {
            continue; // return to the top of the while loop and parse a new line
        } 

        // update sum and count for hourly bucket
        sum += data.temperature;
        count++;
    }
}

void TemperatureAnalysis::generateReport(const std::string& reportName) {
    std::ofstream reportFile(reportName);
    if (!reportFile.is_open()) {
        std::cerr << "Error opening report file!" << std::endl;
        return;
    }

    for (int month = 1; month <= 12; ++month) {  // Assuming `dataset` has 12 inner vectors, one for each month
        if (dataset[month].empty()) {
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
        reportFile << "Month: " << month + 1 << " | Mean Temperature: " << mean
                   << " | Standard Deviation: " << stddev;
        if (stddev > 1) {
            reportFile << " FAIL\n";
        } else {
            reportFile << " pass\n";
        }
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
