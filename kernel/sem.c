#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_sem_open(void)
{
  printf("Soy sys_sem_open\n");
  return 0;
}

uint64
sys_sem_up(void)
{
  printf("Soy sys_sem_up\n");
  return 0;
}

uint64
sys_sem_close(void)
{
  printf("Soy sys_sem_close\n");
  return 0;
}

uint64
sys_sem_down(void)
{
  printf("Soy sys_sem_down\n");
  return 0;
}
