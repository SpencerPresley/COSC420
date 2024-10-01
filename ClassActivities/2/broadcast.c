/*
    Broadcast aka One-to-All mode example
    One process contributes to the results.
    All processes receive the result
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char broadcast_message[100];
    if (rank == 0) {
        strcpy(broadcast_message, "Hello World");
        for (int i = 0; i < size; i++) {
            MPI_Send(broadcast_message, strlen(broadcast_message) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(broadcast_message, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d received: %s\n", rank, broadcast_message);
    }

    MPI_Finalize();
    return 0;
}

