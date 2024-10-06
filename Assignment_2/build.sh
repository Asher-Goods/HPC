#!/bin/bash

# Define build directory
BUILD_DIR="build"

# Remove any existing cache or CMake build files
rm -rf ${BUILD_DIR}/*

# Create a build directory if it doesn't exist
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Run CMake to configure the project
cmake .. || { echo "CMake configuration failed. Check your setup."; exit 1; }

# Build the project
cmake --build . --config Release || { echo "Build failed."; exit 1; }

echo "Build completed successfully."
