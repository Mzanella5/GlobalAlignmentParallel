#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int numProcesses, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Processo %d|%d\n", rank, numProcesses);

    MPI_Finalize();
    return 0;
}
