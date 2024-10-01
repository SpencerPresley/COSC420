// Spencer Presley
// This program uses non-blocking sends and receives to calculate the sum of an array of numbers
// It distributes the work among process equally by calculating the number of elements each process will handle and accounting for any remainder that may occur causing an uneven distribution
// The program then calculates the local sum of the elements in the local array and sends the result to the root process
// The root process receives the sums from the other processes and adds them to the total sum
// Finally, the program prints the total sum
// There are also prints to show what is happening to make it more clear, rather than only relying on the reading of the logic

#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>

// Function to calculate start and count for each process
void calculate_start_and_count(int rank, int size, int n, int *start, int *count) {
    // Elements_per_process is the base number of elements each process will handle
    // It is calculated by dividing the total number of elements by the number of processes
    int elements_per_process = n / size;
    // Remainder is the number of elements that need to be distributed among the first 'remainder' processes
    // It is calculated by getting the remainder of the total number of elements divided by the number of processes
    int remainder = n % size;
    // The start is calculated by multiplying the rank by the number of elements each process will handle and adding the rank if the rank is less than the remainder
    // This ensures that the first 'remainder' processes will have 1 more element than the others
    // If the rank is greater than the remainder, then the start is the base number of elements each process will handle times the remainder plus the rank otherwise it is the base number of elements each process will handle times the rank plus the remainder
    *start = rank * elements_per_process + (rank < remainder ? rank : remainder);
    // The count is the number of elements each process will handle, which is the base number of elements each process will handle plus 1 if the rank is less than the remainder
    // This ensures that the first 'remainder' processes will have 1 more element than the others
    // If the rank is greater than the remainder, then the count is just the base number of elements each process will handle otherwise it is the base number of elements each process will handle plus 1
    *count = elements_per_process + (rank < remainder ? 1 : 0);
}

int main(int argc, char **argv) {
    // Ensure the correct number of arguments, it should be only 2, the program name and the number n in which to sum from 1 to n
    if (argc != 2) {
        // If the number of arguments is not 2, print an error message and exit with code 1
        fprintf(stderr, "Usage: %s <number_of_elements>\n", argv[0]);
        return 1;
    }

    // Convert the argument to an integer
    int n = atoi(argv[1]);

    /*
    Initialize the MPI environment
    - Get the rank of the process and the total number of processes
    - Get the number of elements per process and the remainder
    */
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /*
    Calculate the number of elements per process and the remainder
    - The number of elements per process is the total number of elements divided by the number of processes
    - The remainder is the total number of elements modulo the number of processes
        - This does the following:
            - n / size gives the base number of elements each process will handle
            - n % size gives the remainder (the extra elements that need to be distributed)
            - The remainder elements are distributed among the first 'remainder' processes
        - Example: 
            - n = 100
            - size = 8
            - elements_per_process = 100 / 8 = 12
            - remainder = 100 % 8 = 4
            - Distribution:
                - First 4 processes will have 13 elements (12 + 1)
                - Last 4 processes will have 12 elements
        - This approach ensures an even distribution with at most 1 element difference between processes
    */
    int local_start, local_elements;
    calculate_start_and_count(rank, size, n, &local_start, &local_elements);
    int local_end = local_start + local_elements - 1;

    // Initialize the array, requests, and request count
    int *array = NULL;
    MPI_Request *requests = NULL;
    int req_count = 0;

    // If the current process is the root process (rank 0)
    if (rank == 0) {
        // Allocate memory for the array
        array = (int *)malloc(n * sizeof(int));
        // Fill the array with sequential numbers from 1 to n with an offset of 1
        for (int i = 0; i < n; i++) {
            array[i] = i + 1;
        }

        // Allocate memory for the requests
        requests = (MPI_Request *)malloc(3 * (size - 1) * sizeof(MPI_Request));

        // Update the sending logic for rank 0
        for (int i = 1; i < size; i++) {
            int start, count;
            calculate_start_and_count(i, size, n, &start, &count);
            int end = start + count - 1;
            
            printf("Process 0: Sending start=%d and end=%d to process %d\n", start, end, i);

            // Send the start and end indices to the current process in a non-blocking fashion
            MPI_Isend(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &requests[req_count++]);
            MPI_Isend(&end, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &requests[req_count++]);
            MPI_Isend(&array[start], count, MPI_INT, i, 2, MPI_COMM_WORLD, &requests[req_count++]);
        }
    } else {
        // Allocate memory for the requests
        requests = (MPI_Request *)malloc(3 * sizeof(MPI_Request));

        // Receive the start and end indices from the root process in a non-blocking fashion
        MPI_Irecv(&local_start, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &requests[req_count++]);
        MPI_Irecv(&local_end, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &requests[req_count++]);
    }

    // Calculate the size of the local array
    int local_size = local_end - local_start + 1;
    // Allocate memory for the local array
    int *local_array = (int *)malloc(local_size * sizeof(int));

    if (rank != 0) {
        MPI_Irecv(local_array, local_size, MPI_INT, 0, 2, MPI_COMM_WORLD, &requests[req_count++]);
    }

    // Wait for all the requests to complete
    MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);
    // Free the memory allocated for the requests
    free(requests);

    printf("Process %d: Received local_start=%d and local_end=%d\n", rank, local_start, local_end);
    printf("Process %d: Received array segment of size %d\n", rank, local_size);

    if (rank == 0) {
        // Copy the elements from the global array to the local array
        for (int i = 0; i < local_size; i++) {
            local_array[i] = array[i];
        }
    }

    // Calculate the local sum of the elements in the local array
    int local_sum = 0;
    for (int i = 0; i < local_size; i++) {
        local_sum += local_array[i];
    }

    printf("Process %d: local_start=%d, local_end=%d, local_sum=%d\n", rank, local_start, local_end, local_sum);

    // If the current process is the root process (rank 0)
    if (rank == 0) {
        // Initialize the total sum to the local sum of the current process
        int total_sum = local_sum;
        // Allocate memory for the received sums
        int *received_sums = (int *)malloc((size - 1) * sizeof(int));
        // Allocate memory for the requests
        requests = (MPI_Request *)malloc((size - 1) * sizeof(MPI_Request));

        // Receive the sums from the other processes in a non-blocking fashion
        for (int i = 1; i < size; i++) {
            MPI_Irecv(&received_sums[i-1], 1, MPI_INT, i, 3, MPI_COMM_WORLD, &requests[i-1]);
        }

        // Wait for all the requests to complete
        MPI_Waitall(size - 1, requests, MPI_STATUSES_IGNORE);

        // Add the received sums to the total sum
        for (int i = 0; i < size - 1; i++) {
            total_sum += received_sums[i];
        }

        printf("Total sum: %d\n", total_sum);

        // Free the memory allocated for the received sums
        free(received_sums);
        // Free the memory allocated for the array
        free(array);
    } else {
        // Allocate memory for the request
        MPI_Request send_request;
        // Send the sum to the root process in a non-blocking fashion
        MPI_Isend(&local_sum, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &send_request);
        printf("Process %d: Sent sum %d to process 0\n", rank, local_sum);
        // Wait for the request to complete
        MPI_Wait(&send_request, MPI_STATUS_IGNORE);
    }

    // Free the memory allocated for the local array
    free(local_array);
    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}