#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

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
    }
    else if (command[0] == LIST){
      printf("Lista de sucursales: \n");
      for(int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
        printf("Sucursal almacenada con ID '%d'\n", pidArray[sucIndex]);
      }
    }
    else if (command[0] == KILL){
      int childPID = command[1];
      killChild(childPID);
      printf("Sucursal %lu cerrada\n", childPID);
    }
    else if (command[0] == INIT) {
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
      }
      // Proceso de sucursal
      else if (!sucid) {
        int sucId = getpid() % 1000;
        int accountAmount = command[1];
        int accountsArray[accountAmount];

        printf("Hola, soy la sucursal '%d'\n", sucId);

        // 100 milisegundos...
        int bytes = read(bankPipe[0], readbuffer, sizeof(readbuffer));
        printf("Soy la sucursal '%d' y me llego mensaje '%s' de '%d' bytes.\n",
               sucId, readbuffer, bytes);

        // Usar usleep para dormir una cantidad de microsegundos
        // usleep(100000);

        // Cerrar lado de lectura del pipe
        close(bankPipe[0]);

        // Para terminar, el proceso hijo debe llamar a _exit,
        // debido a razones documentadas aqui:
        // https://goo.gl/Yxyuxb
        _exit(EXIT_SUCCESS);
      }
      // error
      else {
        fprintf(stderr, "Error al crear proceso de sucursal!\n");
        return (EXIT_FAILURE);
      }
    }
    else {
      fprintf(stderr, "Comando no reconocido.\n");
    }
    // Implementar a continuacion los otros comandos
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
