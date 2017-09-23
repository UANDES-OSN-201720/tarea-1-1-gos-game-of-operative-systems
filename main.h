#ifndef MAIN
#define MAIN

int* getCommand();
void executeQuitCommand(int* pidArray, int* pidArrayCounter);
int* splitCommand(char** commandBuf);
int* parseCommandArguments(char* commandBuf);
void killOffice(int officeId, int* pidArray, int* pidArrayCounter, int* pidAccountNumberArray, int* pidTerminalNumberArray);
void killChild(int pid);
void* asyncTransactionBroadcast(void* argunemts);
void* asyncPostTransaction(void* arguments);
void* asyncListenTransactions(void* arguments);
char* intToString(int pid);
void broadcastFromPipe(void* arguments);
void broadcastDumpCommand(const int bankPID, int* childPID, int* toBankPipe);
void broadcastDumpAccsCommand(const int bankPID, int* childPID, int* toBankPipe);
void broadcastDumpErrsCommand(const int bankPID, int* childPID, int* toBankPipe);
char* generateRandomTransaction(int sourcePid, int destinationPid);
char* messageToString(long long int message);

// Message parsing functions
struct messageData;

int shouldExecuteMessage(int destinationPid, int currentPid);

int parseNumericMessage(struct messageData *parsedMessage, long long int numericMessage);
int getSourcePid(long long int message);
int getSourceAccount(long long int message);
int getDestinationPid(long long int message);
int getDestinationAccount(long long int message);
int getTransactionAmount(long long int message);
int getOperationCommand(long long int message);

char* executeMessageOperation(struct messageData* parsedMessage, int* accountsArray, char** errorsArray, int** transactionsArray);

void storeTransacction(int** transactionsArray, struct messageData *parsedMessage);
int is_transaction_empty(int* transaction);

void generateDumpRequest(int pidBank, int pidOffice, int dumpCode, int* toBankPipe);
char* parseFileName(char* rawFileName, int officePID);
void logError(char** errorsArray, char* errorMessage);

#endif
