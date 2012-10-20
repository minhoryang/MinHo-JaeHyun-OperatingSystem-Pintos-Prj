#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */

// XXX: semaphore

struct child_list{
	struct list_elem elem;
	tid_t value;
};

void add_child(struct thread *, tid_t);
void del_child(struct thread *, tid_t);

// XXX
