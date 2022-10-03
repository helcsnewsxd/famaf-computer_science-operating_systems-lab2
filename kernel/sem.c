#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// Estructura usada para cada semáforo. Value es el "contador", lock es el spinlock que administra los procesos dormidos
struct sem{
  int value;
  struct spinlock lock;
};

// Declaración de tamaño máximo del arreglo de semáforos
struct sem semaphore_counter[MAXCNTSEM];

// Syscal auxiliar para inicializar los semáforos en el arreglo.
void seminit(void)
{
  for(unsigned int i = 0; i < MAXCNTSEM; i++){
    semaphore_counter[i].value = -1;
  }
}

uint64
sys_sem_open(void)
{
  char num[4];
  char lock_name[17] = "sem_spinlock ";

  int sem_id, init_value;
  argint(0, &sem_id); // argint toma el id pasado por usuario y lo guarda en sem_id
  argint(1, &init_value); // argint toma el valor pasado por usuario y lo guarda en init_value

  if(sem_id < 0 || sem_id >= MAXCNTSEM || init_value < 0 || semaphore_counter[sem_id].value != -1){
    return 0;
    // Error en init, devuelve cero en caso de cumplirse alguna de las guardas.
  }

  uint_to_str(num, sem_id);
  strcat(lock_name, num);

  initlock(&semaphore_counter[sem_id].lock, lock_name);  // Inicializa lock con nombre "sem_spinlock".

  // Zona crítica para evitar que datos se sobreescriban
  acquire(&semaphore_counter[sem_id].lock); // Se lockea

  semaphore_counter[sem_id].value = init_value;  // Se guarda el valor deseado
  
  release(&semaphore_counter[sem_id].lock); // Se libera
  // Fin de zona crítica

  return 1; // Devuelve 1 en caso de no haber errores
}

uint64
sys_sem_up(void)
{
  int sem_id; 
  argint(0, &sem_id); // Se toma el id del semáforo ingresado por usuario y se guarda en sem_id.

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    return 0; // Hay errores? Si los hay, devuelve cero.
  }

  // Zona crítica para evitar que datos se sobreescriban
  acquire(&semaphore_counter[sem_id].lock); // Se lockea

  if(semaphore_counter[sem_id].value == 0){
    wakeup(&semaphore_counter[sem_id]); // Al sumar en uno el contador, los procesos dormidos tienen prioridad para usar el recurso. Se utiliza wakeup para despertarlos.
  }

  semaphore_counter[sem_id].value++; // Aumenta el valor del semáforo

  release(&semaphore_counter[sem_id].lock); // Se libera
  // Fin de zona crítica.

  return 1;
}

uint64
sys_sem_down(void)
{
  int sem_id;
  argint(0, &sem_id); // Se toma el id del semáforo ingresado por usuario y se guarda en sem_id.

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    return 0; // En caso de haber errores, devuelve cero.
  }

  // Zona crítica para evitar que datos se sobreescriban
  acquire(&semaphore_counter[sem_id].lock); // Se lockea

  while(semaphore_counter[sem_id].value == 0){                               
    sleep(&semaphore_counter[sem_id], &semaphore_counter[sem_id].lock);       // Duerme aquellos procesos que necesiten el recurso en el caso de que el contador esté en cero
  }                                                                          
  semaphore_counter[sem_id].value--;  // Disminuye el valor del semáforo                                        
                                        
  release(&semaphore_counter[sem_id].lock); // Se libera
  // Fin de zona crítica

  return 1;
}

uint64
sys_sem_close(void)
{
  int sem_id;
  argint(0, &sem_id); // Se toma el id del semáforo ingresado por usuario y se guarda en sem_id.

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    return 0; // En caso de haber errores, devuelve cero
  }

  wakeup(&semaphore_counter[sem_id]); // Si hay procesos dormidos esperando, los despierta

  // Zona crítica para evitar que datos se sobreescriban
  acquire(&semaphore_counter[sem_id].lock);  // Se lockea

  semaphore_counter[sem_id].value = -1;       // Guardo -1 en el valor para simbolizar que el semáforo está cerrado
  
  release(&semaphore_counter[sem_id].lock);  // Se libera
  // Fin de zona crítica

  return 1; 
}
