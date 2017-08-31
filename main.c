#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

// Cuenten con este codigo monolitico en una funcion
// main como punto de partida.
// Idealmente, el codigo del programa deberia estar
// adecuadamente modularizado en distintas funciones,
// e incluso en archivos separados, con dependencias
// distribuidas en headers. Pueden modificar el Makefile
// libremente para lograr esto.
const int QUIT = 0;
const int INIT = 1;
const int KILL = 2;
const int LIST = 3;
const int DUMP = 4;
const int DUMP_ACCS = 5;
const int DUMP_ERRS = 6;

int* splitCommand(char** commandBuf);
void killChild(int pid);
int get_int(char *string);
void *asyncRequestTransaction(void *voidOfficePID);
void *asyncResponseTransaction(void *voidOfficePID);

int main(int argc, char** argv) {
  size_t bufsize = 512;
  char* commandBuf = malloc(sizeof(char)*bufsize);
  int pidArray[10];
  int pidArrayCounter = 0;
  // Para guardar descriptores de pipe
  // el elemento 0 es para lectura
  // y el elemento 1 es para escritura.
  int bankPipe[2];
  char readbuffer[80]; // buffer para lectura desde pipe

  // Se crea un pipe...
  pipe(bankPipe);

  const int bankId = getpid() % 1000;
  printf("Bienvenido a Banco '%d'\n", bankId);

  while (true) {
    printf(">>");
    getline(&commandBuf, &bufsize, stdin);

    // Manera de eliminar el \n leido por getline
    commandBuf[strlen(commandBuf)-1] = '\0';

    int* command = splitCommand(&commandBuf);

    printf("Comando ingresado: '%d'\n", command[0]);

    if (command[0] == QUIT) {
      for(int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
        int childPID = pidArray[sucIndex];
        killChild(childPID);
      }
      break;

    } else if (command[0] == LIST) {
      printf("Lista de sucursales: \n");
      for(int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
        printf("Sucursal almacenada con ID '%d'\n", pidArray[sucIndex]);
        // TODO: Missing accounts amount for every office.

      }

    } else if (command[0] == KILL) {
      int childPID = command[1];
      killChild(childPID);

      printf("Sucursal %lu cerrada\n", childPID);
    } else if (command[0] == INIT) {
      // OJO: Llamar a fork dentro de un ciclo
      // es potencialmente peligroso, dado que accidentalmente
      // pueden iniciarse procesos sin control.
      // Buscar en Google "fork bomb"
      pid_t sucid = fork();

      if (sucid > 0) {
        printf("Sucursal creada con ID '%d'\n", sucid);

        pidArray[pidArrayCounter] = sucid;
        pidArrayCounter = pidArrayCounter + 1;

        // Enviando saludo a la sucursal
        char msg[] = "Hola sucursal, como estas?";
        write(bankPipe[1], msg, (strlen(msg)+1));

        continue;
      } else if (!sucid) {
        int officeId = getpid();
        int accountAmount = command[1];
        int accountsArray[accountAmount];

        int* availableOffices[128];

        // Create transactionRequest thread
        pthread_t transactionRequestThread;
        int transactionsRequestsDisabled = pthread_create(&transactionRequestThread, NULL, asyncRequestTransaction, &officeId);
        if (transactionsRequestsDisabled){
          printf("Error creating transactions request thread. Consider killing the office.\n");
        }

        // Create transactionResponse thread
        pthread_t transactionResponseThread;
        int transactionsResponsesDisabled = pthread_create(&transactionResponseThread, NULL, asyncResponseTransaction, &officeId);
        if (transactionsResponsesDisabled){
          printf("Error creating transactions response thread. Consider killing the office.\n");
        }

        // This while prevents the process from being killed
        while(true){
        }

        _exit(EXIT_SUCCESS);

      } else {
        fprintf(stderr, "Error al crear proceso de sucursal!\n");
        return (EXIT_FAILURE);

      }
    } else if (command[0] == DUMP) {
      int childPID = command[1];
      // TODO: Generate transactions CSV

    }  else if (command[0] == DUMP_ACCS) {
      int childPID = command[1];
      // TODO: Generate accounts statuses CSV

    }  else if (command[0] == DUMP_ERRS) {
      int childPID = command[1];
      // TODO: Generate transactions errors CSV

    } else {
      fprintf(stderr, "Comando no reconocido.\n");
    }
  }

  printf("Terminando ejecucion limpiamente...\n");
  // Cerrar lado de escritura del pipe
  close(bankPipe[1]);

  return(EXIT_SUCCESS);
}

int* splitCommand(char** commandBuf){
  printf("Comando a hacer split '%s'\n", *commandBuf);

  static int output[2];

  if (!strncmp("quit", *commandBuf, strlen("quit"))){

    output[0] = QUIT;
    output[1] = 33;

    return output;
  } else if (!strncmp("init", *commandBuf, strlen("init"))){

    output[0] = INIT;
    output[1] = 33;

    return output;
  } else if (!strncmp("kill", *commandBuf, strlen("kill"))){

    output[0] = KILL;
    output[1] = getPid(*commandBuf);

    return output;
  } else if (!strncmp("list", *commandBuf, strlen("list"))){

    output[0] = LIST;
    output[1] = 33;

    return output;
  } else if (!strncmp("dump", *commandBuf, strlen("dump"))){

    output[0] = DUMP;

    output[1] = getPid(*commandBuf);

    return output;
  } else if (!strncmp("dump_accs", *commandBuf, strlen("dump_accs"))){

    output[0] = DUMP_ACCS;
    output[1] = getPid(*commandBuf);

    return output;
  } else if (!strncmp("dump_errs", *commandBuf, strlen("dump_errs"))){

    output[0] = DUMP_ERRS;
    output[1] = getPid(*commandBuf);

    return output;
  }

  output[0] = -1;
  output[1] = -1;

  return output;
}

void killChild(int pid){
  kill(pid, SIGTERM);
  printf("Matando hijo pid:'%d'\n", pid);

  bool died = false;
  for (int loop = 0; !died && loop < 5; loop++){
    int status;
    pid_t id;
    sleep(1);
    if (waitpid(pid, &status, WNOHANG) == pid) died = true;
  }

  if (!died) kill(pid, SIGKILL);
}

int getPid(char *commandBuf){
	char *command = commandBuf;
	char *str_pid;
	int i = 0, j, pid;
	while (command[i] != ' '){
		i++;
	}
	i++;
	j = i;
	while (command[j] != '\0'){
		j++;
	}
	str_pid = malloc(sizeof(char)*(j-i+1));
	for (int k=i; k<j; k++){
		str_pid[k-i] = command[k];
	}
	str_pid[j] = '\0';

	pid = atoi(str_pid);
	free(str_pid);

	return pid;
}
void *asyncRequestTransaction(void *voidOfficePID) {
  int *officePID = voidOfficePID;

  while(true){
    printf("Async transaction request initiated from pid: '%d'\n", *officePID);
    sleep(1);
  }
}

void *asyncResponseTransaction(void *voidOfficePID) {
    int *officePID = voidOfficePID;

    while(true){
      printf("Async transaction response initiated from pid: '%d'\n", *officePID);
      sleep(1);
    }
}
