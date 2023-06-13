#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#define LINE_SIZE 5
#define COLUMN_SIZE 5

int MOUNT = 1;

typedef struct {
    int line;
    int previousPid;
    int nextPid;
    int previousScore[LINE_SIZE];
    int currentScore[LINE_SIZE];
} SCORE;

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
    char direction;
    int *processQueue = (int*) malloc(sizeof(int) * LINE_SIZE);
    printf("Master %d\n", pid);

    SCORE score;

    for (i=0; i < LINE_SIZE; i++)
    {
        score.previousScore[i] = 0;
    }

    for (i=1; i < LINE_SIZE; i++)
    {
        score.previousScore[i] = score.previousScore[i-1] -1;
    }

    processQueue[0] = 0;
    for (i=1, slaveId = 1; i <= LINE_SIZE; i++, slaveId++)
    {
        if(slaveId == numProcesses)
            slaveId = 1;
        
        score.line = i;
        score.previousPid = processQueue[i-1];

        if(i == LINE_SIZE)
            score.nextPid = 0;
        else
            if(slaveId +1 == numProcesses)
                score.nextPid = 1;
            else
                score.nextPid = slaveId +1;

        MPI_Send(&score, 1, MyStructType, slaveId, 0, MPI_COMM_WORLD);

        // Send the line that will be processed by the process
        //MPI_Send(&i, 1, MPI_INT, slaveId, 0, MPI_COMM_WORLD);
        processQueue[i] = slaveId;
    }

    // when the matrix lines finished we need to inform this to the slave process
    buffer = 0;
    for (slaveId = 1; slaveId < numProcesses; slaveId++)
    {
        // zero means the process will calculate the backtracing path
        score.line = 0;
        MPI_Send(&score, 1, MyStructType, slaveId, 0, MPI_COMM_WORLD);
        //MPI_Send(&buffer, 1, MPI_INT, slaveId, 0, MPI_COMM_WORLD);
    }

    // wait the last process send the start backtracing path
    for (i = LINE_SIZE -1; i > 0; i--)
    {
        printf("Waiting process %d\n", processQueue[i]);
        MPI_Recv(&direction, 1, MPI_INT, processQueue[i], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d receive direction %c\n", processQueue[i], direction);
    }

    free(processQueue);
}

void RunSlave(int pid, int numProcesses, MPI_Datatype MyStructType)
{
    int i, line, buffer, count, sendMount, receiveMount;
    char direction;
    SCORE score;
    int *subScores = (int*) calloc(MOUNT, sizeof(int));
    //printf("Slave %d\n", pid);

    // SCORE receivedData;

    // MPI_Recv(&receivedData, 1, MyStructType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // printf("%dReceived data:\n", pid);
    // printf("line: %d\n", receivedData.line);
    // printf("previousScore pid: %d\n", receivedData.previousPid);

    // PrintVet(receivedData.previousScore, LINE_SIZE, pid);

    // printf("\n");
    // PrintVet(receivedData.currentScore, LINE_SIZE, pid);
    // receivedData.currentScore[receivedData.line] = receivedData.line * -1;
    // for(i=1; i<LINE_SIZE; i++)
    // {
    //     printf(" %d|", receivedData.currentScore[i]);
    //     receivedData.currentScore[i] = receivedData.previousScore[i-1] + receivedData.previousScore[i] + receivedData.currentScore[i-1];
    // }

    // PrintVet(receivedData.currentScore, LINE_SIZE, pid);
    // printf("\n");
    // MPI_Send(&receivedData, 1, MyStructType, 0, 0, MPI_COMM_WORLD);

    MPI_Recv(&score, 1, MyStructType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    while (score.line)
    {
        score.currentScore[0] = score.line * -1;

        for(i=1, count=0; i<COLUMN_SIZE; i++, count++)
        {
            score.currentScore[i] = score.previousScore[i-1] + score.previousScore[i] + score.currentScore[i-1];
            count++;

            if(count == MOUNT)
            {
                if(score.nextPid)
                {
                    MPI_Send(&score.currentScore[i-count], MOUNT, MPI_INT, score.nextPid, 0, MPI_COMM_WORLD);
                }
         
                MPI_Recv(&score.previousScore[i+1], MOUNT, MPI_INT, score.previousPid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
                count = 0;
            }
        }

        MPI_Recv(&score, 1, MyStructType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("P:%d L:%d\n", pid, line);
    }
    direction = 'D';
    MPI_Send(&direction, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    
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
    SCORE xvet;
    MPI_Datatype MyStructType;
    MPI_Datatype types[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    int blocklengths[4] = { 1, 1, LINE_SIZE, LINE_SIZE };
    MPI_Aint offsets[4];
    MPI_Get_address(&xvet.line, &offsets[0]);
    MPI_Get_address(&xvet.previousPid, &offsets[1]);
    MPI_Get_address(&xvet.previousScore, &offsets[2]);
    MPI_Get_address(&xvet.currentScore, &offsets[3]);
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
