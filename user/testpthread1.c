//Create 1 pthread and print it
#include <inc/lib.h>
void *func1(void *a)
{
  cprintf("I am POSIX thread!\n");
  sys_print_pthread_state(0);
  return NULL;
}

void
umain(int argc, char **argv)
{
  pthread_t t1;
  struct pthread_attr_t attr;
  attr.priority = 2;
  attr.sched_policy = SCHED_FIFO;
  attr.pthread_type = PTHREAD_CREATE_DETACHED;
  cprintf("Creating POSIX thread\n");
  sys_pthread_create(&t1, &attr, &func1, NULL);
  sys_yield();
}
