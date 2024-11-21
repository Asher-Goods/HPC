#ifndef TEMPERATUREANALYSISMPI_H
#define TEMPERATUREANALYSISMPI_H

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <set>

using namespace std;

// TemperatureData structure
struct TemperatureData {
    int month, day, year, hour, minute, second;
    double temperature;

    bool isValid() const { return month != -1; }
};

// MPI Pipeline roles
enum Role { FILEREADER = 0, PARSER = 1, ANOMALYDETECTOR = 2, EVALUATETEMPERATURES = 3, FILEWRITER = 4 };

// Batch size for data transfer
constexpr int BATCH_SIZE = 100;



class TemperatureAnalysisMPI {
public:
    // Parse a line of input
    TemperatureData parseLine(const string &line);
    // File reader stage
    void fileReader(const string &filename);
    // Parser stage
    void parser();
    // Anomaly detection stage
    void anomalyDetector();
    // evaluate stage
    void evaluateMonthlyTemperatures();
    // anomaly detector helper
    bool isAnomaly(double current, double previous);
    // File writer stage
    void fileWriter(const string &outputFile);

    // Helper function declarations
    bool isHeatingMonth(int month);
    bool isCoolingMonth(int month);
    void setHeatingMonths(const vector<int>& months);
    void setCoolingMonths(const vector<int>& months);
    
    
    double calculateMean(vector<TemperatureData> &temperatures);
    double calculateStdDev(vector<TemperatureData> &temperatures, double mean);

private:
    vector<int> heatingMonths;
    vector<int> coolingMonths;
};




#endif // TEMPERATUREANALYSISMPI_H
