#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
 
#define  TOP_LEVEL 0

#define NUMBER_OF_ARGUMENTS 3


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


	for (int i = 0; i < 10; ++i) {
		if (level == i) {
			printf("Sunt aici : %d\n", level);
		}
	}


 
    MPI_Finalize();

	return EXIT_SUCCESS;
}