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
#include "TemperatureAnalysisMPI.h"

using namespace std;

// Parse a line of input
TemperatureData TemperatureAnalysisMPI::parseLine(const string &line) {
    stringstream ss(line);
    char delim;
    int month, day, year, hour, minute, second;
    double temperature;

    if (line.empty())
        return {-1, 0, 0, 0, 0, 0, 0.0}; // Invalid data

    ss >> month >> delim >> day >> delim >> year >> hour >> delim >> minute >> delim >> second >> temperature;
    return {month, day, year, hour, minute, second, temperature};
}

// File reader stage
void TemperatureAnalysisMPI::fileReader(const string &filename) {
    ifstream inputFile(filename);
    string line;
    vector<string> batch;

    while (getline(inputFile, line)) {
        batch.push_back(line);
        if (batch.size() == BATCH_SIZE) {
            // Concatenate the batch of lines into one large buffer
            int totalSize = 0;
            for (const auto &line : batch) {
                totalSize += line.size() + 1; // +1 for the null terminator
            }

            // Create a buffer to hold all lines in the batch
            char *buffer = new char[totalSize];
            int offset = 0;

            // Copy each line into the buffer
            for (const auto &line : batch) {
                memcpy(buffer + offset, line.c_str(), line.size() + 1);
                offset += line.size() + 1;
            }

            // Send the total size of the buffer and the buffer itself
            MPI_Send(&totalSize, 1, MPI_INT, PARSER, 0, MPI_COMM_WORLD);
            MPI_Send(buffer, totalSize, MPI_CHAR, PARSER, 0, MPI_COMM_WORLD);

            // Clean up
            delete[] buffer;

            batch.clear();
        }
    }

    // Send any remaining lines
    if (!batch.empty()) {
        int totalSize = 0;
        for (const auto &line : batch) {
            totalSize += line.size() + 1;
        }

        char *buffer = new char[totalSize];
        int offset = 0;
        for (const auto &line : batch) {
            memcpy(buffer + offset, line.c_str(), line.size() + 1);
            offset += line.size() + 1;
        }

        MPI_Send(&totalSize, 1, MPI_INT, PARSER, 0, MPI_COMM_WORLD);
        MPI_Send(buffer, totalSize, MPI_CHAR, PARSER, 0, MPI_COMM_WORLD);

        delete[] buffer;
    }

    // Signal end of file
    int sentinel = -1;
    MPI_Send(&sentinel, 1, MPI_INT, PARSER, 0, MPI_COMM_WORLD);
    printf("read terminate\n");
}


// Parser stage
void TemperatureAnalysisMPI::parser() {
    while (true) {
        // Receive the total size of the batch
        int totalSize;
        MPI_Recv(&totalSize, 1, MPI_INT, FILEREADER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (totalSize == -1) break; // End signal

        // Allocate a buffer to hold all lines in the batch
        char *buffer = new char[totalSize];

        // Receive the entire batch of lines
        MPI_Recv(buffer, totalSize, MPI_CHAR, FILEREADER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vector<TemperatureData> parsedData;

        // Parse the received buffer into individual lines
        int offset = 0;
        while (offset < totalSize) {
            // Find the end of the current line (null terminator)
            string line(buffer + offset);
            TemperatureData temp_line = parseLine(line);
            if(temp_line.month != -1){
                parsedData.push_back(parseLine(line));
            }
            
            // Move the offset to the next line
            offset += line.size() + 1; // +1 for the null terminator
        }

        // Send parsed data to anomaly detector
        int batchSize = parsedData.size();
        MPI_Send(&batchSize, 1, MPI_INT, ANOMALYDETECTOR, 0, MPI_COMM_WORLD);
        MPI_Send(parsedData.data(), batchSize * sizeof(TemperatureData), MPI_BYTE, ANOMALYDETECTOR, 0, MPI_COMM_WORLD);

        // Clean up
        delete[] buffer;
    }

    // Signal end of parsing
    int sentinel = -1;
    MPI_Send(&sentinel, 1, MPI_INT, ANOMALYDETECTOR, 0, MPI_COMM_WORLD);
    printf("parse terminate\n");
}


// Anomaly detector stage
void TemperatureAnalysisMPI::anomalyDetector()
{
    queue<TemperatureData> parseQueue;
    vector<TemperatureData> sendBuffer;

    // Initialize currentMonth and previousTemp
    int currentMonth = -1;  // Use -1 as an uninitialized value
    double previousTemp = -1;  // Use -1 as an uninitialized value

    while (true)
    {
        // Receive batch size
        int batchSize;
        MPI_Recv(&batchSize, 1, MPI_INT, PARSER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (batchSize == -1) {
            // send remaining monthly data from sendbuffer
            int totalDataSize = sendBuffer.size();  // Update the size of the sendBuffer
            if(totalDataSize > 0) {
                MPI_Send(&totalDataSize, 1, MPI_INT, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD);
                // Send the data in sendBuffer (ensure we are sending the right number of elements)
                MPI_Send(sendBuffer.data(), totalDataSize * sizeof(TemperatureData), MPI_BYTE, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD);
            }
            break; // End signal
        }

        // Create a vector to hold the received data batch
        vector<TemperatureData> dataBatch(batchSize); // Create a batch of the correct size

        // Receive the entire batch of TemperatureData
        MPI_Recv(dataBatch.data(), batchSize * sizeof(TemperatureData), MPI_BYTE, PARSER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Push the newly received data to the parseQueue
        for (const auto& data : dataBatch) {
            parseQueue.push(data);
        }

        // Process the data in the queue
        while (!parseQueue.empty())
        {
            auto data = parseQueue.front();
            parseQueue.pop();

            // Handle edge case for first temperature and month
            if (previousTemp == -1) { // Assume -1 means uninitialized
                previousTemp = data.temperature; // Initialize with first data temperature
            }

            if (currentMonth == -1) { // Assume -1 means uninitialized
                currentMonth = data.month; // Initialize with first data month
            }

            // Check if current month is same as previous month
            // If it isn't, send data and set current month and previous value to equal current value, clear sendbuffer
            if (data.month != currentMonth) {
                int totalDataSize = sendBuffer.size();  // Update the size of the sendBuffer
                if (totalDataSize > 0) {
                    MPI_Send(&totalDataSize, 1, MPI_INT, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD);
                    MPI_Send(sendBuffer.data(), totalDataSize * sizeof(TemperatureData), MPI_BYTE, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD);
                }
                // Update current month and reset previousTemp
                currentMonth = data.month;
                previousTemp = data.temperature;  // Initialize for the new month

                // Clear sendBuffer after sending
                sendBuffer.clear();
            }
            else {
                // Detect anomaly
                if (isAnomaly(data.temperature, previousTemp)) {
                    continue; // Skip this entry as it's an anomaly
                }

                // Add to sendBuffer if it's not an anomaly
                sendBuffer.push_back(data);

                // Update previous temperature
                previousTemp = data.temperature;
            }

        }
    }

    // Send sentinel value to indicate the end of processing
    int endSignal = -1;
    MPI_Send(&endSignal, 1, MPI_INT, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD); // Send end signal to terminate the next stage
    printf("anomaly terminate\n");
}




void TemperatureAnalysisMPI::evaluateMonthlyTemperatures(void)
{
    // Local variable to track processed hours
    set<int> processedHours;
    vector<TemperatureData> sendBuffer;

    while (true) {
        // Receive the batch size of the data
        int batchSize;
        MPI_Recv(&batchSize, 1, MPI_INT, ANOMALYDETECTOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (batchSize <= 0) {
            cerr << "Received invalid batchSize: " << batchSize << endl;
            break; // Handle gracefully or exit
        }

        // Receive chunk of monthly temperature data (clean of anomalies)
        vector<TemperatureData> data(batchSize);
        MPI_Recv(data.data(), batchSize * sizeof(TemperatureData), MPI_BYTE, ANOMALYDETECTOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Calculate mean and standard deviation for the current month's data
        double mean = calculateMean(data);
        double stddev = calculateStdDev(data, mean);
        printf("Month: %d\t Mean: %f\t STDV: %f\n", data[0].month, mean, stddev);
        // Process temperatures for heating/cooling issues

        int hourProcessed = -1;
        int currentDay = -1;

        for (const TemperatureData& entry : data)
        {

            if(isCoolingMonth(entry.month)) {
                if ((entry.temperature > mean + stddev) && (hourProcessed != entry.hour) && (currentDay != entry.day) ) {
                sendBuffer.push_back(entry);
                hourProcessed = entry.hour;
                currentDay = entry.day;
            }
            }
            
            else if(isHeatingMonth(entry.month)) {
                if ((entry.temperature < mean - stddev) && (hourProcessed != entry.hour) && (currentDay != entry.day) ) {
                sendBuffer.push_back(entry);
                hourProcessed = entry.hour;
                currentDay = entry.day;
            }
            }

        }

        // Send buffer via MPI to FileWriter
        if(!sendBuffer.empty()){
            int totalDataSize = sendBuffer.size();  // Get the size of the sendBuffer
            MPI_Send(&totalDataSize, 1, MPI_INT, FILEWRITER, 0, MPI_COMM_WORLD);  // Send the size to FileWriter
            if (totalDataSize > 0) {
                MPI_Send(sendBuffer.data(), totalDataSize * sizeof(TemperatureData), MPI_BYTE, FILEWRITER, 0, MPI_COMM_WORLD);  // Send data to FileWriter
            }
        }
        processedHours.clear();

        // Clear sendBuffer after sending
        sendBuffer.clear();
        data.clear();
    }

    int endSignal = -1;
    MPI_Send(&endSignal, 1, MPI_INT, FILEWRITER, 0, MPI_COMM_WORLD); // Send end signal to terminate the next stage
    printf("eval terminate\n");
}


// File writer stage
void TemperatureAnalysisMPI::fileWriter(const string &outputFile)
{
    ofstream outFile(outputFile);
    
    while (true) {
        // Receive the size of the current batch of data
        int batchSize;
        MPI_Recv(&batchSize, 1, MPI_INT, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        if (batchSize == -1) {
            // End signal received, exit the loop
            break;
        }

        // Create a vector to hold the received batch of TemperatureData
        vector<TemperatureData> dataBatch(batchSize);
        
        // Receive the batch of data
        MPI_Recv(dataBatch.data(), batchSize * sizeof(TemperatureData), MPI_BYTE, EVALUATETEMPERATURES, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Process each entry in the received data batch
        for (const auto& entry : dataBatch) {
            // Format and write the entry to the output file
            if (isHeatingMonth(entry.month)) {
                outFile << "Heating issue detected: "
                        << entry.month << "/" << entry.day << "/" << entry.year
                        << " At Hour: " << entry.hour << " | Temp: " << entry.temperature << endl;
            } 
            else if (isCoolingMonth(entry.month)) {
                outFile << "Cooling issue detected: "
                        << entry.month << "/" << entry.day << "/" << entry.year
                        << " At Hour: " << entry.hour << " | Temp: " << entry.temperature << endl;
            }
        }
        
        // Flush the output to ensure it's written immediately
        outFile.flush();
    }

    // Close the output file
    outFile.close();
    printf("write file terminate\n");
}

// Helper function to determine if temperature is an anomaly
bool TemperatureAnalysisMPI::isAnomaly(double current, double previous)
{
    return fabs(current - previous) > 2.0; // Custom threshold for anomalies
}

// Function to calculate the mean of the temperatures
double TemperatureAnalysisMPI::calculateMean(vector<TemperatureData> &temperatures) {
    if (temperatures.empty()) {
        return 0.0; // Handle empty vector case
    }

    double sum = 0.0;
    for (const auto &data : temperatures) {
        sum += data.temperature;
    }

    return sum / temperatures.size();
}

// Function to calculate the standard deviation of the temperatures
double TemperatureAnalysisMPI::calculateStdDev(vector<TemperatureData> &temperatures, double mean) {
    if (temperatures.empty()) {
        return 0.0; // Handle empty vector case
    }

    double sumSquaredDiffs = 0.0;
    for (const auto &data : temperatures) {
        double diff = data.temperature - mean;
        sumSquaredDiffs += diff * diff;
    }

    return sqrt(sumSquaredDiffs / temperatures.size());
}

// Check if month is designated for heating
bool TemperatureAnalysisMPI::isHeatingMonth(int month)
{
    return find(heatingMonths.begin(), heatingMonths.end(), month) != heatingMonths.end();
}

// Check if month is designated for cooling
bool TemperatureAnalysisMPI::isCoolingMonth(int month)
{
    return find(coolingMonths.begin(), coolingMonths.end(), month) != coolingMonths.end();
}

// Set the months designated for heating
void TemperatureAnalysisMPI::setHeatingMonths(const vector<int> &months)
{
    heatingMonths = months;
}

// Set the months designated for cooling
void TemperatureAnalysisMPI::setCoolingMonths(const vector<int> &months)
{
    coolingMonths = months;
}