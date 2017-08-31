#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>


const int QUIT = 0;
const int INIT = 1;
const int KILL = 2;
const int LIST = 3;
const int DUMP = 4;
const int DUMP_ACCS = 5;
const int DUMP_ERRS = 6;

const int READ = 0;
const int WRITE = 1;

int* splitCommand(char** commandBuf);
int parseCommandArguments(char *commandBuf);
void killOffice(int officeId,int **pidArray,int *pidArrayCounter);
void killChild(int pid);
void *asyncTransactionBroadcast(void *argunemts);
void *asyncPostTransaction(void *arguments);
void *asyncListenTransactions(void *arguments);

struct arg_struct {
    int *arg1;
    int* arg2;
    int* arg3;
    int** arg4;
};

int main(int argc, char** argv) {
  size_t bufsize = 512;
  char* commandBuf = malloc(sizeof(char)*bufsize);
  int *pidArray;
  int pidArrayCounter = 0;
  pidArray = malloc(sizeof(int)*128);


  // Para guardar descriptores de pipe
  // el elemento 0 es para lectura
  // y el elemento 1 es para escritura.
  int toBankPipe[2];
  pipe(toBankPipe);

  int* childPipes[128];
  int childPipesIndex = 0;

  const int bankId = getpid();
  printf("Bienvenido a Banco '%d'\n", bankId);

  struct arg_struct args;
  args.arg1 = &childPipesIndex;
  args.arg2 = toBankPipe;
  args.arg4 = childPipes;

  // Thread for reading toBankPipe and broadcast the message to every child pipe
  pthread_t transactionBroadcastThread;
  int transactionsBroadcastDisabled = pthread_create(&transactionBroadcastThread, NULL, asyncTransactionBroadcast, &args);
  if (transactionsBroadcastDisabled){
    printf("Error creating transaction broadcast thread. Consider killing the program.\n");
  }

  while (true) {
    printf(">>");
    getline(&commandBuf, &bufsize, stdin);

    // Manera de eliminar el \n leido por getline
    commandBuf[strlen(commandBuf)-1] = '\0';

    int* command = splitCommand(&commandBuf);

    if (command[0] == QUIT) {
        for(int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
          int childPID = pidArray[sucIndex];
          killChild(childPID);
        }
        break;

    } else if (command[0] == LIST) {
      printf("Lista de sucursales: \n");
      for (int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
        printf("Sucursal almacenada con ID '%d'\n", pidArray[sucIndex]);
        // TODO: Missing accounts amount for every office.

      }

    } else if (command[0] == KILL) {
      killOffice(command[1], &pidArray, &pidArrayCounter);
      }

     else if (command[0] == INIT) {

      //Create parent to child pipe
      int toChildPipe[2];
      pipe(toChildPipe);

      childPipes[childPipesIndex] = toChildPipe;
      childPipesIndex = childPipesIndex + 1;

      pid_t sucid = fork();

      if (sucid > 0) {
        printf("Sucursal creada con ID '%d'\n", sucid);

        pidArray[pidArrayCounter] = sucid;
        pidArrayCounter = pidArrayCounter + 1;

        continue;
      } else if (!sucid) {

        int officeId = getpid();
        int accountAmount = command[1];
        int accountsArray[accountAmount];

        printf("Sucursal '%d' creada.\n", officeId);
        accountsArray[0] = 10;

        struct arg_struct args;
        args.arg2 = toBankPipe;
        args.arg3 = toChildPipe;

        // Create transactionRequest thread
        pthread_t transactionRequestThread;
        int transactionsRequestsDisabled = pthread_create(&transactionRequestThread, NULL, asyncPostTransaction, &args);
        if (transactionsRequestsDisabled){
          printf("Error creating transactions request thread. Consider killing the office.\n");
        }

        // Create transactionResponse thread
        pthread_t transactionResponseThread;
        int transactionsResponsesDisabled = pthread_create(&transactionResponseThread, NULL, asyncListenTransactions, &args);
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
      //int childPID = command[1];
      // TODO: Generate transactions CSV

    }  else if (command[0] == DUMP_ACCS) {
      //int childPID = command[1];
      // TODO: Generate accounts statuses CSV

    }  else if (command[0] == DUMP_ERRS) {
      //int childPID = command[1];
      // TODO: Generate transactions errors CSV

    } else {
      fprintf(stderr, "Comando no reconocido.\n");
    }
  }

  printf("Terminando ejecucion limpiamente...\n");

  free(pidArray);

  return(EXIT_SUCCESS);
}

int* splitCommand(char** commandBuf){
  static int output[] = {-1, -1};
  output[1] = parseCommandArguments(*commandBuf);

  if (!strncmp("quit", *commandBuf, strlen("quit"))){

    output[0] = QUIT;

  } else if (!strncmp("init", *commandBuf, strlen("init"))){

    output[0] = INIT;

    if (output[1] <= 0){
      output[1] = 1000;
    }

  } else if (!strncmp("kill", *commandBuf, strlen("kill"))){

    output[0] = KILL;

  } else if (!strncmp("list", *commandBuf, strlen("list"))){

    output[0] = LIST;

  } else if (!strncmp("dump", *commandBuf, strlen("dump"))){

    output[0] = DUMP;

  } else if (!strncmp("dump_accs", *commandBuf, strlen("dump_accs"))){

    output[0] = DUMP_ACCS;

  } else if (!strncmp("dump_errs", *commandBuf, strlen("dump_errs"))){

    output[0] = DUMP_ERRS;
  }

  return output;
}

int parseCommandArguments(char *commandBuf){
	char *command = commandBuf;
	char *str_number;
	int last_letter_index = 0, last_number_index, number;
	while (command[last_letter_index] != ' ' && command[last_letter_index] != '\0'){
		last_letter_index++;
	}
	last_letter_index++;
  if (command[last_letter_index] != '\0'){
    last_number_index = last_letter_index;
  	while (command[last_number_index] != '\0'){
  		last_number_index++;

  	}
    str_number = malloc(sizeof(char)*(last_number_index-last_letter_index + 1));
  	for (int k = last_letter_index; k < last_number_index; k++){
  		str_number[k - last_letter_index] = command[k];
  	}
  	str_number[last_number_index] = '\0';

  	number = atoi(str_number);

  	free(str_number);
    } else {
      number = -1;
    }
	return number;
}

void killOffice(int officeId,int **pidArray,int *pidArrayCounter ){
  int childPID = officeId;
  int *pids = *pidArray;
  killChild(childPID);
  for (int i = 0; i < *pidArrayCounter; i++){
    if (pids[i] == childPID){
      printf("%d\n", pids[i]);
      printf("%d %d\n", *pidArrayCounter,pids[*pidArrayCounter - 1]);
      pids[i] = pids[*pidArrayCounter - 1];

      printf("%d\n", pids[i]);
      *pidArrayCounter -= 1;
    }
  }
  *pidArray = pids;
  printf("%d\n", *pidArrayCounter);
  printf("Sucursal %d cerrada\n", childPID);
}

void killChild(int pid){
  kill(pid, SIGTERM);
  printf("Matando hijo pid:'%d'\n", pid);

  bool died = false;
  for (int loop = 0; !died && loop < 5; loop++){
    int status;
    sleep(1);
    if (waitpid(pid, &status, WNOHANG) == pid) died = true;
  }

  if (!died) kill(pid, SIGKILL);
}

void *asyncTransactionBroadcast(void *arguments){
    struct arg_struct *args = arguments;
    int *childPipesIndex = args -> arg1;
    int* toBankPipe = args -> arg2;
    int** toChildPipes = args -> arg4;


    printf("CENTRAL: Async transaction broadcast initiated.\n");
    while(true){
        char readbuffer[80];
        read(toBankPipe[READ], readbuffer, sizeof(readbuffer));

        // printf("CENTRAL: Received broadcast '%s'\n", readbuffer);
        for (int pipe = 0; pipe < *childPipesIndex; pipe++){

            printf("CENTRAL: Broadcasting to '%p'.\n", (void*)&toChildPipes[pipe]);
            write(toChildPipes[pipe][WRITE], readbuffer, sizeof(readbuffer));
        }
    }
}

void *asyncPostTransaction(void *arguments) {
  struct arg_struct *args = arguments;
  int officePID = getpid();
  int* toBankPipe = args -> arg2;

  printf("CHILD '%d': Async transaction post initiated.\n", officePID);
  while(true){
      char msg[] = "Message from child process and request thread.";
      write(toBankPipe[WRITE], msg, (strlen(msg) + 1));
      sleep(10);
  }
}

void *asyncListenTransactions(void *arguments) {
    struct arg_struct *args = arguments;
    int officePID = getpid();
    int* toBankPipe = args -> arg2;
    int* toChildPipe = args -> arg3;

    printf("CHILD %d: Async transaction listening initiated\n", officePID);
    while(true){

      char readbuffer[80];
      read(toChildPipe[READ], readbuffer, sizeof(readbuffer));

      printf("CHILD %d: pipe address '%p'\n", officePID, (void*)&toChildPipe);
      printf("CHILD %d: Received broadcast '%s'\n", officePID, readbuffer);
      printf("CHILD %d: Should post response to '%p'\n", officePID, (void*)&toBankPipe);
    }
}
