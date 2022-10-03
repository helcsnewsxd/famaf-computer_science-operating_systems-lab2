#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define exit_error(error) if(error == 0){printf("Error in sumamultiplicadivide\n");return 0;}

int
main(int argc, char *argv[])
{
  if(argc != 2){
    exit_error(0);

  }

  int error = 1;

  
  error = sem_open(0,1);
  exit_error(error);
  
  error = sem_open(1,0); 
  exit_error(error);
  
  error = sem_open(2,0); 
  exit_error(error);

  int N = atoi(argv[1]);
  
  int pid = fork(); // A partir de aquí tendremos en total 2 procesos
  if(pid < 0){
    exit_error(0);
    
  }else if(pid == 0){ // Hijo
    for(unsigned int i = 0; i < N; i++){
      error = sem_down(0);
      exit_error(error);
      
      printf("ping\n");

      error = sem_up(1);
      exit_error(error);
    }

  }else{ // Padre

    int pid_2 = fork(); // A partir de aquí, tendremos en total 3 procesos
    if(pid_2 < 0){
    exit_error(0); 
    } else if (pid_2 == 0) { // Hijo
            for(unsigned int i = 0; i < N; i++){
            error = sem_down(1);
            exit_error(error);

            printf("    pong\n");

            error = sem_up(2);
            exit_error(error);
            }
        } else { // Padre

            for(unsigned int i = 0; i < N; i++){
            error = sem_down(2);
            exit_error(error);

            printf("        pung\n");

            error = sem_up(0);
            exit_error(error);
            }

            // Cerramos semÃ¡foro
            error = sem_close(0); 
            exit_error(error);
            // Cerramos semÃ¡foro
            error = sem_close(1);
            exit_error(error);
            // Cerramos semÃ¡foro
            error = sem_close(2);
            exit_error(error);
        }
    }

  return 1;
}