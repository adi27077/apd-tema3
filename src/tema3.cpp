#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>
#include <cmath>

using namespace std;

void logMessage(int from, int to) {
    cout << "M(" << from << "," << to << ")" << endl;
}

void printTopology(int topology[4][100], int rank) {
    cout << rank << " -> ";
    for (int i = 0; i < 4; i++) {
        cout << i << ":";
        for (int j = 1; topology[i][j] != 0; j++) {
            cout << topology[i][j];
            if (topology[i][j+1] != 0) {
                cout << ",";
            }
        }
        cout << " ";
    }
    cout << endl;
}

/*
Message Tags Used:
0 - Send topology from boss to boss
1 - Send topology from boss to worker
2 - Send vector from boss to boss
3 - Send vector from boss to worker
4 - Send finished vector from worker to boss
5 - Send finished vector from boss to boss
*/

int main(int argc, char *argv[]) {
    int  numtasks, rank, ret;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int vectorSize = stoi(argv[1]);
    int commError = stoi(argv[2]);

    int noOfWorkers;
    vector<int> workers;
    int topology[4][100] = {0};
    int v[vectorSize];
    int index, workersAvailable, elementsLeft;
    int vStartIndex;

    int bossRank, chunkSize;

    int workerVector[vectorSize];

    if (rank < 4) {  ///////////////////////////////Bosses
        bossRank = -1;

        /*-----------------------------Build topology-----------------------------*/

        ifstream f("cluster" + to_string(rank) + ".txt");
        f >> noOfWorkers;
        topology[rank][0] = noOfWorkers;

        //Read clusters
        for (int i = 0; i < noOfWorkers; i++) {
            int worker;
            f >> worker;
            workers.push_back(worker);
            topology[rank][i + 1] = worker;

            ret = MPI_Send(&rank, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << worker << endl;
            }
            logMessage(rank, worker);
        }
        
        //Build and pass topology
        if (rank == 0) {
            ret = MPI_Send(&topology[0][0], 100, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank + 1 << endl;
            }
            logMessage(rank, rank + 1);
        } else if (rank == 1 || rank == 2) {
            ret = MPI_Recv(&topology[0][0], rank * 100, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Send(&topology[0][0], (rank + 1) * 100, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank + 1 << endl;
            }
            logMessage(rank, rank + 1);
        } else if (rank == 3) {
            ret = MPI_Recv(&topology[0][0], 300, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }

            //At this point the whole topology is known so it can printed and sent to the others
            printTopology(topology, rank);
            //Send topology to workers
            for (int i = 0; i < noOfWorkers; i++) {
                ret = MPI_Send(&topology[0][0], 400, MPI_INT, workers[i], 1, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
            }
            //Send topology to boss 0
            ret = MPI_Send(&topology[0][0], 400, MPI_INT, 0, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 0 << endl;
            }
            logMessage(rank, 0);
            
        }

        //Receive full topology at boss 0, send it to the others for printing
        if (rank == 0) {
            ret = MPI_Recv(&topology[0][0], 400, MPI_INT, 3, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << 3 << " to " << rank << endl;
            }
            printTopology(topology, rank);
            //Send to workers
            for (int i = 0; i < noOfWorkers; i++) {
                ret = MPI_Send(&topology[0][0], 400, MPI_INT, workers[i], 1, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
            }
            //Send to boss 1
            ret = MPI_Send(&topology[0][0], 400, MPI_INT, 1, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 1 << endl;
            }
            logMessage(rank, 1);
        } else if (rank == 1) {
            ret = MPI_Recv(&topology[0][0], 400, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << 0 << " to " << rank << endl;
            }
            printTopology(topology, rank);
            //Send to workers
            for (int i = 0; i < noOfWorkers; i++) {
                ret = MPI_Send(&topology[0][0], 400, MPI_INT, workers[i], 1, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
            }
            //Send to boss 2
            ret = MPI_Send(&topology[0][0], 400, MPI_INT, 2, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 2 << endl;
            }
            logMessage(rank, 2);
        } else if (rank == 2) {
            ret = MPI_Recv(&topology[0][0], 400, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << 1 << " to " << rank << endl;
            }
            printTopology(topology, rank);
            //Send to workers
            for (int i = 0; i < noOfWorkers; i++) {
                ret = MPI_Send(&topology[0][0], 400, MPI_INT, workers[i], 1, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
            }

        }  //End of topology sending

        f.close();

        /*----------------------Vector generation and sending----------------------*/

        if (rank == 0) {
            //Generate vector
            if (rank == 0) {
                for (int i = 0; i < vectorSize; i++) {
                    v[i] = vectorSize - i - 1;
                }
            }

            //Divide vector for workers
            workersAvailable = topology[0][0] + topology[1][0] + topology[2][0] + topology[3][0];
            elementsLeft = vectorSize;
            index = 0;
            vStartIndex = 0;
            chunkSize = (int)ceil((double)elementsLeft / (double)workersAvailable);
            elementsLeft -= chunkSize * noOfWorkers;
            workersAvailable -= noOfWorkers;


            //Send vector to workers
            for (int i = 0; i < noOfWorkers; i++) {
                

                ret = MPI_Send(&chunkSize, 1, MPI_INT, workers[i], 3, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
                ret = MPI_Send(&v[index], chunkSize, MPI_INT, workers[i], 3, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
                index += chunkSize;
            }

            //Send info to boss 1
            ret = MPI_Send(&elementsLeft, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 1 << endl;
            }
            logMessage(rank, 1);
            ret = MPI_Send(&v[0], vectorSize, MPI_INT, 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 1 << endl;
            }
            logMessage(rank, 1);
            ret = MPI_Send(&workersAvailable, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 1 << endl;
            }
            logMessage(rank, 1);
            ret = MPI_Send(&index, 1, MPI_INT, 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << 1 << endl;
            }
            logMessage(rank, 1);

        } else if (rank == 1 || rank == 2) {
            cout << "Rank " << rank << " waiting for vector" << endl;
            
            //Receive info from previous boss
            ret = MPI_Recv(&elementsLeft, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Recv(&v[0], vectorSize, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Recv(&workersAvailable, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Recv(&index, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            vStartIndex = index;
            chunkSize = (int)ceil((double)elementsLeft / (double)workersAvailable);
            elementsLeft -= chunkSize * noOfWorkers;
            workersAvailable -= noOfWorkers;

            //Send vector to workers
            for (int i = 0; i < noOfWorkers; i++) {
                

                ret = MPI_Send(&chunkSize, 1, MPI_INT, workers[i], 3, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
                ret = MPI_Send(&v[index], chunkSize, MPI_INT, workers[i], 3, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
                index += chunkSize;
            }

            //Send info to next boss
            ret = MPI_Send(&elementsLeft, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank + 1 << endl;
            }  
            logMessage(rank, rank + 1);
            ret = MPI_Send(&v[0], vectorSize, MPI_INT, rank + 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank + 1 << endl;
            }
            logMessage(rank, rank + 1);
            ret = MPI_Send(&workersAvailable, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank + 1 << endl;
            }
            logMessage(rank, rank + 1);
            ret = MPI_Send(&index, 1, MPI_INT, rank + 1, 2, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank + 1 << endl;
            }
            logMessage(rank, rank + 1);

        } else if (rank == 3) {
            //Receive info from previous boss
            ret = MPI_Recv(&elementsLeft, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Recv(&v[0], vectorSize, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Recv(&workersAvailable, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            ret = MPI_Recv(&index, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << rank - 1 << " to " << rank << endl;
            }
            vStartIndex = index;
            chunkSize = (int)ceil((double)elementsLeft / (double)workersAvailable);
            elementsLeft -= chunkSize * noOfWorkers;
            workersAvailable -= noOfWorkers;

            //Send vector to workers
            for (int i = 0; i < noOfWorkers; i++) {
                

                ret = MPI_Send(&chunkSize, 1, MPI_INT, workers[i], 3, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
                ret = MPI_Send(&v[index], chunkSize, MPI_INT, workers[i], 3, MPI_COMM_WORLD);
                if (ret != MPI_SUCCESS) {
                    cout << "Error sending message from " << rank << " to " << workers[i] << endl;
                }
                logMessage(rank, workers[i]);
                index += chunkSize;
            }
        }

        /*-------------------------Result receiving---------------------------*/

        if (rank == 1 || rank == 2 || rank == 3) {
            if (rank == 1 || rank == 2) {
                //Receive partial result from previous boss
                ret = MPI_Recv(&v[0], vectorSize, MPI_INT, rank + 1, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret != MPI_SUCCESS) {
                    cout << "Error receiving message from " << rank + 1 << " to " << rank << endl;
                }
            }
            
            //Receive result from workers
            for (int i = 0; i < noOfWorkers; i++) {
                ret = MPI_Recv(&chunkSize, 1, MPI_INT, workers[i], 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret != MPI_SUCCESS) {
                    cout << "Error receiving message from " << workers[i] << " to " << rank << endl;
                }
                
                ret = MPI_Recv(&workerVector[0], chunkSize, MPI_INT, workers[i], 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret != MPI_SUCCESS) {
                    cout << "Error receiving message from " << workers[i] << " to " << rank << endl;
                }
                //Copy result to final vector
                for (int j = 0; j < chunkSize; j++) {
                    v[vStartIndex + i * chunkSize + j] = workerVector[j];
                }
            }

            //Send partial result to next boss
            ret = MPI_Send(&v[0], vectorSize, MPI_INT, rank - 1, 5, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS) {
                cout << "Error sending message from " << rank << " to " << rank - 1 << endl;
            }
            logMessage(rank, rank - 1);

        } else if (rank == 0) {
            //Receive partial result from previous boss
            ret = MPI_Recv(&v[0], vectorSize, MPI_INT, 1, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS) {
                cout << "Error receiving message from " << 1 << " to " << rank << endl;
            }

            //Receive result from workers
            for (int i = 0; i < noOfWorkers; i++) {
                ret = MPI_Recv(&chunkSize, 1, MPI_INT, workers[i], 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret != MPI_SUCCESS) {
                    cout << "Error receiving message from " << workers[i] << " to " << rank << endl;
                }

                ret = MPI_Recv(&workerVector[0], chunkSize, MPI_INT, workers[i], 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret != MPI_SUCCESS) {
                    cout << "Error receiving message from " << workers[i] << " to " << rank << endl;
                }
                //Copy result to final vector
                for (int j = 0; j < chunkSize; j++) {
                    v[i * chunkSize + j] = workerVector[j];
                }
            }

            //Print result
            cout << "Rezultat: ";
            for (int i = 0; i < vectorSize; i++) {
                cout << v[i] << " ";
            }
            cout << endl;
        }

    } else {  ///////////////////////////Workers
        
        /*------------------------Receive topology------------------------*/
        
        //Receive boss's rank
        ret = MPI_Recv(&bossRank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (ret != MPI_SUCCESS) {
            cout << "Error receiving message from " << MPI_ANY_SOURCE << " to " << rank << endl;
        }

        //Receive topology
        ret = MPI_Recv(&topology[0][0], 400, MPI_INT, bossRank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (ret != MPI_SUCCESS) {
            cout << "Error receiving message from " << bossRank << " to " << rank << endl;
        }
        printTopology(topology, rank);

        /*------------------------Compute vector------------------------*/

        //Receive vector
        ret = MPI_Recv(&chunkSize, 1, MPI_INT, bossRank, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (ret != MPI_SUCCESS) {
            cout << "Error receiving message from " << bossRank << " to " << rank << endl;
        }
        ret = MPI_Recv(&workerVector[0], chunkSize, MPI_INT, bossRank, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (ret != MPI_SUCCESS) {
            cout << "Error receiving message from " << bossRank << " to " << rank << endl;
        }

        //Compute
        for (int i = 0; i < chunkSize; i++) {
            workerVector[i] = workerVector[i] * 5;
        }

        //Send result to boss
        ret = MPI_Send(&chunkSize, 1, MPI_INT, bossRank, 4, MPI_COMM_WORLD);
        if (ret != MPI_SUCCESS) {
            cout << "Error sending message from " << rank << " to " << bossRank << endl;
        }
        logMessage(rank, bossRank);
        ret = MPI_Send(&workerVector[0], chunkSize, MPI_INT, bossRank, 4, MPI_COMM_WORLD);
        if (ret != MPI_SUCCESS) {
            cout << "Error sending message from " << rank << " to " << bossRank << endl;
        }
        logMessage(rank, bossRank);
    }

    MPI_Finalize();

    return 0;
}
