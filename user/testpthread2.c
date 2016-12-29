//First two functions work while they have quantum of time.
//Third work forever, because of FIFO policy
#include <inc/lib.h>
void *func1(void *a)
{
  cprintf("I am POSIX thread! number [%08x]\n", *(pthread_t*)a);
  for(;;) {}
  return NULL;
}
void *func2(void *a)
{
  cprintf("I am POSIX thread! number [%08x]\n", *(pthread_t*)a);
  for(;;) {}
  return NULL;
}

void *func3(void *a)
{
  cprintf("I am POSIX thread! number [%08x]\n", *(pthread_t*)a);
  for(;;) {}
  return NULL;
}

void
umain(int argc, char **argv)
{
  pthread_t t1, t2, t3;
  struct pthread_attr_t attr;
  attr.priority = 2;
  attr.sched_policy = SCHED_FIFO;
  attr.pthread_type = PTHREAD_CREATE_DETACHED;
  cprintf("Creating POSIX thread\n");
  sys_pthread_create(&t1, NULL, &func1, &t1);
  sys_pthread_create(&t2, NULL, &func2, &t2);
  sys_pthread_create(&t3, &attr, &func3, &t3);
  sys_yield();
}
