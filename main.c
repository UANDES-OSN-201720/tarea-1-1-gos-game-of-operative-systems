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

    printf("Comando ingresado: '%s'\n", commandBuf);

    if (!strncmp("quit", commandBuf, strlen("quit"))) {
        break;
    }
    else if (!strncmp("list", commandBuf, strlen("list"))){
      printf("Lista de sucursales: \n");
      for(int sucIndex = 0; sucIndex < pidArrayCounter; sucIndex++){
        printf("Sucursal almacenada con ID '%d'\n", pidArray[sucIndex]);
      }
    }
    else if (!strncmp("init", commandBuf, strlen("init"))) {
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
