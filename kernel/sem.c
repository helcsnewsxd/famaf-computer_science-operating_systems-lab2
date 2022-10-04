#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// Used structure for the semaphores. Value is the "counter", lock is the spinlock that controls asleep processes
struct sem{
  int value;
  struct spinlock lock;
};

// max number of semaphores (indexes)
struct sem semaphore_counter[MAXCNTSEM];

// syscall used for the initialization of the semaphores
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
  argint(0, &sem_id); // argint processes the id the user passed as input
  argint(1, &init_value); // argint processes the value the user passed as input

  if(sem_id < 0 || sem_id >= MAXCNTSEM || init_value < 0 || semaphore_counter[sem_id].value != -1){
    return 0;
    // if there's an error, return 0
  }

  uint_to_str(num, sem_id);
  strcat(lock_name, num);

  initlock(&semaphore_counter[sem_id].lock, lock_name);  // initialization of the lock.

  // Critical zone
  acquire(&semaphore_counter[sem_id].lock); // locked

  semaphore_counter[sem_id].value = init_value;  // value is saved into the semaphore's counter
  
  release(&semaphore_counter[sem_id].lock); // unlocked
  // End of critical zone

  return 1; // returns 1 if successful
}

uint64
sys_sem_up(void)
{
  int sem_id; 
  argint(0, &sem_id); // argint processes the id the user passed as input

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    return 0; // return 0 if there are any errors
  }

  // Critical zone
  acquire(&semaphore_counter[sem_id].lock); // locked

  if(semaphore_counter[sem_id].value == 0){
    wakeup(&semaphore_counter[sem_id]); // Asleep processes can now access the resources, they have priority if they were asleep and the counter is now > 0
  }

  semaphore_counter[sem_id].value++; // counter = counter + 1

  release(&semaphore_counter[sem_id].lock); // unlocked
  // End of critical zone

  return 1;
}

uint64
sys_sem_down(void)
{
  int sem_id;
  argint(0, &sem_id); // argint processes the id the user passed as input

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    return 0; // return 0 if there are any errors
  }

  // Critical zone
  acquire(&semaphore_counter[sem_id].lock); // locked

  while(semaphore_counter[sem_id].value == 0){                               
    sleep(&semaphore_counter[sem_id], &semaphore_counter[sem_id].lock);       // If the counter = 0, the process that want to access to a certain resource will now be put to sleep.
  }                                                                          
  semaphore_counter[sem_id].value--;  // counter = counter - 1                                      
                                        
  release(&semaphore_counter[sem_id].lock); // unlocked
  // End of critical zone

  return 1;
}

uint64
sys_sem_close(void)
{
  int sem_id;
  argint(0, &sem_id); // argint processes the id the user passed as input

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    return 0; // return 0 if unsuccessful
  }

  wakeup(&semaphore_counter[sem_id]); // Wakes up asleep processes

  // Critical zone
  acquire(&semaphore_counter[sem_id].lock);  // locked

  semaphore_counter[sem_id].value = -1;       // -1 symbolizes a closed semaphore. Writes -1 in all the semaphore's positions.
  
  release(&semaphore_counter[sem_id].lock);  // unlocked
  // End of critical zone

  return 1; 
}
