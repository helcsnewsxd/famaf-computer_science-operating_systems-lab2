#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define exit_if_zero(error,msg) if(error == 0){printf("%s\n",msg);return 0;}

int
main(int argc, char *argv[])
{
  if(argc != 2){
    exit_if_zero(0,"Cnt inválida de argumentos");
  }

  int error = 1;

  // abrimos un semáforo para el ping
  error = sem_open(0,1);
  exit_if_zero(error,"Error con open semáforos");
  // abrimos un semáforo para el pong
  error = sem_open(1,0); 
  exit_if_zero(error,"Error con open semáforos");
  // abrimos un semáforo para el pung
  error = sem_open(2,0); 
  exit_if_zero(error,"Error con open semáforos");

  int N = atoi(argv[1]);
  
  int pid = fork(); // Hacemos el ping en el proceso hijo y el pong en el proceso padre.
  if(pid < 0){
    exit_if_zero(0,"Error con fork");

  }else if(pid == 0){ // proceso hijo PING
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(0);
      exit_if_zero(error,"Error con down semáforos");
      
      printf("ping\n");

      error = sem_up(1);
      exit_if_zero(error,"Error con up semáforos");
    }

  }else{ // proceso padre, 2 procesos restantes
    int pid2 = fork();
    if(pid2 < 0){
      exit_if_zero(0,"Error con fork");

    }else if(pid2 == 0){ // proceso hijo PONG
      for(unsigned int i = 0; i < N; i++){
        error = sem_down(1);
        exit_if_zero(error,"Error con down semáforos");

        printf("    pong\n");

        error = sem_up(2);
        exit_if_zero(error,"Error con up semáforos");
      }

    }else{ // proceso padre PUNG
      for(unsigned int i = 0; i < N; i++){
        error = sem_down(2);
        exit_if_zero(error,"Error con down semáforos");

        printf("        pung\n");

        error = sem_up(0);
        exit_if_zero(error,"Error con up semáforos");
      }

      // Cerramos semáforo
      error = sem_close(0); 
      exit_if_zero(error,"Error con close semáforos");
      // Cerramos semáforo
      error = sem_close(1);
      exit_if_zero(error,"Error con close semáforos");
      // Cerramos semáforo
      error = sem_close(2);
      exit_if_zero(error,"Error con close semáforos");
    }
  }

  return 1;
}
