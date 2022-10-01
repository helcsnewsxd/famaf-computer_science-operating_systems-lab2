#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define exit_error(error) if(error == 0){printf("Error in pingpong\n");return 0;}

int
main(int argc, char *argv[])
{
  if(argc != 2){
    exit_error(0);

  }

  int error = 1;

  // abrimos un semáforo para el ping
  error = sem_open(0,1);
  exit_error(error);
  // abrimos un semáforo para el pong
  error = sem_open(1,0); 
  exit_error(error);

  int N = atoi(argv[1]);
  
  int pid = fork(); // Hacemos el ping en el proceso hijo y el pong en el proceso padre.
  if(pid < 0){
    exit_error(0);

  }else if(pid == 0){ // proceso hijo
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(0);
      exit_error(error);
      
      printf("ping\n");

      error = sem_up(1);
      exit_error(error);
    }

  }else{ // proceso padre
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(1);
      exit_error(error);

      printf("pong\n");

      error = sem_up(0);
      exit_error(error);
    }

    // Cerramos semáforo
    error = sem_close(0); 
    exit_error(error);
    // Cerramos semáforo
    error = sem_close(1);
    exit_error(error);
  }

  return 1;
}
