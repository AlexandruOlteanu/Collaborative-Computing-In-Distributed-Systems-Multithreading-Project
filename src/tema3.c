#include "mpi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 
#define  TOP_LEVEL 0

#define NUMBER_OF_ARGUMENTS 3
#define MAXIMUM_TOP_LEVEL  4
#define MAXIMUM_LOW_LEVEL 100
#define FILE_NAME_LENGTH 100
#define SEND_DATA_SIZE_TAG 0
#define SEND_DATA_TAG 1
#define SEND_TOP_LEVEL_TAG 2
#define SEND_START_INDEX_TAG 3
#define SEND_END_INDEX_TAG 4
#define DEFAULT_MULTIPLY_VALUE 5

char *my_itoa(int number) {

	if (!number) {
		return "0";
	}
	char *result = (char *) malloc(10 * sizeof(char));
	int p = 0;
	while (number) {
		result[p] = number % 10 + '0';
		number /= 10;
		++p;
	}
	result[p] = '\0';
	return result;
}


void extract_data(int *top_level_workers, int level) {

	char file_name[FILE_NAME_LENGTH];
	strcpy(file_name, "cluster");
	strcat(file_name, my_itoa(level));
	strcat(file_name, ".txt");

	FILE *file = fopen(file_name, "r");
	int workers_number;
	fscanf(file, "%d", &workers_number);
	top_level_workers[0] = workers_number;
	for (int i = 1; i <= workers_number; ++i) {
		fscanf(file, "%d", &top_level_workers[i]);
	}
}

void show_communication_message(int source, int destination) {

	printf("M(%d,%d)\n", source, destination);

}

void send_data_between_top_level(int **topology_data, int level) {

	int before = level - 1;
	int after = level + 1;

	if (before < 0) {
		before = MAXIMUM_TOP_LEVEL - 1;
	}	
	if (after >= MAXIMUM_TOP_LEVEL) {
		after = 0;
	}
	

	int data_size;
	MPI_Send(&topology_data[level][0], 1, MPI_INT, after, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
	show_communication_message(level, after);

	MPI_Send(topology_data[level], topology_data[level][0] + 1, MPI_INT, after, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(level, after);

	MPI_Send(&topology_data[level][0], 1, MPI_INT, before, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
	show_communication_message(level, before);

	MPI_Send(topology_data[level], topology_data[level][0] + 1, MPI_INT, before, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(level, before);

	MPI_Recv(&data_size, 1, MPI_INT, after, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	topology_data[after] = (int *) malloc((data_size + 1) * sizeof(int));
	MPI_Recv(topology_data[after], data_size + 1, MPI_INT, after, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	MPI_Recv(&data_size, 1, MPI_INT, before, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	topology_data[before] = (int *) malloc((data_size + 1) * sizeof(int));
	MPI_Recv(topology_data[before], data_size + 1, MPI_INT, before, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	MPI_Send(&topology_data[before][0], 1, MPI_INT, after, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
	show_communication_message(level, after);

	MPI_Send(topology_data[before], topology_data[before][0] + 1, MPI_INT, after, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(level, after);

	MPI_Send(&topology_data[after][0], 1, MPI_INT, before, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
	show_communication_message(level, before);
	
	MPI_Send(topology_data[after], topology_data[after][0] + 1, MPI_INT, before, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(level, before);

	int second_after = after + 1;
	if (second_after >= MAXIMUM_TOP_LEVEL) {
		second_after = 0;
	}

	MPI_Recv(&data_size, 1, MPI_INT, after, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	topology_data[second_after] = (int *) malloc((data_size + 1) * sizeof(int));
	MPI_Recv(topology_data[second_after], data_size + 1, MPI_INT, after, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	int second_before = before - 1;
	if (second_before < 0) {
		second_before = MAXIMUM_TOP_LEVEL - 1;
	}

	MPI_Recv(&data_size, 1, MPI_INT, before, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	topology_data[second_before] = (int *) malloc((data_size + 1) * sizeof(int));
	MPI_Recv(topology_data[second_before], data_size + 1, MPI_INT, before, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
}

void send_topology_to_workers(int** topology, int level) {
  	int workers = topology[level][0];
  	for (int i = 1; i <= workers; ++i) {
		int worker_rank = topology[level][i];
    	MPI_Send(&level, 1, MPI_INT, worker_rank, SEND_TOP_LEVEL_TAG, MPI_COMM_WORLD);
    	show_communication_message(level, worker_rank);

    	for (int j = 0; j < MAXIMUM_TOP_LEVEL; ++j) {
      		int data_size = topology[j][0];
      		MPI_Send(&data_size, 1, MPI_INT, worker_rank, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
      		show_communication_message(level, worker_rank);

      		MPI_Send(topology[j], data_size + 1, MPI_INT, worker_rank, SEND_DATA_TAG, MPI_COMM_WORLD);
      		show_communication_message(level, worker_rank);
    	}
  	}
}


void receive_topology_from_top_level(int **topology_data, int *top_level) {

	MPI_Recv(top_level, 1, MPI_INT, MPI_ANY_SOURCE, SEND_TOP_LEVEL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		int data_size;
		MPI_Recv(&data_size, 1, MPI_INT, *top_level, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		topology_data[i] = (int *) malloc((data_size + 1) * sizeof(int));
		MPI_Recv(topology_data[i], data_size + 1, MPI_INT, *top_level, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
}

void init_and_send_work(int *result, int result_size, int *topology_at_level, int total_workers) {
	for (int i = 0; i < result_size; ++i) {
		result[i] = result_size - i - 1;
	}
	
	int work_amount = result_size / total_workers;
	int start = 0, end = work_amount - 1;
	for (int i = 1; i <= topology_at_level[0]; ++i) {
		MPI_Send(&start, 1, MPI_INT, topology_at_level[i], SEND_START_INDEX_TAG, MPI_COMM_WORLD);
    	show_communication_message(TOP_LEVEL, topology_at_level[i]);

		MPI_Send(&end, 1, MPI_INT, topology_at_level[i], SEND_END_INDEX_TAG, MPI_COMM_WORLD);
    	show_communication_message(TOP_LEVEL, topology_at_level[i]);

		MPI_Send(result, result_size, MPI_INT, topology_at_level[i], SEND_DATA_TAG, MPI_COMM_WORLD);
    	show_communication_message(TOP_LEVEL, topology_at_level[i]);

		start += work_amount;
		end += work_amount;
	}
}

void receive_result_from_top_level(int top_level, int result_size, int *result, int *start, int *end) {

	MPI_Recv(start, 1, MPI_INT, top_level, SEND_START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(end, 1, MPI_INT, top_level, SEND_END_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(result, result_size, MPI_INT, top_level, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}


void calculate_and_send_result_chunk(int level, int top_level, int result_size, int *result, int start, int end) {

	for (int i = start; i <= end; ++i) {
		result[i] *= DEFAULT_MULTIPLY_VALUE;
	}

	MPI_Send(&start, 1, MPI_INT, top_level, SEND_START_INDEX_TAG, MPI_COMM_WORLD);
	show_communication_message(level, top_level);

	MPI_Send(&end, 1, MPI_INT, top_level, SEND_END_INDEX_TAG, MPI_COMM_WORLD);
	show_communication_message(level, top_level);

	MPI_Send(result, result_size, MPI_INT, top_level, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(level, top_level);


}

void receive_result_from_workers(int level, int *topology_at_level, int *result, int result_size) {

	int start, end;
	int *updated_result = (int *) malloc(result_size * sizeof(int));

	for (int i = 1; i <= topology_at_level[0]; ++i) {

		MPI_Recv(&start, 1, MPI_INT, topology_at_level[i], SEND_START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&end, 1, MPI_INT, topology_at_level[i], SEND_END_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(updated_result, result_size, MPI_INT, topology_at_level[i], SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		for (int j = start; j <= end; ++j) {
			result[j] = updated_result[j];
		}

	}
}

void transfer_result(int source, int destination, int *result, int result_size) {

	MPI_Send(result, result_size, MPI_INT, destination, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(source, destination);

}

void receive_result(int source, int destination, int *result, int result_size) {

	MPI_Recv(result, result_size, MPI_INT, destination, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}

void transfer_result_to_workers(int level, int *result, int result_size, int **topology_data, int total_workers, short last) {

	int work_amount = result_size / total_workers;
	int start = 0, end = 0;

	int current_level = level;
	int taken = 0;
	while (current_level != TOP_LEVEL) {
		++current_level;
		if (current_level >= MAXIMUM_TOP_LEVEL) {
			current_level = TOP_LEVEL;
		}
		taken += topology_data[current_level][0];
	}

	start = taken * work_amount;
	end = start + work_amount - 1;

	for (int i = 1; i <= topology_data[level][0]; ++i) {
		if (last && i == topology_data[level][0]) {
			end = result_size;
		}
		MPI_Send(&start, 1, MPI_INT, topology_data[level][i], SEND_START_INDEX_TAG, MPI_COMM_WORLD);
    	show_communication_message(level, topology_data[level][i]);

		MPI_Send(&end, 1, MPI_INT, topology_data[level][i], SEND_END_INDEX_TAG, MPI_COMM_WORLD);
    	show_communication_message(level, topology_data[level][i]);

		MPI_Send(result, result_size, MPI_INT, topology_data[level][i], SEND_DATA_TAG, MPI_COMM_WORLD);
    	show_communication_message(level, topology_data[level][i]);

		start += work_amount;
		end += work_amount;
	}

}



void show_final_result(int result_size, int *result) {

	printf("Rezultat: ");
	for (int i = 0; i < result_size; ++i) {
		printf("%d ", result[i]);
	}
	printf("\n");
}

void write_topology(int **topology_data, int level) {

	printf("%d -> ", level);
	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		if (topology_data[i] != NULL) {
			int workers = topology_data[i][0];
			printf("%d:", i);
			for (int j = 1; j <= workers; ++j) {
				printf("%d", topology_data[i][j]);
				if (j < workers) {
					printf(",");
				}
			}
			printf(" ");
		}
	}
	printf("\n");

}


int main (int argc, char *argv[]) {
    int level;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &level);

	if (argc != NUMBER_OF_ARGUMENTS) {
		
		if (level == TOP_LEVEL) {
			printf("####################################\n");
			printf("# Error: Wrong number of arguments #\n");
			printf("####################################\n");
		}
		MPI_Finalize();
		return EXIT_FAILURE;
	}

	int **topology_data = (int **) malloc(MAXIMUM_TOP_LEVEL * sizeof(int *));
	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		topology_data[i] = NULL;
	}
	
	int *top_level_workers = (int *) malloc(MAXIMUM_LOW_LEVEL * sizeof(int));

	int top_level;
	if (level < MAXIMUM_TOP_LEVEL) {
		extract_data(top_level_workers, level);
		int workers_number = top_level_workers[0];
		topology_data[level] = (int *) malloc((workers_number + 1) * sizeof(int));
		for (int i = 0; i <= workers_number; ++i) {
			topology_data[level][i] = top_level_workers[i];
		}
		
		send_data_between_top_level(topology_data, level);
		send_topology_to_workers(topology_data, level);
	}

	if (level >= MAXIMUM_TOP_LEVEL) {
		receive_topology_from_top_level(topology_data, &top_level);
	}
	
	write_topology(topology_data, level);

	MPI_Barrier(MPI_COMM_WORLD);

	int result_size = atoi(argv[1]);
	int error = atoi(argv[2]);
	int *result = (int *) malloc(result_size * sizeof(int));

	int total_workers = 0;
	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		total_workers += topology_data[i][0];
	}

	if (level == TOP_LEVEL) {

		init_and_send_work(result, result_size, topology_data[level], total_workers);
		receive_result_from_workers(level, topology_data[level], result, result_size);
		transfer_result(level, 3, result, result_size);
		if (error != 0) {

		}
		else {
			receive_result(level, 1, result, result_size);
			show_final_result(result_size, result);
		}
		
	}
	else if (level < MAXIMUM_TOP_LEVEL) {

		if (level == 3) {
			receive_result(level, TOP_LEVEL, result, result_size);
			transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 0);
			receive_result_from_workers(level, topology_data[level], result, result_size);
			transfer_result(level, 2, result, result_size);

		}

		if (level == 2) {
			receive_result(level, 3, result, result_size);
			transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 0);
			receive_result_from_workers(level, topology_data[level], result, result_size);
			transfer_result(level, 1, result, result_size);
		}

		if (level == 1) {
			receive_result(level, 2, result, result_size);
			transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 1);
			receive_result_from_workers(level, topology_data[level], result, result_size);
			transfer_result(level, 0, result, result_size);
		}

	}
	else {

		int start, end;
		receive_result_from_top_level(top_level, result_size, result, &start, &end);
		calculate_and_send_result_chunk(level, top_level, result_size, result, start, end);
	}


	
 
    MPI_Finalize();

	return EXIT_SUCCESS;
}