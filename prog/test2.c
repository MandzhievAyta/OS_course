#include <inc/lib.h>

int (* volatile cprintf) (const char *fmt, ...);
void (* volatile sys_yield)(void);

#if defined(GRADE3_TEST)
# define xstr(s) str(s)
# define str(s) #s
void (* volatile GRADE3_TEST) (unsigned);
#endif

void
umain( int argc, char **argv )
{
	int test2_i;
	int test2_j;

#if !defined(GRADE3_TEST)
	cprintf("TEST2 LOADED.\n");
#else
	GRADE3_TEST(xstr(GRADE3_TEST)[0]);
#endif

	for(test2_j = 0; test2_j < 5; ++test2_j) {
		for(test2_i = 0; test2_i < 10000; ++test2_i ) {}
		sys_yield();
	}
}

