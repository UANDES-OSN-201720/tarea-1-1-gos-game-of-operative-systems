#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "main.h"


const int QUIT = 0;
const int INIT = 1;
const int KILL = 2;
const int LIST = 3;
const int DUMP = 4;
const int DUMP_ACCS = 5;
const int DUMP_ERRS = 6;

const int READ = 0;
const int WRITE = 1;

const int TOTAL_OFFICES = 128;

struct arg_struct {
    int *arg1;
    int *arg2;
    int *arg3;
    int **arg4;
};

int main(int argc, char** argv) {
    int *pidArray;
    int pidArrayCounter = 0;
    pidArray = malloc(sizeof(int)*128);

    int toBankPipe[2];
    pipe(toBankPipe);

    // Initialize all child pipes
    int** childPipes = malloc(sizeof(int*) * TOTAL_OFFICES);
    for( int pipe = 0; pipe < TOTAL_OFFICES; pipe++) {
        childPipes[pipe] = malloc(sizeof(int) * 2);
    }
    int currentChild = 0;

    const int bankId = getpid();
    printf("Bienvenido a Banco '%d'\n", bankId);

    struct arg_struct args;
    args.arg1 = &currentChild;
    args.arg2 = toBankPipe;
    args.arg4 = childPipes;

    // Thread for reading toBankPipe and broadcast the message to every child pipe
    pthread_t transactionBroadcastThread;
    int transactionsBroadcastDisabled = pthread_create(&transactionBroadcastThread, NULL, asyncTransactionBroadcast, &args);
    if (transactionsBroadcastDisabled){
        printf("Error creating transaction broadcast thread. Consider killing the program.\n");
    }

    while (true) {
        int* command = getCommand();

        if (command[0] == QUIT) {
            executeQuitCommand(pidArray, &pidArrayCounter);
            break;

        } else if (command[0] == LIST) {
            printf("Lista de sucursales: \n");
            for (int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
                printf("Sucursal almacenada con ID '%d'\n", pidArray[sucIndex]);
                // TODO: Missing accounts amount for every office.
            }

        } else if (command[0] == KILL) {
            killOffice(command[1], pidArray, &pidArrayCounter);

        } else if (command[0] == INIT) {

            //Create parent to child pipe
            pipe(childPipes[currentChild]);
            currentChild = currentChild + 1;

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
                accountsArray[0] = 1000;

                struct arg_struct args;
                args.arg1 = accountsArray;
                //TODO: Change pipe to have unique child-parent pipes
                args.arg2 = toBankPipe;
                args.arg3 = childPipes[currentChild - 1];

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
            // Broadcast to every toBankPipe

        }  else if (command[0] == DUMP_ACCS) {
            //int childPID = command[1];
            // TODO: Generate accounts statuses CSV
            // Broadcast to every toBankPipe

        }  else if (command[0] == DUMP_ERRS) {
            //int childPID = command[1];
            // TODO: Generate transactions errors CSV
            // Broadcast to every toBankPipe

        } else {
            fprintf(stderr, "Comando no reconocido.\n");
        }
    }

    printf("Terminando ejecucion limpiamente...\n");

    free(pidArray);

    return(EXIT_SUCCESS);
}

int* getCommand() {
    size_t bufsize = 512;
    char* commandBuf = malloc(sizeof(char)*bufsize);

    printf(">>");
    getline(&commandBuf, &bufsize, stdin);

    // Manera de eliminar el \n leido por getline
    commandBuf[strlen(commandBuf)-1] = '\0';

    return splitCommand(&commandBuf);
}

void executeQuitCommand(int* pidArray, int *pidArrayCounter){
    for(int sucIndex = 0; sucIndex < *pidArrayCounter; sucIndex++){
        int childPID = pidArray[sucIndex];
        killChild(childPID);
    }
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

void killOffice(int officeId,int *pidArray,int *pidArrayCounter ){
    int childPID = officeId;
    killChild(childPID);

    for (int i = 0; i < *pidArrayCounter; i++){
        if (pidArray[i] == childPID){
            printf("%d\n", pidArray[i]);
            printf("%d %d\n", *pidArrayCounter, pidArray[*pidArrayCounter - 1]);
            pidArray[i] = pidArray[*pidArrayCounter - 1];

            printf("%d\n", pidArray[i]);
            *pidArrayCounter -= 1;
        }
    }
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
        printf("CENTRAL: Broadcasting message '%s'\n", readbuffer);

        for (int pipe = 0; pipe < *childPipesIndex; pipe++){
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
        char* msg = intToString(officePID);
        write(toBankPipe[WRITE], msg, (strlen(msg) + 1));
        sleep(10);
    }
}

char* intToString(int pid) {
    char *message = malloc(sizeof(char) * 20);
    sprintf(message, "%d", pid);

    return message;
}

void *asyncListenTransactions(void *arguments) {
    struct arg_struct *args = arguments;
    int officePID = getpid();
    int *accountsArray = args -> arg1;
    // int* toBankPipe = args -> arg2;
    int* toChildPipe = args -> arg3;

    printf("CHILD %d: Async transaction listening initiated\n", officePID);
    while(true){
        char message[80];
        read(toChildPipe[READ], message, sizeof(message));

        char *strOfficePID = intToString(officePID);

        int notMyMessage = strncmp(strOfficePID, message, strlen(strOfficePID));

        if (notMyMessage && strlen(message) < 10) {
            useMessage(&officePID, message, accountsArray);
            /*
            char *response = useMessage(&officePID, message, accountsArray);

            if(*response) {
                printf("CHILD %d: Writing post response to '%p'\n", officePID, (void*)&toBankPipe);
                write(toBankPipe[WRITE], response, sizeof(response));
            }
            */
        }
        printf("CHILD %d: Account 0 amount '%d'\n", officePID, accountsArray[0]);
    }
}

char *useMessage(int *officePID, char *message, int *accountsArray) {
    char *response = NULL;
    const char *DEPOSIT_OP = "00";
    const char *WIDTHRAW_OP = "01";
    const char *DUMP_OP = "10";
    const char *DUMP_ACCS_OP = "11";
    const char *DUMP_ERRS_OP = "12";

    printf("CHILD %d: Received broadcast message '%s'\n", *officePID, message);

    // TODO: Cut the message to fill the following variables
    char *transactionOperation = "01";
    int accountNumber = 20000;
    int operationAmount = 1000;

    if (sizeof(accountsArray) < accountNumber){
        response = "ERROR: Account doesn't exist!";
        printf("CHILD %d: error message '%s'\n", *officePID, response);
        return response;
    }

    // Operation 01: Deposit money to accountNumber
    if (strncmp(DEPOSIT_OP, transactionOperation, strlen(DEPOSIT_OP))) {
        printf("CHILD %d: EXECUTING DEPOSIT COMMAND\n", *officePID);
        accountsArray[accountNumber] += operationAmount;

    // Operation 01: Widthraw money from accountNumber
    } else if (strncmp(WIDTHRAW_OP, transactionOperation, strlen(WIDTHRAW_OP))) {
        printf("CHILD %d: EXECUTING WIDTHRAW COMMAND\n", *officePID);

        if (accountsArray[accountNumber] >= operationAmount){
            accountsArray[accountNumber] -= operationAmount;
            return NULL;
        } else {
            response = "ERROR: Account doesn't have enough money!";
            printf("CHILD %d: error message '%s'\n", *officePID, response);
            return response;
        }

    // Operation 10: DUMP Command
    } else if (strncmp(DUMP_OP, transactionOperation, strlen(DUMP_OP))) {
        printf("CHILD %d: EXECUTING DUMP COMMAND\n", *officePID);

    // Operation 11: DUMP 1 Command
    } else if (strncmp(DUMP_ACCS_OP, transactionOperation, strlen(DUMP_ACCS_OP))) {
        printf("CHILD %d: EXECUTING DUMP_ACCS COMMAND\n", *officePID);

    // Operation 12: DUMP 2 Command
    } else if (strncmp(DUMP_ERRS_OP, transactionOperation, strlen(DUMP_ERRS_OP))) {
        printf("CHILD %d: EXECUTING DUMP_ERRS COMMAND\n", *officePID);
    }

    response = "Responding message to sender apparently...";
    return response;
}
