#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
// XXX : Adding headers for [TimerCore].
#include "threads/thread.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"
// XXX

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

/* Sleep and yield the CPU to other threads. */
void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

/* Busy waits. */
void timer_mdelay (int64_t milliseconds);
void timer_udelay (int64_t microseconds);
void timer_ndelay (int64_t nanoseconds);

void timer_print_stats (void);

// XXX : [TimerCore] Structure Declare.
struct TimerCore{
	int64_t unblock_when;
	struct thread *target;
	struct list_elem elem;
};
// XXX : [TimerCore] Variables.
struct list *TimerCore;
// XXX : [TimerCore] Functions.
void TimerCore_init(void);
void TimerCore_destroy(void);
void TimerCore_check(void);
void TimerCore_add(int64_t);
bool TimerCore_list_less_func(
		const struct list_elem *a,
		const struct list_elem *b,
		void *aux UNUSED);
// XXX
#endif /* devices/timer.h */
