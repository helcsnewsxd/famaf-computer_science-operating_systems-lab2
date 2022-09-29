#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

typedef struct sem_s{
  int value;
  struct spinlock lock;
} sem;

sem semaphore_counter[MAXCNTSEM];

void seminit(void)
{
  for(unsigned int i = 0; i < MAXCNTSEM; i++){
    semaphore_counter[i].value = -1;
  }
}

uint64
sys_sem_open(void)
{
  int sem_id, init_value;
  argint(0, &sem_id);
  argint(1, &init_value);

  //printf("%d\n", semaphore_counter[sem_id].value);

  if(sem_id < 0 || sem_id >= MAXCNTSEM || init_value < 0 || semaphore_counter[sem_id].value != -1){
    panic("sem_open");
    // Index errox, Init value error or Semaphore is used
  }

  initlock(&semaphore_counter[sem_id].lock, "sem_spinlock");
  semaphore_counter[sem_id].value = init_value;

  return 0;
}

uint64
sys_sem_up(void)
{
  int sem_id;
  argint(0, &sem_id);

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    panic("sem_up");
  }

  if(semaphore_counter[sem_id].value == 0){
    wakeup(&semaphore_counter[sem_id].value);
  }
  
  semaphore_counter[sem_id].value++;

  return 0;
}

uint64
sys_sem_close(void)
{
  int sem_id;
  argint(0, &sem_id);

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    panic("sem_close");
  }

  wakeup(&semaphore_counter[sem_id].value);
  semaphore_counter[sem_id].value = -1;

  return 0;
}

uint64
sys_sem_down(void)
{
  int sem_id;
  argint(0, &sem_id);

  if(sem_id < 0 || sem_id >= MAXCNTSEM || semaphore_counter[sem_id].value == -1){
    panic("sem_down");
  }

  while(semaphore_counter[sem_id].value == 0){
    sleep(&semaphore_counter[sem_id].value, &semaphore_counter[sem_id].lock);
  }

  semaphore_counter[sem_id].value--;
  
  return 0;
}
