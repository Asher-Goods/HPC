#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <numeric>

double calculateMean(const std::vector<double>& temperatures) {
    double sum = std::accumulate(temperatures.begin(), temperatures.end(), 0.0);
    return sum / temperatures.size();
}

double calculateStdDev(const std::vector<double>& temperatures, double mean) {
    if (temperatures.size() <= 1) {
        return 0.0; // Return 0 or appropriate value if there's insufficient data
    }
    double accum = 0.0;
    for (const auto& temp : temperatures) {
        accum += (temp - mean) * (temp - mean);
    }
    return std::sqrt(accum / (temperatures.size() - 1)); // Use N - 1 for sample std deviation
}

bool isAnomaly(double prev, double curr) {
    return std::abs(prev - curr) >= 2.0;
}

int main() {
    std::ifstream inputFile("testInput.txt"); // Change this to your file name
    if (!inputFile.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
        return 1;
    }

    std::vector<double> temperatures;
    std::string line;
    double temperature;
    int count = 0;

    // Read first line:
    std::getline(inputFile, line);
    std::istringstream ss(line);
    std::string date, time;
    ss >> date >> time >> temperature;
    temperatures.push_back(temperature); // Add the first temperature
    double previousTemp = temperature; // Initialize previousTemp

    // Read temperature data from the file
    while (std::getline(inputFile, line)) {
        std::istringstream ss(line);
        std::string date, time;

        // Extract date, time, and temperature
        if (ss >> date >> time >> temperature && !isAnomaly(previousTemp, temperature)) {
            temperatures.push_back(temperature);
            previousTemp = temperature; // Update previousTemp
            count++;
        }   
    }

    inputFile.close();

    // Check if we have enough data
    if (temperatures.empty()) {
        std::cerr << "No temperature data found!" << std::endl;
        return 1;
    }

    // Calculate mean and standard deviation
    double mean = calculateMean(temperatures);
    double stddev = calculateStdDev(temperatures, mean);

    // Output results
    std::cout << "Mean temperature: " << mean << std::endl;
    std::cout << "Standard deviation: " << stddev << std::endl;
    std::cout << "Count: " << count << std::endl;

    return 0;
}
