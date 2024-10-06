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
    while(getline(inputFile, line)){
        // read in a line and parse it, storing it temporarily to "data"
        TemperatureData data = parseLine(line);
        if (data.hour == INT_MAX) {
            continue;
        }
        // check if the bucket has been created, if not, initialize it
        if (hourlyData.find(data.hour) == hourlyData.end()) {
            hourlyData[hourTimeStamp] = HourlyData();
        }
        // determine previous temperature within bucket
        double previousTemp;
        if (hourlyData[hourTimeStamp].count > 0) {
            previousTemp = hourlyData[hourTimeStamp].sum / hourlyData[hourTimeStamp].count;
        } else {
            previousTemp = data.temperature;
        }
        // check for existence of other items in the bucket and anomalies
        if (hourlyData[hourTimeStamp].count > 0 && isAnomaly(data.temperature, previousTemp)) {
            continue; // return to the top of the while loop and parse a new line
        } 
        // update sum and count for hourly bucket
        hourlyData[hourTimeStamp].sum += data.temperature;
        hourlyData[hourTimeStamp].count++;
    }
}

void TemperatureAnalysis::generateReport(const std::string& reportName) {
    std::ofstream reportFile(reportName);
    if (!reportFile.is_open()) {
        std::cerr << "Error opening report file!" << std::endl;
        return;
    }

    for (int month = 1; month <= 12; ++month) {
        double sumMeans = 0.0;
        int countMeans = 0;
        std::vector<double> monthlyMeans;  // Store means for variance calculation

        // Calculate the mean temperature for each hour in the current month
        for (const auto& [hourTimeStamp, entry] : hourlyData) {
            int tempMonth = getMonthFromTimeStamp(hourTimeStamp);
            if (tempMonth == month) {
                double hourlyMean = entry.sum / entry.count;  // Calculate hourly mean
                monthlyMeans.push_back(hourlyMean);           // Store it for later std deviation
                sumMeans += hourlyMean;
                ++countMeans;
            }
        }

        // Skip empty months
        if (countMeans == 0) {
            reportFile << "Month: " << month << " - No data available\n";
            continue;
        }

        // Calculate the mean temperature for the month
        double monthlyMean = sumMeans / countMeans;

        // Calculate the standard deviation for the month
        double varianceSum = 0.0;
        for (double mean : monthlyMeans) {
            varianceSum += std::pow(mean - monthlyMean, 2);
        }
        double stddev = std::sqrt(varianceSum / countMeans);

        // Write the report entry for this month
        reportFile << "Month: " << month << " | Mean Temperature: " << monthlyMean
                   << " | Standard Deviation: " << stddev;
        if (abs(stddev) > 1) {
            reportFile << "FAIL\n";
        } else {
            reportFile << "pass\n";
        }
    }

    reportFile.close();
    printf("Report generated: %s\n", reportName);
}

int TemperatureAnalysis::getMonthFromTimeStamp(const string &timestamp) {
    string monthStr = timestamp.substr(0,2);
    printf("%s\n",monthStr);
    return stoi(monthStr);
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

    printf("Date = %d/%d/%d\t Time = %d:%d:%d\t Temp = %f\n", month, day, year, hour, minute, second, temperature);
    return TemperatureData(month, day, year, hour, minute, second, temperature);
}

// Determine if new data point is an anomaly (more than two degrees away from the previous temperature)
bool TemperatureAnalysis::isAnomaly(double currentTemp, double previousTemp) {
    return std::abs(currentTemp - previousTemp) > 2.0;
}
