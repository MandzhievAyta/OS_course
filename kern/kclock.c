/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/time.h>

static int get_time(void)
{
  struct tm time;
  time.tm_sec = BCD2BIN(mc146818_read(RTC_SEC));
  time.tm_min = BCD2BIN(mc146818_read(RTC_MIN));
  time.tm_hour = BCD2BIN(mc146818_read(RTC_HOUR));
  time.tm_mday = BCD2BIN(mc146818_read(RTC_DAY));
  time.tm_mon = BCD2BIN(mc146818_read(RTC_MON)) - 1;
  time.tm_year = BCD2BIN(mc146818_read(RTC_YEAR)) + 100;
  return timestamp(&time);
}

int gettime(void)
{
	nmi_disable();
	// LAB 12: your code here
  int t1, t2;
  do {
    t1 = get_time();
    t2 = get_time();
  } while (t1 != t2);

	nmi_enable();
  return t1;
}

void
rtc_init(void)
{
	nmi_disable();
	// LAB 4: your code here
  uint8_t A, B;
  outb(IO_RTC_CMND, RTC_BREG);
  B = inb(IO_RTC_DATA);
  B |= RTC_PIE;
  outb(IO_RTC_DATA, B);
  outb(IO_RTC_CMND, RTC_AREG);
  A = inb(IO_RTC_DATA);
  A = A;
  outb(IO_RTC_DATA, A);

	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status;
	// LAB 4: your code here
  outb(IO_RTC_CMND, RTC_CREG);
  status = inb(IO_RTC_DATA);
	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}

