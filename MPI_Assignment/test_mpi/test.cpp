#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE 1000

// Task 1: Generate random numbers
void generate_random_numbers(int* numbers, int size) {
    for (int i = 0; i < size; i++) {
        numbers[i] = rand() % 100;  // Random numbers between 0 and 99
    }
}

// Task 2: Calculate sum of numbers
int calculate_sum(int* numbers, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += numbers[i];
    }
    return sum;
}

// Task 3: Calculate average
float calculate_average(int sum, int count) {
    return (float)sum / count;
}

// Task 4: Find maximum value
int find_maximum(int* numbers, int size) {
    int max = numbers[0];
    for (int i = 1; i < size; i++) {
        if (numbers[i] > max) {
            max = numbers[i];
        }
    }
    return max;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_size != 4) {
        if (world_rank == 0) {
            printf("This program requires exactly 4 processes.\n");
        }
        MPI_Finalize();
        return 1;
    }

    int numbers[ARRAY_SIZE];
    int sum, max;
    float avg;

    if (world_rank == 0) {
        // Task 1: Generate random numbers
        srand(time(NULL));
        generate_random_numbers(numbers, ARRAY_SIZE);
        printf("Process 0: Generated %d random numbers\n", ARRAY_SIZE);

        // Send numbers to other processes
        for (int i = 1; i < world_size; i++) {
            MPI_Send(numbers, ARRAY_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // Receive results from other processes
        MPI_Recv(&sum, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&avg, 1, MPI_FLOAT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&max, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("Sum: %d\n", sum);
        printf("Average: %.2f\n", avg);
        printf("Maximum: %d\n", max);
    }
    else {
        // Receive numbers from process 0
        MPI_Recv(numbers, ARRAY_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (world_rank == 1) {
            // Task 2: Calculate sum
            sum = calculate_sum(numbers, ARRAY_SIZE);
            printf("Process 1: Calculated sum\n");
            MPI_Send(&sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        else if (world_rank == 2) {
            // Task 3: Calculate average
            sum = calculate_sum(numbers, ARRAY_SIZE);
            avg = calculate_average(sum, ARRAY_SIZE);
            printf("Process 2: Calculated average\n");
            MPI_Send(&avg, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
        }
        else if (world_rank == 3) {
            // Task 4: Find maximum
            max = find_maximum(numbers, ARRAY_SIZE);
            printf("Process 3: Found maximum\n");
            MPI_Send(&max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}