/*
    Gather aka All-to-One mode.
    All processes contribute to the result.
    One process receives the result.
*/

#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>
struct Result {
    int inserted_value;
    int rank_who_inserted;
};

int main(int argc, char **argv) {
    int rank, size;
    srand(time(NULL));

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    struct Result *result_arr = malloc(size * sizeof(struct Result));

    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            MPI_Send(&result_arr[rank], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&result_arr[rank], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int insert_value = rand() % 100;
        result_arr[rank].inserted_value = insert_value;
        result_arr[rank].rank_who_inserted = rank;
    }

    MPI_Gather(&result_arr[rank], 1, MPI_INT, result_arr, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int *result_arr_final = malloc(size * sizeof(int));
    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            result_arr_final[i] = result_arr[i].inserted_value;
            printf("inserted_value: %d, rank_who_inserted: %d\n", result_arr_final[i], result_arr[i].rank_who_inserted);
        }
    }
    MPI_Finalize();
    free(result_arr);
    free(result_arr_final);
    return 0;
}