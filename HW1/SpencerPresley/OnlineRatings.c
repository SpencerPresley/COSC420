// let m be the number of products to be rated
// let n be the corresponding online ratings
// ratings are in the range 1 to 5 and are integers
// Compute the average ratings for each of the products and sort them in the end

/*
Details:

Program takes 'm' and 'n' and command line arguments.

Each score (in the range of [1, 5]) will be randomly generated. The seed will be the current time as defined by srand(time(NULL)). Call rand() with a simple formula to get the scores.

The program is restricted to only use the blocking methods 'MPI_Send' and 'MPI_Recv'.

Assign one process (corresponding to a score) to be the master whose responsibility includes preparing data and sending an array of scores to each worker process to compute the average. 

Because each array has the same typ of data, the tag can be the same when it's sent from the master process to each of it's worker processes. 

When a worker process finishes it's computation, it sends the result (the average rating for the assigned scores) back to the master process.

The reply message will send a different type of tag to the master process.

Once the master process receives the results from all worker processes, it sorts them and displays to the console, with precision 1 digit after the decimal point, from highest rating to lowest rating for each product indexed 1 through m.

For simplicity: assume m, the number of products to be rated, has its maximum to be equal to the total number of cores minus 1 of your cluster.

For example: If your cluster has a total of 16 cores, then the maximum number of products to be rated is 15. If your cluster has 56 cores, then the maximum number of products to be rated is 55.

For 'n', the number of ratings, it can be as large as 1 million, or as small as 5. This is to test the performance of the cluster if the size is large enough.

The following matrix illustrates some sample data and it's ratings. For simplicify you can prepare one array at a time and assume they all have the same number of ratings. 

  _____________________
1 | 5 | 4 | 3 | 2 | 1 |
   --------------------
2 | 1 | 2 | 3 | 1 | 4 |
   --------------------
3 | 3 | 5 | 1 | 5 | 2 |
   --------------------
4 | 2 | 4 | 3 | 4 | 2 |
   --------------------

Sorted ratings:
1: 3.4
2: 3.2
3: 3.0
4: 2.2
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

void get_ratings(int *ratings, int n) {
    for (int i = 0; i < n; i++) {
        ratings[i] = (rand() % 5) + 1;
    }
}

double get_average_for_ratings(int *ratings, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += ratings[i];
    }
    return (double)sum / n;
}

void merge(double *ratings_averages, int left, int middle, int right) {
    int n1 = middle - left + 1;
    int n2 = right - middle;

    double *left_array = (double *)malloc(n1 * sizeof(double));
    double *right_array = (double *)malloc(n2 * sizeof(double));

    for (int i = 0; i < n1; i++) {
        left_array[i] = ratings_averages[left + i];
    }

    for (int i = 0; i < n2; i++) {
        right_array[i] = ratings_averages[middle + 1 + i];
    }

    int i = 0;
    int j = 0;
    int k = left;

    // Sort in descending order
    while (i < n1 && j < n2) {
        if (left_array[i] >= right_array[j]) {
            ratings_averages[k] = left_array[i];
            i++;
        } else {
            ratings_averages[k] = right_array[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        ratings_averages[k] = left_array[i];
        i++;
        k++;
    }

    while (j < n2) {
        ratings_averages[k] = right_array[j];
        j++;
        k++;
    }

    free(left_array);
    free(right_array);
}

void merge_sort(double *ratings_averages, int left, int right) {
    if (left < right) {
        int middle = left + (right - left) / 2; // find middle point
        merge_sort(ratings_averages, left, middle); // recursively sort left half
        merge_sort(ratings_averages, middle + 1, right); // recursively sort right half
        merge(ratings_averages, left, middle, right); // merge sorted halves
    }
}

void sort(double *ratings_averages, int m) {
    return merge_sort(ratings_averages, 0, m - 1);
}

int is_argument_error(int argc, char **argv, int rank, int size, int m, int n) {
    if (argc != 3) {
        if (rank == 0) {
            printf("Ensure to enter an m and n as command line arguments, m is the number of products to be rated and n is the number of ratings for each product.\n");
        }
        MPI_Finalize();
        return 1;
    } else if (m > size - 1) {
        if (rank == 0) {
            printf("m cannot be greater than the number of cores minus 1. Number of cores: %d\n", size);
        }
        MPI_Finalize();
        return 1;
    } else if(n < 5) {
        if (rank == 0) {
            printf("n must be greater than 5\n");
        }
        MPI_Finalize();
        return 1;
    } else if (n > 1000000) {
        if (rank == 0) {
            printf("n must be less than 1,000,000\n");
        }
        MPI_Finalize();
        return 1;
    } else if (m < 1) {
        if (rank == 0) {
            printf("m must be greater than 0\n");
        }
        MPI_Finalize();
        return 1;
    } else if (n < 1) {
        if (rank == 0) {
            printf("n must be greater than 0\n");
        }
        MPI_Finalize();
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, char **argv) {
    int m, n, rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    m = atoi(argv[1]);
    n = atoi(argv[2]);

    if (is_argument_error(argc, argv, rank, size, m, n)) {
        return 1;
    }

    if (rank == 0) { // Master process
        srand(time(NULL));
        int *ratings = (int *)malloc(n * sizeof(int));
        double *ratings_averages = (double *)malloc(m * sizeof(double));

        for (int i = 0; i < m; i++) {
            get_ratings(ratings, n);
            MPI_Send(
                ratings, // Buffer which stores the ratings being sent
                n, // Count of elements to send, each product has n ratings
                MPI_INT, // Data type of the ratings are integers
                i + 1, // Rank of destination process
                0, // Tag of 0 for initial data
                MPI_COMM_WORLD
            );
        }

        for (int i = 0; i < m; i++) {
            MPI_Recv(
                &ratings_averages[i], // Buffer to store the received average rating
                1, // Count of elements to receive 
                MPI_DOUBLE, // Datatype of the average rating
                i + 1, // Source process
                1, // Tag of 1 for rating averages
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE
            );
        }

        sort(ratings_averages, m);

        printf("Sorted ratings:\n");
        for (int i = 0; i < m; i++) {
            printf("%d: %.1f\n", i + 1, ratings_averages[i]);
        }

        free(ratings);
        free(ratings_averages);
    } else { // Worker processes
        int *ratings = (int *)malloc(n * sizeof(int));
        MPI_Recv(
            ratings, // Buffer to store the received ratings
            n, // Count of elements to receive, each product has n ratings
            MPI_INT, // Datatype of the ratings are integers
            0, // Rank of the sender (master process)
            0, // Tag of 0 for inital data
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE
        );

        double average = get_average_for_ratings(ratings, n);
        MPI_Send(
            &average, // Buffer to store the average rating
            1, // Count of elements to send
            MPI_DOUBLE, // Datatype of the average rating
            0, // Rank of the destination process
            1, // Tag of 1 for rating average
            MPI_COMM_WORLD
        );
        free(ratings);
    }

    MPI_Finalize();
    return 0;
}