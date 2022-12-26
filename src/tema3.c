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

void send_topology_to_workers(int **topology_data, int level) {

	int workers = topology_data[level][0];
	for (int i = 1; i <= workers; ++i) {
		MPI_Send(&level, 1, MPI_INT, topology_data[level][i], SEND_TOP_LEVEL_TAG, MPI_COMM_WORLD);
		show_communication_message(level, topology_data[level][i]);

		for (int j = 0; j < MAXIMUM_TOP_LEVEL; ++j) {
			MPI_Send(&topology_data[j][0], 1, MPI_INT, topology_data[level][i], SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
			show_communication_message(level, topology_data[level][i]);

			MPI_Send(topology_data[j], topology_data[j][0] + 1, MPI_INT, topology_data[level][i], SEND_DATA_TAG, MPI_COMM_WORLD);
			show_communication_message(level, topology_data[level][i]);
			
		}

	}
}

void receive_data_from_top_level(int **topology_data, int *top_level) {

	MPI_Recv(top_level, 1, MPI_INT, MPI_ANY_SOURCE, SEND_TOP_LEVEL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		int data_size;
		MPI_Recv(&data_size, 1, MPI_INT, *top_level, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		topology_data[i] = (int *) malloc((data_size + 1) * sizeof(int));
		MPI_Recv(topology_data[i], data_size + 1, MPI_INT, *top_level, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
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
		receive_data_from_top_level(topology_data, &top_level);
	}
	
	write_topology(topology_data, level);

	
 
    MPI_Finalize();

	return EXIT_SUCCESS;
}