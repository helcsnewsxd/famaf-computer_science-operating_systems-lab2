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

  // Opening a semaphore for "ping"
  error = sem_open(0,1);
  exit_if_zero(error,"Error con open semáforos");
  // Opening a semaphore for "pong"
  error = sem_open(1,0); 
  exit_if_zero(error,"Error con open semáforos");

  int N = atoi(argv[1]);
  
  int pid = fork(); // Ping prints in the child process, pong in the parent process
  if(pid < 0){
    exit_if_zero(0,"Error con fork");

  }else if(pid == 0){ // child process
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(0);
      exit_if_zero(error,"Error con down semáforos");
      
      printf("ping\n");

      error = sem_up(1);
      exit_if_zero(error,"Error con up semáforos");
    }

  }else{ // parent process
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(1);
      exit_if_zero(error,"Error con down semáforos");

      printf("    pong\n");

      error = sem_up(0);
      exit_if_zero(error,"Error con up semáforos");
    }

    // Closing the semaphore
    error = sem_close(0); 
    exit_if_zero(error,"Error con close semáforos");
    // Closing the semaphore
    error = sem_close(1);
    exit_if_zero(error,"Error con close semáforos");
  }

  return 1;
}
