// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/tsc.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line



struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
  { "printsome", "Prints something", mon_printsome},
  { "backtrace", "Prints backtrace", mon_backtrace},
  { "bt", "Alias of backtrace", mon_backtrace},
  { "start", "start of Time Stamp Counter", mon_start},
  { "stop", "stop of Time Stamp Counter", mon_stop},
  { "pages", "Info about memory pages", mon_pages }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/


int mon_pages(int argc, char **argv, struct Trapframe *tf)
{
  size_t i, left = 0;
  int cur_status = 1;
  for (i = 1; i <= npages; i++) {
    if ( i == npages ||
        (!pages[i].pp_link && (cur_status == 0) ) ||
        ((pages[i].pp_link || top_free_list == &pages[i]) && (cur_status == 1)) )
    {
      cprintf("%u", left + 1);
      if (left != i - 1 && i != 1) {
        cprintf("..%u", i);
      }
      cprintf(" %s\n", cur_status ? "ALLOCATED" : "FREE");
      left = i;
    }
    cur_status = pages[i].pp_link ? 0 : 1;
  }
  return 0;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", (uint32_t)_start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n",
            (uint32_t)entry, (uint32_t)entry - KERNTOP);
	cprintf("  etext  %08x (virt)  %08x (phys)\n",
            (uint32_t)etext, (uint32_t)etext - KERNTOP);
	cprintf("  edata  %08x (virt)  %08x (phys)\n",
            (uint32_t)edata, (uint32_t)edata - KERNTOP);
	cprintf("  end    %08x (virt)  %08x (phys)\n",
            (uint32_t)end, (uint32_t)end - KERNTOP);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_start(int argc, char **argv, struct Trapframe *tf)
{
  timer_start();
  return 0;
}

int
mon_stop(int argc, char **argv, struct Trapframe *tf)
{
  timer_stop();
  return 0;
}

int
mon_printsome(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("I printed something!\n");
  return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  uint32_t ebp = read_ebp();
  cprintf("Stack backtrace:\n");
  while (ebp) {
    uint32_t *cur_frame = (uint32_t *)ebp;
    struct Eipdebuginfo file_info;
    cprintf("ebp %08x eip %08x args %08x %08x %08x %08x %08x\n",
            ebp, cur_frame[1], cur_frame[2], cur_frame[3],
            cur_frame[4], cur_frame[5], cur_frame[6]);
    debuginfo_eip(cur_frame[1], &file_info);
    cprintf("         %.*s:%d: %.*s+%d\n", 100, file_info.eip_file, file_info.eip_line,
            file_info.eip_fn_namelen, file_info.eip_fn_name, cur_frame[1] - file_info.eip_fn_addr);
    ebp = cur_frame[0];
  }
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
