#include <stdio.h>

int
sys_sem_open (int sem, int value)
{
  printf ("Soy sys_sem_open");
  return 0;
}

int
sys_sem_up (int sem)
{
  printf ("Soy sys_sem_up");
  return 0;
}

int
sys_sem_close (int sem)
{
  printf ("Soy sys_sem_close");
  return 0;
}

int
sys_sem_down (int sem)
{
  printf ("Soy sys_sem_down");
  return 0;
}
