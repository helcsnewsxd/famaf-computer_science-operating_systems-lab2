#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  sem_open(1,0);
  sem_up(1);
  sem_down(1);
  sem_close(1);
  return 0;
}
