#ifndef MAIN
#define MAIN

int* getCommand();
void executeQuitCommand(int* pidArray, int* pidArrayCounter);
int* splitCommand(char** commandBuf);
int* parseCommandArguments(char* commandBuf);
void killOffice(int officeId,int* pidArray,int* pidArrayCounter);
void killChild(int pid);
void* asyncTransactionBroadcast(void* argunemts);
void* asyncPostTransaction(void* arguments);
void* asyncListenTransactions(void* arguments);
char* intToString(int pid);
char* useMessage(int* officePID, long long int message, int* accountsArray, char** errorsArray, int* transactionsArray);
void storeTransacction(int* transactionsArray, int transactionValue);
void broadcastFromPipe(void* arguments);
void broadcastDumpCommand(const int bankPID, int* childPID, int* toBankPipe);
void broadcastDumpAccsCommand(const int bankPID, int* childPID, int* toBankPipe);
void broadcastDumpErrsCommand(const int bankPID, int* childPID, int* toBankPipe);
char* generateTransaction(int pidBank, int pidOffice);
char* messageToString(long long int message);
int getChildPID(long long int message);
void generateDumpRequest(int pidBank, int pidOffice, int dumpCode, int* toBankPipe);
char* parseFileName(char* rawFileName, int officePID);
void logError(char** errorsArray, char* errorMessage);

#endif
