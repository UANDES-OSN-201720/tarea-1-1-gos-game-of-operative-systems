#ifndef MAIN
#define MAIN

int* getCommand();
int* splitCommand(char** commandBuf);
int parseCommandArguments(char *commandBuf);
void killOffice(int officeId,int **pidArray,int *pidArrayCounter);
void killChild(int pid);
void *asyncTransactionBroadcast(void *argunemts);
void *asyncPostTransaction(void *arguments);
void *asyncListenTransactions(void *arguments);
char* generateMessage(int pid);

#endif
