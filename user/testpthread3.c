//First two functions work while they have quantum of time.
//Third work forever, because of FIFO policy
#include <inc/lib.h>
void *func1(void *a)
{
  cprintf("I am POSIX thread! number [%08x]\n", *(pthread_t*)a);
  return NULL;
}

void
umain(int argc, char **argv)
{
  pthread_t t1;
  cprintf("Creating POSIX thread\n");
  sys_pthread_create(&t1, NULL, &func1, &t1);
  sys_print_pthread_state(t1);
  cprintf("Changing priority to 4 and policy to FIFO!\n");
  sys_sched_setscheduler(t1, SCHED_FIFO, 4);
  sys_print_pthread_state(t1);
  sys_yield();
}
