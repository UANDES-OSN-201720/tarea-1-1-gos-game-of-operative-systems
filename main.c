#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include "main.h"

const int QUIT = 0;
const int INIT = 1;
const int KILL = 2;
const int LIST = 3;
const int DUMP = 4;
const int DUMP_ACCS = 5;
const int DUMP_ERRS = 6;

const int DEPOSIT_OP = 0;
const int WIDTHRAW_OP = 1;
const int DUMP_OP = 2;
const int DUMP_ACCS_OP = 3;
const int DUMP_ERRS_OP = 4;

const int READ = 0;
const int WRITE = 1;

const int EMPTY_TRANSACTION = -1;
const char* EMPTY_ERROR = "-";

const int TOTAL_OFFICES = 128;
const int TRANSACTIONS_AMOUNT = 1000;

const int DEVELOPMENT = 1;

struct arg_struct {
    int* childsAmount;
    int* toBankPipe;
    int* toChildPipe;
    int** toChildPipes;
    int** toBankPipes;
    int* accounts;
    int* transactions;
    int* officesPID;
    char** errors;
};

int main(int argc, char** argv) {
    int* pidArray;
    int pidArrayCounter = 0;
    pidArray = malloc(sizeof(int) * 128);

    // Initialize all child-bank and bank-child pipes
    int** toBankPipes = malloc(sizeof(int*) * TOTAL_OFFICES);
    int** toChildPipes = malloc(sizeof(int*) * TOTAL_OFFICES);

    for( int pipe = 0; pipe < TOTAL_OFFICES; pipe++) {
        toChildPipes[pipe] = malloc(sizeof(int) * 2);
        toBankPipes[pipe] = malloc(sizeof(int) * 2);
    }
    int currentChild = 0;

    const int bankId = getpid();
    printf("Bienvenido a Banco '%d'\n", bankId);

    struct arg_struct threadArguments;
    threadArguments.childsAmount = &currentChild;
    threadArguments.toChildPipes = toChildPipes;
    threadArguments.toBankPipes = toBankPipes;

    // Thread for reading toBankPipe and broadcast the message to every child pipe
    pthread_t transactionBroadcastThread;
    int transactionsBroadcastDisabled = pthread_create(&transactionBroadcastThread, NULL, asyncTransactionBroadcast, &threadArguments);
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
                printf("Sucursal almacenada con ID '%d'\n", pidArray[sucIndex] % 1000);
                // TODO: Missing accounts amount for every office.
            }

        } else if (command[0] == KILL) {
            killOffice(command[1], pidArray, &pidArrayCounter);

        } else if (command[0] == INIT) {
            // Create child to parent pipe
            pipe(toBankPipes[currentChild]);
            //Create parent to child pipe
            pipe(toChildPipes[currentChild]);
            currentChild = currentChild + 1;

            pid_t sucid = fork();

            if (sucid > 0) {
                printf("Sucursal creada con ID '%d'\n", sucid % 1000);

                pidArray[pidArrayCounter] = sucid;
                pidArrayCounter = pidArrayCounter + 1;

                continue;
            } else if (!sucid) {

                int accountAmount = command[1];
                int accountsArray[accountAmount];
                for (int account = 0; account < accountAmount; account++) {
                    srand(time(NULL)+account);
                    accountsArray[account] = rand()%490000000 +1000;
                    //printf("Generando montos para las cuentas! %d\n", accountsArray[account]);
                }

                int transactionsArray[TRANSACTIONS_AMOUNT];
                for (int transaction = 0; transaction < TRANSACTIONS_AMOUNT; transaction++) {
                    transactionsArray[transaction] = EMPTY_TRANSACTION;
                }

                char** errorsArray = malloc(sizeof(char) * 80);
                for (int error = 0; error < 1000; error++) {
                    errorsArray[error] = "-";
                }

                accountsArray[0] = 1000;

                struct arg_struct threadArguments;
                threadArguments.toBankPipe = toBankPipes[currentChild - 1];
                threadArguments.toChildPipe = toChildPipes[currentChild - 1];
                threadArguments.childsAmount = &currentChild;
                threadArguments.accounts = accountsArray;
                threadArguments.transactions = transactionsArray;
                threadArguments.officesPID = pidArray;
                threadArguments.errors = errorsArray;

                // Create transactionRequest thread
                pthread_t transactionRequestThread;
                int transactionsRequestsDisabled = pthread_create(&transactionRequestThread, NULL, asyncPostTransaction, &threadArguments);
                if (transactionsRequestsDisabled){
                    printf("Error creating transactions request thread. Consider killing the office.\n");
                }

                // Create transactionResponse thread
                pthread_t transactionResponseThread;
                int transactionsResponsesDisabled = pthread_create(&transactionResponseThread, NULL, asyncListenTransactions, &threadArguments);
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
            // TODO: Filter in case childPID is empty or doesn't exist
            if (currentChild > 0) {
                broadcastDumpCommand(bankId, &childPID, toBankPipes[0]);
            } else {
                printf("CENTRAL: Aún no hay sucursales creadas.\n");
            }

        }  else if (command[0] == DUMP_ACCS) {
            int childPID = command[1];
            // TODO: Filter in case childPID is empty or doesn't exist
            if (currentChild > 0) {
                broadcastDumpAccsCommand(bankId, &childPID, toBankPipes[0]);
            } else {
                printf("CENTRAL: Aún no hay sucursales creadas.\n");
            }

        }  else if (command[0] == DUMP_ERRS) {
            int childPID = command[1];
            // TODO: Filter in case childPID is empty or doesn't exist
            if (currentChild > 0) {
                broadcastDumpErrsCommand(bankId, &childPID, toBankPipes[0]);
            } else {
                printf("CENTRAL: Aún no hay sucursales creadas.\n");
            }

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

void executeQuitCommand(int* pidArray, int* pidArrayCounter){
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

    } else if (!strncmp("dump_accs", *commandBuf, strlen("dump_accs"))){

        output[0] = DUMP_ACCS;

    } else if (!strncmp("dump_errs", *commandBuf, strlen("dump_errs"))){

        output[0] = DUMP_ERRS;
    } else if (!strncmp("dump", *commandBuf, strlen("dump"))){

        output[0] = DUMP;

    }
    return output;
}

int parseCommandArguments(char* commandBuf){
    char* command = commandBuf;
    char* str_number;
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

void killOffice(int officeId,int* pidArray,int* pidArrayCounter ){
    int childPID = officeId;
    killChild(childPID);

    for (int i = 0; i < *pidArrayCounter; i++){
        if (pidArray[i] == childPID){
            pidArray[i] = pidArray[*pidArrayCounter - 1];
            *pidArrayCounter -= 1;
        }
    }
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

void* asyncTransactionBroadcast(void* arguments){
    struct arg_struct* threadArguments = arguments;
    int* childsAmount = threadArguments -> childsAmount;
    int** toChildPipes = threadArguments -> toChildPipes;
    int** toBankPipes = threadArguments -> toBankPipes;

    if(DEVELOPMENT) {
        printf("CENTRAL: Async transaction broadcast initiated.\n");
    }
    while(true){
        for (int toBankPipe = 0; toBankPipe < *childsAmount; toBankPipe++) {

            struct arg_struct broadcastArguments;
            broadcastArguments.childsAmount = childsAmount;
            broadcastArguments.toChildPipes = toChildPipes;
            broadcastArguments.toBankPipe = toBankPipes[toBankPipe];

            broadcastFromPipe(&broadcastArguments);
        }
    }
}

void broadcastFromPipe(void* arguments) {
    struct arg_struct* threadArguments = arguments;

    int* childsAmount = threadArguments -> childsAmount;
    int* fromPipe = threadArguments -> toBankPipe;
    int** toChildPipes = threadArguments -> toChildPipes;

    char readbuffer[80];
    read(fromPipe[READ], readbuffer, sizeof(readbuffer));
    if(DEVELOPMENT) {
        printf("\nCENTRAL: Broadcasting message '%s'\n", readbuffer);
    }

    for (int pipe = 0; pipe < *childsAmount; pipe++){
        write(toChildPipes[pipe][WRITE], readbuffer, sizeof(readbuffer));
    }
}

void* asyncPostTransaction(void* arguments) {
    struct arg_struct* threadArguments = arguments;
    int officePID = getpid();
    int* childsAmount = threadArguments -> childsAmount;
    int* toBankPipe = threadArguments -> toBankPipe;
    int* officesPID = threadArguments -> officesPID;
    if(DEVELOPMENT) {
        printf("CHILD '%d': Async transaction post initiated.\n", officePID % 1000);
    }
    while(true){
        srand(time(NULL));
        int officePIDindex = rand() % (childsAmount[0]); // puede que haya que poner un -1
        int officePIDtransaction = officesPID[officePIDindex];
        //printf("office t %d childsAmount %d officePID %d\n", officePIDtransaction, childsAmount[0], officePID);
        char* message = generateTransaction(officePID % 1000, officePIDtransaction % 1000);
        write(toBankPipe[WRITE], message, (strlen(message) + 1));
        sleep(10);
    }
}

char* generateTransaction(int pidBank, int pidOffice){
  sleep(1);
  srand(time(NULL));
  long long int account = rand() % 90 + 10;
  long long int amount = rand() % 90000 + 10000;
  long long int transaction = rand() % 2;
  // printf("%d-%d-%lli-%lli-%lli\n", pidBank, pidOffice, account, amount, transaction);
  long long int final = (((pidBank*1000 + pidOffice)*100 + account)*1000000 + amount)*10 + transaction;
  return messageToString(final);
}

char* intToString(int pid) {
    char* message = malloc(sizeof(char) * 20);
    sprintf(message, "%d", pid);

    return message;
}

void* asyncListenTransactions(void* arguments) {
    struct arg_struct* threadArguments = arguments;
    int officePID = getpid();
    int* accountsArray = threadArguments -> accounts;
    char** errorsArray = threadArguments -> errors;
    int* transactionsArray = threadArguments -> transactions;
    int* toChildPipe = threadArguments -> toChildPipe;

    if(DEVELOPMENT) {
        printf("CHILD %d: Async transaction listening initiated\n", officePID % 1000);
    }
    while(true){
        char message[80];
        read(toChildPipe[READ], message, sizeof(message));

        long long int numericMessage = atoll(message);
        int childPIDM = getChildPID(numericMessage);
        int notMyMessage = childPIDM - officePID % 1000;

        if (!notMyMessage) {
            useMessage(&childPIDM, numericMessage, accountsArray, errorsArray, transactionsArray);
            /*
            char* response = useMessage(&officePID, message, accountsArray);

            if(*response) {
                printf("CHILD %d: Writing post response to '%p'\n", officePID, (void*)&toBankPipe);
                write(toBankPipe[WRITE], response, sizeof(response));
            }
            */
        }
    }
}

int getChildPID(long long int message) {
    return message % 1000000000000 / 1000000000;
}

int getBankPID(long long int message){
  return message / 1000000000000;
}

int getAccount(long long int message){
  return message % 1000000000 / 10000000;
}

int getAmount(long long int message){
    return message % 10000000 / 10;
}

int getTransaction(long long int message){
    return message % 10;
}

char* messageToString(long long int message) {
    char* finalMessage = malloc(sizeof(char) * 20);
    sprintf(finalMessage, "%lli", message);
    return finalMessage;
}

char* useMessage(int* officePID, long long int message, int* accountsArray, char** errorsArray, int* transactionsArray) {
    char* response = NULL;

    if(DEVELOPMENT) {
        printf("CHILD %d: Received broadcast message '%lli'\n", *officePID, message);
    }

    // TODO: Cut the message to fill the following variables
    int transactionOperation = getTransaction(message);
    int accountNumber = getAccount(message);
    int operationAmount = getAmount(message);

    // TODO: Buggy sizeof doesn't give the array length
    int total_accounts = accountNumber;

    // TODO: Keep in mind the exeption when accountNumber is not needed
    if (total_accounts < accountNumber){
        response = "ERROR: Account doesn't exist!";
        logError(errorsArray, response);
        return response;
    }

    // Operation 00: Deposit money to accountNumber
    if (transactionOperation == DEPOSIT_OP) {
        if(DEVELOPMENT) {
            printf("\tCHILD %d: EXECUTING DEPOSIT COMMAND\n", *officePID);
        }
        accountsArray[accountNumber] += operationAmount;

        storeTransacction(transactionsArray, operationAmount);

    // Operation 01: Widthraw money from accountNumber
    } else if (transactionOperation == WIDTHRAW_OP) {
        if(DEVELOPMENT) {
            printf("\tCHILD %d: EXECUTING WIDTHRAW COMMAND\n", *officePID);
        }

        if (accountsArray[accountNumber] >= operationAmount){
            accountsArray[accountNumber] -= operationAmount;

            storeTransacction(transactionsArray, -1 * operationAmount);

            return NULL;
        } else {
            response = "ERROR: Account doesn't have enough money!";
            if(DEVELOPMENT) {
                printf("\tCHILD %d: Error message '%s'\n", *officePID, response);
            }
            logError(errorsArray, response);
            return response;
        }

    // Operation 10: DUMP Command
    } else if (transactionOperation == DUMP_OP) {
        if(DEVELOPMENT) {
            printf("\tCHILD %d: EXECUTING DUMP COMMAND\n", *officePID);
        }
        char* fileName = parseFileName("dump_%d.csv", *officePID);

        FILE *dumpFile = fopen(fileName,"w");
        if (dumpFile != NULL) {

            for (int transaction = 0; transaction < TRANSACTIONS_AMOUNT; transaction++) {
                if (transactionsArray[transaction] != EMPTY_TRANSACTION) {
                    fprintf(dumpFile,"%d,%d\n", transaction, transactionsArray[transaction]);
                }
            }
            fclose(dumpFile);
            printf("\tSucursal %d: Archivo '%s' generado exitosamente!\n", *officePID, fileName);
        } else {
            printf("\tSucursal %d: Se produjo un error al generar el archivo '%s'.\n", *officePID, fileName);
        }

    // Operation 11: DUMP 1 Command
    } else if (transactionOperation == DUMP_ACCS_OP) {
        if(DEVELOPMENT) {
            printf("\tCHILD %d: EXECUTING DUMP_ACCS COMMAND\n", *officePID);
        }
        char* fileName = parseFileName("dump_accs_%d.csv", *officePID);

        FILE *dumpFile = fopen(fileName,"w");
        if (dumpFile != NULL) {

            for (int account = 0; account < total_accounts; account++) {
                fprintf(dumpFile,"%d,%d\n", account, accountsArray[account]);
            }
            fclose(dumpFile);
            printf("\tSucursal %d: Archivo '%s' generado exitosamente!\n", *officePID, fileName);
        } else {
            printf("\tSucursal %d: Se produjo un error al generar el archivo '%s'.\n", *officePID, fileName);
        }

    // Operation 12: DUMP 2 Command
    } else if (transactionOperation == DUMP_ERRS_OP) {
        if(DEVELOPMENT) {
            printf("\tCHILD %d: EXECUTING DUMP_ERRS COMMAND\n", *officePID);
        }
        char* fileName = parseFileName("dump_errs_%d.csv", *officePID);

        FILE *dumpFile = fopen(fileName,"w");
        if (dumpFile != NULL) {

            for (int error = 0; error < 50; error++) {
                if(errorsArray[error] != EMPTY_ERROR) {
                    fprintf(dumpFile,"%s\n", errorsArray[error]);
                }
            }
            fclose(dumpFile);
            printf("\tSucursal %d: Archivo '%s' generado exitosamente!\n", *officePID, fileName);
        } else {
            printf("\tSucursal %d: Se produjo un error al generar el archivo '%s'.\n", *officePID, fileName);
        }
    }

    response = "Responding message to sender apparently...";
    return response;
}

char* parseFileName(char* rawFileName, int officePID) {
    char* finalMessage = malloc(sizeof(char) * 20);
    sprintf(finalMessage, rawFileName, officePID);
    return finalMessage;
}

void storeTransacction(int* transactionsArray, int transactionValue) {
    for (int transaction = 0; transaction < sizeof(transactionsArray); transaction++) {
        if (transactionsArray[transaction] == EMPTY_TRANSACTION) {
            transactionsArray[transaction] = transactionValue;
            break;
        }
    }
}

void logError(char** errorsArray, char* errorMessage){
    for (int error = 0; error < 1000; error++) {
        if (errorsArray[error] == EMPTY_ERROR) {
            errorsArray[error] = errorMessage;
            break;
        }
    }
}

void broadcastDumpCommand(const int bankPID, int* childPID, int* toBankPipe) {
    generateDumpRequest(bankPID % 1000, *childPID % 1000, DUMP_OP, toBankPipe);
}

void broadcastDumpAccsCommand(const int bankPID, int* childPID, int* toBankPipe) {
    generateDumpRequest(bankPID % 1000, *childPID % 1000, DUMP_ACCS_OP, toBankPipe);
}

void broadcastDumpErrsCommand(const int bankPID, int* childPID, int* toBankPipe) {
    generateDumpRequest(bankPID % 1000, *childPID % 1000, DUMP_ERRS_OP, toBankPipe);
}

void generateDumpRequest(int pidBank, int pidOffice, int dumpCode, int* toBankPipe){
  sleep(1);
  srand(time(NULL));
  long long int account = rand() % 90 + 10;
  long long int amount = 10000;
  long long int transaction = dumpCode;
  long long int final = (((pidBank * 1000 + pidOffice) * 100 + account) * 1000000 + amount) * 10 + transaction;
  char* message = messageToString(final);
  write(toBankPipe[WRITE], message, (strlen(message) + 1));
}
