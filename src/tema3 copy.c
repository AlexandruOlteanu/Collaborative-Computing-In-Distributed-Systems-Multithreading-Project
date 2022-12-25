#include "mpi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

#define TOPOLOGY_MESSAGE_LEN 200
#define RESULT_MESSAGE_LEN 1000000

#define ROOT_COORDINATOR 0
#define NUM_COORDINATOR_PROCESS 4
#define SIZE_TAG 1
#define DATA_TAG 2
#define PARENT_REVEAL_TAG 3

void log_message(int source, int destination) {
	printf("M(%d,%d)\n", source, destination);
}

void print_result(int array_size, int* array) {
    char message[RESULT_MESSAGE_LEN];
    sprintf(message, "Rezultat: ");
	for (int i = 0; i < array_size - 1; i++) {
		sprintf(message + strlen(message), "%d ", array[i]);
	}
	sprintf(message + strlen(message), "%d\n", array[array_size - 1]);

    printf("%s", message);
}

void announce_topology(int rank, int* coordinator_sizes, int** topology) {
	char message[TOPOLOGY_MESSAGE_LEN];
	sprintf(message, "%d -> ", rank);
	for (int i = 0; i < NUM_COORDINATOR_PROCESS; i++) {
		sprintf(message + strlen(message), "%d:", i);
		for (int j = 0; j < coordinator_sizes[i]; j++) {
			if (j == coordinator_sizes[i] - 1 && i == NUM_COORDINATOR_PROCESS - 1) {
				sprintf(message + strlen(message), "%d", topology[i][j]);
			} else if (j == coordinator_sizes[i] - 1) { 
				sprintf(message + strlen(message), "%d ", topology[i][j]);
			} else {
				sprintf(message + strlen(message), "%d,", topology[i][j]);
			}
		}
	}
	printf("%s\n", message);
}

void read_cluster_file(int rank, int* coordinator_sizes, int** topology) {
    char cluster_file_name[20];
	sprintf(cluster_file_name, "cluster%d.txt", rank);
	FILE *fp = fopen(cluster_file_name, "r");;

	fscanf(fp, "%d", &coordinator_sizes[rank]);
	topology[rank] = calloc(coordinator_sizes[rank], sizeof(int));
	for (int i = 0; i < coordinator_sizes[rank]; i++) {
		fscanf(fp, "%d", &topology[rank][i]);
	}

	fclose(fp);
}

void sync_coordinators(int rank, int* coordinator_sizes, int** topology) {
	for (int i = 0; i < NUM_COORDINATOR_PROCESS; i++) {
		if (rank != i) {
			MPI_Send(&coordinator_sizes[rank], 1, MPI_INT, i, SIZE_TAG, MPI_COMM_WORLD);
			log_message(rank, i);

			MPI_Send(topology[rank], coordinator_sizes[rank], MPI_INT, i, DATA_TAG, MPI_COMM_WORLD);
			log_message(rank, i);
		}
	}

	for (int i = 0; i < NUM_COORDINATOR_PROCESS; i++) {
		if (rank != i) {
			MPI_Recv(&coordinator_sizes[i], 1, MPI_INT, i, SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			topology[i] = calloc(coordinator_sizes[i], sizeof(int));

			MPI_Recv(topology[i], coordinator_sizes[i], MPI_INT, i, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}

	announce_topology(rank, coordinator_sizes, topology);
}

void sync_workers(int rank, int* coordinator_sizes, int** topology) {
	for (int i = 0; i < coordinator_sizes[rank]; i++) {
		// send parent rank
		MPI_Send(&rank, 1, MPI_INT, topology[rank][i], PARENT_REVEAL_TAG, MPI_COMM_WORLD);
		log_message(rank, topology[rank][i]);

		// send topology
		for (int j = 0; j < NUM_COORDINATOR_PROCESS; j++) {
			MPI_Send(&coordinator_sizes[j], 1, MPI_INT, topology[rank][i], SIZE_TAG, MPI_COMM_WORLD);
			log_message(rank, topology[rank][i]);

			MPI_Send(topology[j], coordinator_sizes[j], MPI_INT, topology[rank][i], DATA_TAG, MPI_COMM_WORLD);
			log_message(rank, topology[rank][i]);
		}
	}
}

void receive_topology(int* parent_coordinator, int* coordinator_sizes, int** topology, int rank) {
    // receive parent rank
	MPI_Recv(parent_coordinator, 1, MPI_INT, MPI_ANY_SOURCE, PARENT_REVEAL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
    // receive topology
	for (int i = 0; i < NUM_COORDINATOR_PROCESS; i++) {
		MPI_Recv(&coordinator_sizes[i], 1, MPI_INT, *parent_coordinator, SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		topology[i] = calloc(coordinator_sizes[i], sizeof(int));

		MPI_Recv(topology[i], coordinator_sizes[i], MPI_INT, *parent_coordinator, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	announce_topology(rank, coordinator_sizes, topology);
};

void calculate_start_end_indexes(int** worker_starts, int** worker_ends, int num_workers, int array_size) {
	int num_tasks = num_workers + NUM_COORDINATOR_PROCESS;
	*worker_starts = calloc(num_tasks, sizeof(int));
	*worker_ends = calloc(num_tasks, sizeof(int));

	for (int i = 3; i < num_tasks; i++) {
		int temp_id = (i - NUM_COORDINATOR_PROCESS);
		(*worker_starts)[i] = temp_id * (double)array_size / num_workers;
	 	(*worker_ends)[i] = min((temp_id + 1) * (double)array_size / num_workers, array_size);
	}
}

void calculate_initial_array_state(int array_size, int* array) {
    for (int i = 0; i < array_size; i++) {
        array[i] = i;
    }
}

void prelucrate_and_communicate_array_segment(int worker_size, int parent_coordinator, int rank) {
    int* temp_array = calloc(worker_size, sizeof(int));

    MPI_Recv(temp_array, worker_size, MPI_INT, parent_coordinator, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    for (int i = 0; i < worker_size; i++) {
        temp_array[i] *= 2;
    }

    MPI_Send(temp_array, worker_size, MPI_INT, parent_coordinator, DATA_TAG, MPI_COMM_WORLD);
    log_message(rank, parent_coordinator);

    free(temp_array);
}

void free_coordinators_memory(int* worker_starts, int* worker_ends, int* array) {
    free(worker_starts);
    free(worker_ends);
    free(array);
}

void free_general_memory(int* coordinator_sizes, int** topology) {
    free(coordinator_sizes);
    for (int i = 0; i < NUM_COORDINATOR_PROCESS; i++) {
        free(topology[i]);
    }
    free(topology);
}

int main (int argc, char *argv[]) {
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (argc < 3 && rank == 0) {
		fprintf(stderr, "Usage:\n\tmpirun -np <numar_procese> ./tema3 <dimensiune_vector> <eroare_comunicatie>\n");
		MPI_Finalize();
		return 0;
	} else if (argc < 3) {
		MPI_Finalize();
		return 0;
	}

	int parent_coordinator = 0;
	int* coordinator_sizes = calloc(NUM_COORDINATOR_PROCESS, sizeof(int));
	int** topology = calloc(NUM_COORDINATOR_PROCESS, sizeof(int*));

	if (rank < 3) {
        parent_coordinator = rank;

		read_cluster_file(rank, coordinator_sizes, topology);
		sync_coordinators(rank, coordinator_sizes, topology);
		sync_workers(rank, coordinator_sizes, topology);
	} else {
		receive_topology(&parent_coordinator, coordinator_sizes, topology, rank);
	}

	MPI_Barrier(MPI_COMM_WORLD);
 
	int array_size = atoi(argv[1]);
    int *array = calloc(array_size, sizeof(int));

	int num_workers = (coordinator_sizes[0] + coordinator_sizes[1] + coordinator_sizes[2]);
    int* worker_starts;
	int* worker_ends;

	if (rank == 0) {
        calculate_initial_array_state(array_size, array);

        // send array to other coordinators
        for (int i = 1; i < NUM_COORDINATOR_PROCESS; i++) {
            MPI_Send(array, array_size, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD);
            log_message(rank, i);
        }

		calculate_start_end_indexes(&worker_starts, &worker_ends, num_workers, array_size);

		// send to own workers
		for (int i = 0; i < coordinator_sizes[rank]; i++) {
			int worker_start = worker_starts[topology[rank][i]];
			int worker_size = worker_ends[topology[rank][i]] - worker_start;

			MPI_Send(array + worker_start, worker_size, MPI_INT, topology[rank][i], DATA_TAG, MPI_COMM_WORLD);
            log_message(rank, topology[rank][i]);
		}

        // receive array from everyone
        for (int i = 0; i < NUM_COORDINATOR_PROCESS; i++) {
            for (int j = 0; j < coordinator_sizes[i]; j++) {
                int worker_start = worker_starts[topology[i][j]];
			    int worker_size = worker_ends[topology[i][j]] - worker_start;

                int sender = (i == ROOT_COORDINATOR) ? topology[i][j] : i;
                MPI_Recv(array + worker_start, worker_size, MPI_INT, sender, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

		print_result(array_size, array);
	} else if (rank == 1 || rank == 2) {
        // receice initial array from root coordinator
        MPI_Recv(array, array_size, MPI_INT, ROOT_COORDINATOR, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		calculate_start_end_indexes(&worker_starts, &worker_ends, num_workers, array_size);

        // send array bits to workers, receive them prelucrated, and send them again to the root coordinator
        for (int i = 0; i < coordinator_sizes[rank]; i++) {
			int worker_start = worker_starts[topology[rank][i]];
			int worker_size = worker_ends[topology[rank][i]] - worker_start;

			MPI_Send(array + worker_start, worker_size, MPI_INT, topology[rank][i], DATA_TAG, MPI_COMM_WORLD);
            log_message(rank, topology[rank][i]);

			MPI_Recv(array + worker_start, worker_size, MPI_INT, topology[rank][i], DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            MPI_Send(array + worker_start, worker_size, MPI_INT, ROOT_COORDINATOR, DATA_TAG, MPI_COMM_WORLD);
            log_message(rank, ROOT_COORDINATOR);
		}
	} else {
        int temp_id = (rank - NUM_COORDINATOR_PROCESS);
        int start = temp_id * (double)array_size / num_workers;
        int end = min((temp_id + 1) * (double)array_size / num_workers, array_size);
        
        prelucrate_and_communicate_array_segment(end - start, parent_coordinator, rank);
	}

    if (rank < 3) {
        free_coordinators_memory(worker_starts, worker_ends, array);
    }
    free_general_memory(coordinator_sizes, topology);

    MPI_Finalize();
}