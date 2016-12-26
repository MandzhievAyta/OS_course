// hello, world
#include <inc/lib.h>
void *check_func_addr1(void* a)
{
  cprintf("CHECK_FUNC_ADDR1, %d\n", *((int*) a));
//  cprintf("PTHREAD_EXIT_ADRESS%p\n", (void*)&sys_pthread_exit);
//  sys_pthread_exit();
  return NULL;
}

void *check_func_addr2(void* a)
{
  cprintf("CHECK_FUNC_ADDR2, %d\n", *((int*) a));
//  cprintf("PTHREAD_EXIT_ADRESS%p\n", (void*)&sys_pthread_exit);
//  sys_pthread_exit();
  return NULL;
}

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
  int c1 = 228;
  int c2 = 300;
  pthread_t t1 = 333, t2 = 333;
  struct pthread_attr_t attrib;
  attrib.pthread_type = PTHREAD_CREATE_DETACHED;
  attrib.priority = 2;
  attrib.sched_policy = SCHED_FIFO;
  sys_pthread_create(&t1, NULL, &check_func_addr1, (void*)&c1);
  sys_pthread_create(&t2, &attrib, &check_func_addr2, (void*)&c2);
  cprintf("t1 = %08x, t2 = %08x\n", t1, t2);
  sys_print_pthread_state(t1);
  sys_print_pthread_state(t2);
  sys_print_pthread_state(t1 - 1);
  sys_yield();
  sys_pthread_join();
  sys_sched_setparam();
  sys_sched_setscheduler();
}
