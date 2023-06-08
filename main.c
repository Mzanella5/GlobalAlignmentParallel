#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>


#define LINE_SIZE 5
#define COLUMN_SIZE 5

typedef struct {
    int line;
    int previousPid;
    int previous[LINE_SIZE];
    int current[LINE_SIZE];
} XVet;

void PrintVet(int vet[], int size, int pid)
{
    int i;
    printf("\nPid: %d\n", pid);
    for(i=0; i<LINE_SIZE; i++)
        printf(" %d|", vet[i]);
    printf("\n");
}

void RunMaster(int pid, int numProcesses, MPI_Datatype MyStructType)
{
    int i, slaveId, buffer;
    printf("Master %d\n", pid);

    // int **mat = InitializeMatrix(LINE_SIZE, LINE_SIZE);

    // for (i=1; i < LINE_SIZE; i++)
    // {
    //     mat[i][0] = mat[i-1][0] - 1;
    // }

    // for (i=1; i < LINE_SIZE; i++)
    // {
    //     mat[0][i] = mat[0][i-1] -1;
    // }

    XVet data;

    for (i=0; i < LINE_SIZE; i++)
    {
        data.previous[i] = 0;
        data.current[i] = 0;
    }

    for (i=1; i < LINE_SIZE; i++)
    {
        data.previous[i] = data.previous[i-1] -1;
    }

    
    for (i=1, slaveId = 1; i <= LINE_SIZE; i++, slaveId++)
    {
        if(slaveId == numProcesses)
            slaveId = 1;
        
        // slave are ready, send the line that will be processed by the process
        MPI_Send(&i, 1, MPI_INT, slaveId, 0, MPI_COMM_WORLD);
    }

    // when the matrix lines finished we need to inform this to the slave process
    buffer = 0;
    for (slaveId = 1; slaveId < numProcesses; slaveId++)
    {
        // zero means the process will calculate the backtracing path
        MPI_Send(&buffer, 1, MPI_INT, slaveId, 0, MPI_COMM_WORLD);
    }

    //MPI_Send(&data, 1, MyStructType, slaveId, 0, MPI_COMM_WORLD);
}

void RunSlave(int pid, int numProcesses, MPI_Datatype MyStructType)
{
    int i, line, buffer;
    //printf("Slave %d\n", pid);

    // XVet receivedData;

    // MPI_Recv(&receivedData, 1, MyStructType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // printf("%dReceived data:\n", pid);
    // printf("line: %d\n", receivedData.line);
    // printf("previous pid: %d\n", receivedData.previousPid);

    // PrintVet(receivedData.previous, LINE_SIZE, pid);

    // printf("\n");
    // PrintVet(receivedData.current, LINE_SIZE, pid);
    // receivedData.current[receivedData.line] = receivedData.line * -1;
    // for(i=1; i<LINE_SIZE; i++)
    // {
    //     printf(" %d|", receivedData.current[i]);
    //     receivedData.current[i] = receivedData.previous[i-1] + receivedData.previous[i] + receivedData.current[i-1];
    // }

    // PrintVet(receivedData.current, LINE_SIZE, pid);
    // printf("\n");
    // MPI_Send(&receivedData, 1, MyStructType, 0, 0, MPI_COMM_WORLD);

    do
    {
        MPI_Recv(&line, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(line > 0)
        {
            printf("P:%d L:%d\n", pid, line);
        }
    } while (line > 0);
    
}

int** InitializeMatrix(int sizex, int sizey)
{
    int** mat = (int**) calloc(sizex, sizeof(int*));
    for (int i = 0; i < sizex; i++)
        mat[i] = (int*) calloc(sizey, sizeof(int));
    
    return mat;
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int numProcesses, pid;
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);


    // Definindo o tipo de dados derivado
    XVet xvet;
    MPI_Datatype MyStructType;
    MPI_Datatype types[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    int blocklengths[4] = { 1, 1, LINE_SIZE, LINE_SIZE };
    MPI_Aint offsets[4];
    MPI_Get_address(&xvet.line, &offsets[0]);
    MPI_Get_address(&xvet.previousPid, &offsets[1]);
    MPI_Get_address(&xvet.previous, &offsets[2]);
    MPI_Get_address(&xvet.current, &offsets[3]);
    offsets[1] -= offsets[0];
    offsets[2] -= offsets[0];
    offsets[3] -= offsets[0];
    offsets[0] = 0;
    MPI_Type_create_struct(4, blocklengths, offsets, types, &MyStructType);
    MPI_Type_commit(&MyStructType);


    if(pid == 0) // Master process
    {
        RunMaster(pid, numProcesses, MyStructType);
    }
    else // Slave process
    {
        RunSlave(pid, numProcesses, MyStructType);
    }

    MPI_Type_free(&MyStructType);
    MPI_Finalize();
    return 0;
}
