#include "TemperatureAnalysisParallel.h"
#include <mpi.h>

using namespace std;

TemperatureAnalysisParallel::TemperatureAnalysisParallel(const string &filename) : inputFile(filename) {}

// Set the months designated for heating
void TemperatureAnalysisParallel::setHeatingMonths(const vector<int> &months)
{
    heatingMonths = months;
}

// Set the months designated for cooling
void TemperatureAnalysisParallel::setCoolingMonths(const vector<int> &months)
{
    coolingMonths = months;
}

struct TemperatureData {
    int month, day, year, hour, minute, second;
    float temperature;

    // Serialization helper for MPI
    static constexpr int dataSize = 7; // Number of fields in the struct
    void toArray(double *array) const {
        array[0] = month; array[1] = day; array[2] = year;
        array[3] = hour; array[4] = minute; array[5] = second;
        array[6] = temperature;
    }
    void fromArray(const double *array) {
        month = array[0]; day = array[1]; year = array[2];
        hour = array[3]; minute = array[4]; second = array[5];
        temperature = array[6];
    }
};

// Constants for task roles
enum TaskRole { READER = 0, PARSER = 1, DETECTOR = 2, WRITER = 3 };

int startPipeline(const string &outputFile) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 4) {
        if (rank == 0) {
            cerr << "This program requires exactly 4 MPI processes.\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    switch (rank) {
        case READER:
            fileReader(inputFile);
            break;
        case PARSER:
            parser();
            break;
        case DETECTOR:
            anomalyDetector();
            break;
        case WRITER:
            fileWriter(outputFile);
            break;
    }

    MPI_Finalize();
    return 0;
}

void fileReader(const string &filename) {
    if (!inputFile.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    string line;
    while (getline(inputFile, line)) {
        // Send each line to the parser
        MPI_Send(line.c_str(), line.size() + 1, MPI_CHAR, PARSER, 0, MPI_COMM_WORLD);
    }

    // Send a termination signal (empty line)
    MPI_Send("", 1, MPI_CHAR, PARSER, 0, MPI_COMM_WORLD);
    inputFile.close();
}

void parser() {
    while (true) {
        char buffer[1024];
        MPI_Recv(buffer, 1024, MPI_CHAR, READER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Check for termination signal
        if (strlen(buffer) == 0) {
            // Send termination signal to detector
            double sentinel[TemperatureData::dataSize] = {0};
            MPI_Send(sentinel, TemperatureData::dataSize, MPI_DOUBLE, DETECTOR, 0, MPI_COMM_WORLD);
            break;
        }

        // Parse the line into TemperatureData
        string line(buffer);
        TemperatureData data = parseLine(line);

        // Serialize and send to the detector
        double serializedData[TemperatureData::dataSize];
        data.toArray(serializedData);
        MPI_Send(serializedData, TemperatureData::dataSize, MPI_DOUBLE, DETECTOR, 0, MPI_COMM_WORLD);
    }
}

void anomalyDetector() {
    unordered_map<int, vector<double>> hourlyTemps; // Map for temperatures by hour
    double lastTemp = NAN;

    while (true) {
        double buffer[TemperatureData::dataSize];
        MPI_Recv(buffer, TemperatureData::dataSize, MPI_DOUBLE, PARSER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Check for termination signal
        if (buffer[0] == 0) {
            // Send termination signal to writer
            double sentinel[TemperatureData::dataSize] = {0};
            MPI_Send(sentinel, TemperatureData::dataSize, MPI_DOUBLE, WRITER, 0, MPI_COMM_WORLD);
            break;
        }

        // Deserialize the received data
        TemperatureData data;
        data.fromArray(buffer);

        // Check for anomaly
        if (!isnan(lastTemp) && fabs(data.temperature - lastTemp) > 2.0) {
            continue; // Skip anomaly
        }
        lastTemp = data.temperature;

        // Add temperature to hourly map
        int hourKey = data.hour;
        hourlyTemps[hourKey].push_back(data.temperature);

        // Calculate mean and stddev
        double mean = calculateMean(hourlyTemps);
        double stddev = calculateStdDev(hourlyTemps, mean);

        // Send temperature data to writer if it exceeds thresholds
        if (data.temperature > mean + stddev || data.temperature < mean - stddev) {
            double serializedData[TemperatureData::dataSize];
            data.toArray(serializedData);
            MPI_Send(serializedData, TemperatureData::dataSize, MPI_DOUBLE, WRITER, 0, MPI_COMM_WORLD);
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
        MPI_Recv(buffer, TemperatureData::dataSize, MPI_DOUBLE, DETECTOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Check for termination signal
        if (buffer[0] == 0) {
            break;
        }

        // Deserialize and write to the file
        TemperatureData data;
        data.fromArray(buffer);

        outFile << "Issue detected: " << data.month << "/" << data.day << "/" << data.year
                << " Hour: " << data.hour << " Temp: " << data.temperature << endl;
    }

    outFile.close();
}

// Helper function to parse a line of data
TemperatureData TemperatureAnalysisParallel::parseLine(const string &line)
{
    stringstream ss(line);
    char delim{};

    int month, day, year, hour, minute, second;
    float temperature;

    if (line.empty())
    {
        return TemperatureData(INT_MAX, 0, 0, 0, 0, 0, 0.0); // Return an invalid TemperatureData
    }

    // Parse the line
    ss >> month >> delim >> day >> delim >> year >> hour >> delim >> minute >> delim >> second >> temperature;

    // Return structured data
    return TemperatureData(month, day, year, hour, minute, second, temperature);
}

// Helper function to calculate mean temperature
double TemperatureAnalysisParallel::calculateMean(const unordered_map<Hour, vector<double>> &temperatures)
{
    double total = 0;
    int count = 0;

    for (const auto &hourEntry : temperatures)
    {
        for (double temp : hourEntry.second)
        {
            total += temp;
            count++;
        }
    }

    return (count > 0) ? (total / count) : 0;
}

// Helper function to calculate standard deviation
double TemperatureAnalysisParallel::calculateStdDev(const unordered_map<Hour, vector<double>> &temperatures, double mean)
{
    double sumSquaredDiffs = 0;
    int count = 0;

    for (const auto &hourEntry : temperatures)
    {
        for (double temp : hourEntry.second)
        {
            sumSquaredDiffs += (temp - mean) * (temp - mean);
            count++;
        }
    }

    return (count > 1) ? sqrt(sumSquaredDiffs / (count - 1)) : 0; // Using sample std dev
}

// Helper function to determine if temperature is an anomaly
bool TemperatureAnalysisParallel::isAnomaly(double current, double previous)
{
    return fabs(current - previous) > 2.0; // Custom threshold for anomalies
}

// Check if month is designated for heating
bool TemperatureAnalysisParallel::isHeatingMonth(int month)
{
    return find(heatingMonths.begin(), heatingMonths.end(), month) != heatingMonths.end();
}

// Check if month is designated for cooling
bool TemperatureAnalysisParallel::isCoolingMonth(int month)
{
    return find(coolingMonths.begin(), coolingMonths.end(), month) != coolingMonths.end();
}