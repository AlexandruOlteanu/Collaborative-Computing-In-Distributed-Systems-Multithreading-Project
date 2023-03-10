/**
 *  <Copyright Alexandru Olteanu, grupa 332CA, alexandruolteanu2001@gmail.com>
 * */
#include "mpi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 
#define TOP_LEVEL 0
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
#define NO_ERROR 0
#define CONNECTION_ERROR 1
#define PARTITION_ERROR 2

/**
 * Functie ce transforma un numar intr-un sir de caractere, folosita pentru numele 
 * fisierelor de input
 * */
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

/**
 * In functie de cluserul in care suntem extragem numele fisierelor si cititm 
 * procesele worker ale coordonatorului curent
 * */
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

/**
 * Afiseaza notificarea cu privire la trimiterea unui mesaj de la un proces la altul
 * */
void show_communication_message(int source, int destination) {

	printf("M(%d,%d)\n", source, destination);

}

/**
 * Functie care trimite topologia procesului "level_of_data" de la procesul "source" la procesul "destination"
 * */
void send_data_to_top_level(int source, int destination, int level_of_data, int **topology_data) {

	MPI_Send(&topology_data[level_of_data][0], 1, MPI_INT, destination, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
	show_communication_message(source, destination);

	MPI_Send(topology_data[level_of_data], topology_data[level_of_data][0] + 1, MPI_INT, destination, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(source, destination);
}

/**
 * Functie care primeste topologia procesului "level_of_data" de la procesul "destination" la procesul "source"
 * */
void receive_data_from_top_level(int source, int destination, int level_of_data, int **topology_data) {

	int data_size;
	MPI_Recv(&data_size, 1, MPI_INT, destination, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	topology_data[level_of_data] = (int *) malloc((data_size + 1) * sizeof(int));
	MPI_Recv(topology_data[level_of_data], data_size + 1, MPI_INT, destination, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}

/**
 * Functie in care un coordonator parcurge lista de workeri din interiorul clusterului sau si le trimite 
 * topologia fiecaruia dintre ei.
 * */
void send_topology_to_workers(int **topology, int level) {
  	int workers = topology[level][0];
  	for (int i = 1; i <= workers; ++i) {
		int worker_rank = topology[level][i];
    	MPI_Send(&level, 1, MPI_INT, worker_rank, SEND_TOP_LEVEL_TAG, MPI_COMM_WORLD);
    	show_communication_message(level, worker_rank);

    	for (int j = 0; j < MAXIMUM_TOP_LEVEL; ++j) {
      		int data_size = 0;
			if (topology[j] == NULL) {
				MPI_Send(&data_size, 1, MPI_INT, worker_rank, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
      			show_communication_message(level, worker_rank);
				continue;
			}
			data_size = topology[j][0];
      		MPI_Send(&data_size, 1, MPI_INT, worker_rank, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD);
      		show_communication_message(level, worker_rank);

      		MPI_Send(topology[j], data_size + 1, MPI_INT, worker_rank, SEND_DATA_TAG, MPI_COMM_WORLD);
      		show_communication_message(level, worker_rank);
    	}
  	}
}

/**
 * Functie in care un proces worker primeste topologia sistemului de la coordonatorul sau. 
 * */
void receive_topology_from_top_level(int **topology_data, int *top_level) {

	MPI_Recv(top_level, 1, MPI_INT, MPI_ANY_SOURCE, SEND_TOP_LEVEL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		int data_size;
		MPI_Recv(&data_size, 1, MPI_INT, *top_level, SEND_DATA_SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if (!data_size) {
			continue;
		}
		topology_data[i] = (int *) malloc((data_size + 1) * sizeof(int));
		MPI_Recv(topology_data[i], data_size + 1, MPI_INT, *top_level, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
}

/**
 * Functie destinata procesului 0 care initializeaza vectorul rezultat si trimite task-uri proceselor
 * worker din cadrul clusterului sau.
 * */
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

/**
 * Functie in care un proces worker priemste vectorul rezultat de la coordonatorul sau impreuna cu 
 * intervalul pe care il are de calculat
 * */
void receive_result_from_top_level(int top_level, int result_size, int *result, int *start, int *end) {

	MPI_Recv(start, 1, MPI_INT, top_level, SEND_START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(end, 1, MPI_INT, top_level, SEND_END_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(result, result_size, MPI_INT, top_level, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}

/**
 * Functie in care un proces worker realizeaza calculul pe intervalul corespunzator si trimite apoi datele
 * inapoi catre coordonatorul sau
 * */
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

/**
 * Functie in care un coordonator primeste vectorul rezultat dupa task-ul finalizat al unui worker
 * */
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

/**
 * Functie in care doi coordonatori comunica intre ei, "source" trimite vectorul rezultat catre "destination"
 * */
void transfer_result(int source, int destination, int *result, int result_size) {

	MPI_Send(result, result_size, MPI_INT, destination, SEND_DATA_TAG, MPI_COMM_WORLD);
	show_communication_message(source, destination);

}

/**
 * Functie in care un proces coordonator priemste vectorul rezultat de la un altul
 * */
void receive_result(int source, int destination, int *result, int result_size) {

	MPI_Recv(result, result_size, MPI_INT, destination, SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}


/**
 * Functie in care un alt proces coordonator diferit de 0 trimite vectorul rezultat catre workerii sai, calculand
 * corespunzator intervalul pe care acesta are sa lucreze. In cazul in care acesta un coordonator este ultimul
 * responsabil sa imparta task-uri, acesta da ultimului worker si restul de interval ramas ca rest la impartire.
 * */
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

/**
 * Functie ce afiseaza rezultatul de catre procesul 0 odata ce este calculat.
 **/ 
void show_final_result(int result_size, int *result) {

	printf("Rezultat: ");
	for (int i = 0; i < result_size; ++i) {
		printf("%d ", result[i]);
	}
	printf("\n");
}

/**
 * Functie ce afiseaza topologia dintr-un anumit proces, fie el coordonator sau worker.
 * */
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

	// Realizam initializarea MPI-urilor
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

	// Obtinem dimensiunea vectorului result, codul de eroare si alocam memorie pentru result.
	int result_size = atoi(argv[1]);
	int error = atoi(argv[2]);
	int *result = (int *) malloc(result_size * sizeof(int));

	// Alocam memorie pentru topologie si initializam fiecare linie cu NULL (va fi alocata cand se primesc
	// date catre acea linie)
	int **topology_data = (int **) malloc(MAXIMUM_TOP_LEVEL * sizeof(int *));
	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		topology_data[i] = NULL;
	}
	
	int *top_level_workers = (int *) malloc(MAXIMUM_LOW_LEVEL * sizeof(int));

	int top_level;
	if (level < MAXIMUM_TOP_LEVEL) {
		// Cand suntem intr-un proces coordonator extragem datele despre clusterul curent
		// din fisier si alocam memorie in topologie pentru aceste date
		extract_data(top_level_workers, level);
		int workers_number = top_level_workers[0];
		topology_data[level] = (int *) malloc((workers_number + 1) * sizeof(int));
		for (int i = 0; i <= workers_number; ++i) {
			topology_data[level][i] = top_level_workers[i];
		}

		if (error == NO_ERROR) {
			// Construim rutele si transferam date despre topologie in cazul in care nu avem eroare de comunicare
			if (level == 0) {
				send_data_to_top_level(0, 1, 0, topology_data);
				send_data_to_top_level(0, 3, 0, topology_data);
				receive_data_from_top_level(0, 3, 3, topology_data);
				receive_data_from_top_level(0, 1, 1, topology_data);
				send_data_to_top_level(0, 3, 1, topology_data);
				send_data_to_top_level(0, 1, 3, topology_data);
				receive_data_from_top_level(0, 1, 2, topology_data);
			}
			else if (level == 1) {
				send_data_to_top_level(1, 0, 1, topology_data);
				send_data_to_top_level(1, 2, 1, topology_data);
				receive_data_from_top_level(1, 0, 0, topology_data);
				receive_data_from_top_level(1, 2, 2, topology_data);
				send_data_to_top_level(1, 0, 2, topology_data);
				send_data_to_top_level(1, 2, 0, topology_data);
				receive_data_from_top_level(1, 0, 3, topology_data);
			} else if (level == 2) {
				send_data_to_top_level(2, 1, 2, topology_data);
				send_data_to_top_level(2, 3, 2, topology_data);
				receive_data_from_top_level(2, 1, 1, topology_data);
				receive_data_from_top_level(2, 3, 3, topology_data);
				receive_data_from_top_level(2, 1, 0, topology_data);
			} else {
				send_data_to_top_level(3, 0, 3, topology_data);
				send_data_to_top_level(3, 2, 3, topology_data);
				receive_data_from_top_level(3, 0, 0, topology_data);
				receive_data_from_top_level(3, 2, 2, topology_data);
				receive_data_from_top_level(3, 0, 1, topology_data);
			}
		}
		else if (error == CONNECTION_ERROR) {
			// Construim rutele si transferam date despre topologie in cazul in care procesul 0 si 1 nu pot comunica
			 if (level == 0) {
				send_data_to_top_level(0, 3, 0, topology_data);
				receive_data_from_top_level(0, 3, 3, topology_data);
				receive_data_from_top_level(0, 3, 2, topology_data);
				receive_data_from_top_level(0, 3, 1, topology_data);
			}
			else if (level == 1) {
				send_data_to_top_level(1, 2, 1, topology_data);
				receive_data_from_top_level(1, 2, 2, topology_data);
				receive_data_from_top_level(1, 2, 3, topology_data);
				receive_data_from_top_level(1, 2, 0, topology_data);
			} else if (level == 2) {
				send_data_to_top_level(2, 1, 2, topology_data);
				send_data_to_top_level(2, 3, 2, topology_data);
				receive_data_from_top_level(2, 1, 1, topology_data);
				receive_data_from_top_level(2, 3, 3, topology_data);
				send_data_to_top_level(2, 3, 1, topology_data);
				send_data_to_top_level(2, 1, 3, topology_data);
				receive_data_from_top_level(2, 3, 0, topology_data);
				send_data_to_top_level(2, 1, 0, topology_data);
			} else {
				send_data_to_top_level(3, 0, 3, topology_data);
				send_data_to_top_level(3, 2, 3, topology_data);
				receive_data_from_top_level(3, 0, 0, topology_data);
				receive_data_from_top_level(3, 2, 2, topology_data);
				receive_data_from_top_level(3, 2, 1, topology_data);
				send_data_to_top_level(3, 0, 2, topology_data);
				send_data_to_top_level(3, 0, 1, topology_data);
				send_data_to_top_level(3, 2, 0, topology_data);
			}

		} else {
			// Construim rutele si transferam date despre topologie, ignorand complet procesul 1
			if (level == 0) {
				send_data_to_top_level(0, 3, 0, topology_data);
				receive_data_from_top_level(0, 3, 3, topology_data);
				receive_data_from_top_level(0, 3, 2, topology_data);
			}
			else if (level == 2) {
				send_data_to_top_level(2, 3, 2, topology_data);
				receive_data_from_top_level(2, 3, 3, topology_data);
				receive_data_from_top_level(2, 3, 0, topology_data);
			} 
			else if (level == 3) {
				send_data_to_top_level(3, 0, 3, topology_data);
				send_data_to_top_level(3, 2, 3, topology_data);
				receive_data_from_top_level(3, 0, 0, topology_data);
				receive_data_from_top_level(3, 2, 2, topology_data);
				send_data_to_top_level(3, 0, 2, topology_data);
				send_data_to_top_level(3, 2, 0, topology_data);
			}
		}
		// In acest moment coordonatorul are intreaga topologie si o poate trimite workerilor sai
		send_topology_to_workers(topology_data, level);
	}
	// Daca avem un proces worker acesta va primi topologia de la coordonatorul sau
	if (level >= MAXIMUM_TOP_LEVEL) {
		receive_topology_from_top_level(topology_data, &top_level);
	}
	
	// Toate procesele cunosc topologia asa ca este afisata de fiecare proces in parte
	write_topology(topology_data, level);

	// Calculam numarul total de workeri din topologie pentru a putea face distribuirea 
	// corecta a task-urilor catre workeri
	int total_workers = 0;
	for (int i = 0; i < MAXIMUM_TOP_LEVEL; ++i) {
		if (topology_data[i] == NULL) {
			continue;
		}
		total_workers += topology_data[i][0];
	}

	if (level == TOP_LEVEL) {

		// Procesul 0 initializeaza vectorul result si trimite task-uri workerilor sai 
		init_and_send_work(result, result_size, topology_data[level], total_workers);
		// Primeste rezultatul de la acestia
		receive_result_from_workers(level, topology_data[level], result, result_size);
		// Transfera rezultatul mai departe catre procesul 3
		transfer_result(level, 3, result, result_size);
		// In cazul unei erori de comunicare cu procesul 1 acesta primeste rezultatul pe ruta pe care a plecat, daca nu, 
		// prin intermediul procesului 1
		if (error != NO_ERROR) {
			receive_result(level, 3, result, result_size);
			show_final_result(result_size, result);
		}
		else {
			receive_result(level, 1, result, result_size);
			show_final_result(result_size, result);
		}
		
	}
	else if (level < MAXIMUM_TOP_LEVEL) {

		if (level == 3) {
			// Pentru procesul 3 acesta priemste rezultatul de la procesul 0, trimite task-uri catre workeri si il transfera
			// apoi mai departe catre 2. Apoi, in functie de prezenta unei erori de comunicare sau partitionare, asteapta fie 
			// rezultatul inapoi pentru a-l da mai departe lui 0 ori nu mai asteapta date.
			if (error == NO_ERROR) {
				receive_result(level, TOP_LEVEL, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 0);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 2, result, result_size);
			}
			else if (error == CONNECTION_ERROR || error == PARTITION_ERROR) {
				receive_result(level, TOP_LEVEL, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 0);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 2, result, result_size);

				receive_result(level, 2, result, result_size);
				transfer_result(level, 0, result, result_size);

			}
		}

		if (level == 2) {
			// La fel ca procesul 3, acesta trimite mai departe in lantul normal al rutei si asteapta raspuns inapoi de la 
			// 1 daca avem eroare de comunicare.
			if (error == NO_ERROR) {
				receive_result(level, 3, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 0);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 1, result, result_size);
			} else if (error == CONNECTION_ERROR) {
				receive_result(level, 3, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 0);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 1, result, result_size);

				receive_result(level, 1, result, result_size);
				transfer_result(level, 3, result, result_size);

			} else {
				receive_result(level, 3, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 1);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 3, result, result_size);
			}
		}

		if (level == 1) {
			// Procesul 1 procedeaza asemenator, doar ca in cazul unei erori de partitionari acesta nu mai este prezent
			// in realizarea calulelor.
			if (error == NO_ERROR) {
				receive_result(level, 2, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 1);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 0, result, result_size);
			} else if (error == CONNECTION_ERROR) {
				receive_result(level, 2, result, result_size);
				transfer_result_to_workers(level, result, result_size, topology_data, total_workers, 1);
				receive_result_from_workers(level, topology_data[level], result, result_size);
				transfer_result(level, 2, result, result_size);
			}
		}
	}
	else {

		// Procesele worker asteaapta acum sa primeasca task-uri, sa le rezolve si sa le trimita inapoi
		int start, end;
		if (error != PARTITION_ERROR || top_level != 1) {
			receive_result_from_top_level(top_level, result_size, result, &start, &end);
			calculate_and_send_result_chunk(level, top_level, result_size, result, start, end);
		}
	}

 
    MPI_Finalize();

	return EXIT_SUCCESS;
}