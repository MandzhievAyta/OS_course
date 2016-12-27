// hello, world
#include <inc/lib.h>
void *func1(void *a) {
  sys_yield();
  int *res;
  res = malloc(4);
  *res = 777;
  sys_pthread_exit((void*)res);
  return NULL;
}

void *func2(void *a) {
  int *res;
  cprintf("I'am joining\n");
  sys_print_pthread_state(*(pthread_t*)a);
  sys_pthread_join(*(pthread_t*)a, (void**)&res);
  cprintf("I'VE GOT RESULT OF FIRST PTHREAD: %d\n", *res);
  return NULL;
}
void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
  pthread_t t1, t2;
  struct pthread_attr_t attr;
  attr.priority = 1;
  attr.sched_policy = SCHED_RR;
  attr.pthread_type = PTHREAD_CREATE_JOINABLE;
  sys_pthread_create(&t1, &attr, &func1, NULL);
  sys_pthread_create(&t2, NULL, &func2, &t1);
  sys_yield();
  sys_yield();
}
