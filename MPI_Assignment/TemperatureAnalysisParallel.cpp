#include "TemperatureAnalysisParallel.h"
#include <mpi.h>

using namespace std;

TemperatureAnalysisParallel::TemperatureAnalysisParallel(const string &filename) : inputFile(filename) {}

void TemperatureAnalysisParallel::setHeatingMonths(const vector<int> &months) {
    heatingMonths = months;
}

void TemperatureAnalysisParallel::setCoolingMonths(const vector<int> &months) {
    coolingMonths = months;
}

double TemperatureAnalysisParallel::calculateMean(const unordered_map<int, vector<double>>& temperatures) {
    double total = 0;
    int count = 0;

    for (const auto& hourEntry : temperatures) {
        for (double temp : hourEntry.second) {
            total += temp;
            count++;
        }
    }
    return (count > 0) ? total / count : 0;
}

double TemperatureAnalysisParallel::calculateStdDev(const unordered_map<int, vector<double>>& temperatures, double mean) {
    double sumSquaredDiffs = 0;
    int count = 0;

    for (const auto& hourEntry : temperatures) {
        for (double temp : hourEntry.second) {
            sumSquaredDiffs += (temp - mean) * (temp - mean);
            count++;
        }
    }
    return (count > 1) ? sqrt(sumSquaredDiffs / (count - 1)) : 0;
}

TemperatureData TemperatureAnalysisParallel::parseLine(const string &line) {
    stringstream ss(line);
    char delim;
    int month, day, year, hour, minute, second;
    double temperature;

    if (line.empty()) {
        return TemperatureData(INT_MAX, 0, 0, 0, 0, 0, 0.0); // Return invalid TemperatureData
    }

    ss >> month >> delim >> day >> delim >> year >> hour >> delim >> minute >> delim >> second >> temperature;
    return TemperatureData(month, day, year, hour, minute, second, temperature);
}

bool TemperatureAnalysisParallel::isAnomaly(double current, double previous) {
    return fabs(current - previous) > 2.0;
}

bool TemperatureAnalysisParallel::isHeatingMonth(int month) {
    return find(heatingMonths.begin(), heatingMonths.end(), month) != heatingMonths.end();
}

bool TemperatureAnalysisParallel::isCoolingMonth(int month) {
    return find(coolingMonths.begin(), coolingMonths.end(), month) != coolingMonths.end();
}

// Main processing functions

void fileReader(const string &filename) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    string line;
    while (getline(inputFile, line)) {
        MPI_Send(line.c_str(), line.size() + 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
    }
    MPI_Send("", 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD); // Send termination signal
    inputFile.close();
}

void parser() {
    while (true) {
        char buffer[1024];
        MPI_Recv(buffer, 1024, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (strlen(buffer) == 0) {
            double sentinel[TemperatureData::dataSize] = {0};
            MPI_Send(sentinel, TemperatureData::dataSize, MPI_DOUBLE, 2, 0, MPI_COMM_WORLD);
            break;
        }

        string line(buffer);
        TemperatureData data = TemperatureAnalysisParallel::parseLine(line);

        double serializedData[TemperatureData::dataSize];
        data.toArray(serializedData);
        MPI_Send(serializedData, TemperatureData::dataSize, MPI_DOUBLE, 2, 0, MPI_COMM_WORLD);
    }
}

void anomalyDetector() {
    unordered_map<int, vector<double>> hourlyTemps;
    double lastTemp = NAN;

    while (true) {
        double buffer[TemperatureData::dataSize];
        MPI_Recv(buffer, TemperatureData::dataSize, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (buffer[0] == 0) {
            double sentinel[TemperatureData::dataSize] = {0};
            MPI_Send(sentinel, TemperatureData::dataSize, MPI_DOUBLE, 3, 0, MPI_COMM_WORLD);
            break;
        }

        TemperatureData data;
        data.fromArray(buffer);

        if (!isnan(lastTemp) && fabs(data.temperature - lastTemp) > 2.0) {
            continue; // Skip anomaly
        }
        lastTemp = data.temperature;

        int hourKey = data.hour;
        hourlyTemps[hourKey].push_back(data.temperature);

        double mean = calculateMean(hourlyTemps);
        double stddev = calculateStdDev(hourlyTemps, mean);

        if (data.temperature > mean + stddev || data.temperature < mean - stddev) {
            double serializedData[TemperatureData::dataSize];
            data.toArray(serializedData);
            MPI_Send(serializedData, TemperatureData::dataSize, MPI_DOUBLE, 3, 0, MPI_COMM_WORLD);
        }
    }
}

void fileWriter(const string &outputFile) {
    ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        cerr << "Failed to open output file: " << outputFile << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (true) {
        double buffer[TemperatureData::dataSize];
        MPI_Recv(buffer, TemperatureData::dataSize, MPI_DOUBLE, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (buffer[0] == 0) {
            break;
        }

        TemperatureData data;
        data.fromArray(buffer);
        outFile << "Issue detected: " << data.month << "/" << data.day << "/" << data.year
                << " Hour: " << data.hour << " Temp: " << data.temperature << endl;
    }

    outFile.close();
}
