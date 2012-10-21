#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

// XXX
#include <string.h>
#include "lib/user/syscall.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);

void syscall_halt(void);
void syscall_exit(int status);
pid_t syscall_exec(const char *file);
int syscall_wait(pid_t pid);
int syscall_read(int fd, void *buffer, unsigned size);
int syscall_write(int fd, const void *buffer, unsigned size);
bool is_valid_ptr(const void *usr_ptr);
// XXX : ADD 2 Custom System Call
int syscall_pibonacci (int n);
int syscall_sum_of_four_integers(int a, int b, int c, int d);
// XXX

void
syscall_init (void) 
{
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  // XXX : EDIT WITH 'syscall.h' 'lib/user/syscall.c' 'lib/syscall-nr.h
  //printf ("system call!\n");
  void *now = f->esp;
  // XXX : Check PTR Range, and bash to syscall_exit(-1);
  if(!is_valid_ptr(now))
    syscall_exit(-1);
  int syscall_number = *(int *)(f->esp);
  int argc_size_table[22] = {  // CHECK syscall-nr.h
	  0,  // SYS_HALT (*)
	  1,  // SYS_EXIT (*)
	  1,  // SYS_EXEC (*)
	  1,  // SYS_WAIT (*)
	  2,  // SYS_CREATE
	  1,  // SYS_REMOVE
	  1,  // SYS_OPEN
	  1,  // SYS_FILESIZE
	  3,  // SYS_READ (*)
	  3,  // SYS_WRITE (*)
	  2,  // SYS_SEEK
	  1,  // SYS_TELL
	  1,  // SYS_CLOSE
	  2,  // SYS_MMAP
	  1,  // SYS_MUNMAP
	  1,  // SYS_CHDIR
	  1,  // SYS_MKDIR
	  2,  // SYS_READDIR
	  1,  // SYS_ISDIR
	  1,  // SYS_INUMBER
	  1,  // SYS_PIBONACCI
	  4   // SYS_SUM_OF_FOUR_INTEGERS
  };
  int argc_size = argc_size_table[syscall_number];
  //printf("SYSCALL %d SIZE %d\n", syscall_number, argc_size);
  void *argc[4] = {NULL,};
  {
    int i;
    for(i = 0; i < argc_size; i++){
      now += 4; // sizeof(void *);  // IT WILL USE 4 Bytes. (ref:man38).
      argc[i] = now;
	  // XXX : Check argument's ptr;
	  if(!is_valid_ptr(argc[i]))
		syscall_exit(-1);
	 // printf("%x\n", now);
    }
  }
  switch(syscall_number){
	default:
	case -1:
		break;
	case 0:  // SYS_HALT
		syscall_halt();
		break;
	case 1:  // SYS_EXIT
		syscall_exit(*(int *)argc[0]);
		break;
	case 2:  // SYS_EXEC
		f->eax = syscall_exec(*(const char **)argc[0]);
		break;
	case 3:  // SYS_WAIT
		//printf("CALLED SYS_WAIT!\n");
		f->eax = syscall_wait(*(pid_t *)argc[0]);
		break;
	case 8:  // SYS_READ
		f->eax = syscall_read(
				*(int *)argc[0],
				*(void **)argc[1],
				*(unsigned *)argc[2]
		);
		break;
	case 9:  // SYS_WRITE
		//printf("CALLED WRITE! %d %x %u\n", *(int *)argc[0], argc[1], *(unsigned *)argc[2]);
		f->eax = syscall_write(
				*(int *)argc[0],
				*(const void **)argc[1],
				*(unsigned *)argc[2]
		);
	case 20:  // SYS_PIBONACCI
		f->eax = syscall_pibonacci(*(int *)argc[0]);
		break;
	case 21:  // SYS_SUM_OF_FOUR_INTEGERS
		f->eax = syscall_sum_of_four_integers(
				*(int *)argc[0],
				*(int *)argc[1],
				*(int *)argc[2],
				*(int *)argc[3]
		);
		break;
	// case *:
  }
  // printf("SYSCALL_RETURN: %d\n", f->eax);
  // thread_exit ();
  // XXX
}

// XXX: syscall functions below!
//
// * EDIT WITH 'lib/user/syscall.c' 'lib/syscall-nr.h

void syscall_halt(void){
	shutdown_power_off();
	return ;
}

void syscall_exit(int status){
	/* Terminates the current user program, returning status to the kernel.
	 *
	 * If the process's parent waits for it, this is the status that
	 * will be returnd.
	 *
	 * Conventionally, a status of 0 indicates success and nonzero values
	 * indicate errors. (ref:man29-30)
	 */
    struct thread *t = thread_current ();
	char *wanted, *last;
	wanted = strtok_r(t->name, " ", &last);
	printf("%s: exit(%d)\n", wanted, status);
	// XXX : Delete Child from Parent.
	struct thread *parent = t->parent;
	// XXX : Wait until parent waiting child.
	while(!( (t->parent->status == THREAD_BLOCKED)
				&& (t->parent->waiting_tid == t->tid) )){
		thread_yield();
	}
	// SEARCH It;
	struct list_elem *e;
	for (e = list_begin(&(parent->childs));
		 e != list_end(&(parent->childs));
		 e = list_next(e)){
	       //printf("list anything out: %p\n", (void *)e);
		struct child_list *now = list_entry(e,
				                            struct child_list,
											elem);
		if(now->tid == t->tid)
			// Unlink Child from Parent;
			if(e){
				//free(list_remove(e));
				list_remove(e);
				break;
			}
	}
	//}
	// XXX : Store return status to parent.
	t->parent->waited_child_return_value = status;
	// XXX
	// XXX : Wakeup Parent.
    //printf("[%s/%s]SEMAUP? %d\n", t->parent->name, t->name, t->parent->sema.value);
	sema_up(&(t->parent->sema));
    //printf("[%s/%s]SEMAUP! %d\n", t->parent->name, t->name, t->parent->sema.value);

	thread_exit();
	return ;
}

pid_t syscall_exec(const char *file){
	/* Runs the executable whose name is given in cmd_line,
	 * passing any given arguments, and returns the new process's
	 * program id (pid).
	 *
	 * Must return pid -1, which otherwise should not be a valid pid,
	 * if the program cannot load or run for any reason.
	 *
	 * Thus, the parent process cannot return from the exec
	 * until it knows whether the child process successfully loaded
	 * its executable. (ref:man29-30)
	 */
	// TODO check bad ptr; syscall_exit(-1);
    if(!is_valid_ptr(file))
	  syscall_exit(-1);
	return process_execute(file);
}

int syscall_wait(pid_t pid){
	return process_wait(pid);
}

int syscall_read(int fd, void *buffer, unsigned size){
	/* Reads size bytes from the file open as fd into buffer.
	 *
	 * Returns the number of bytes actually read (0 at end of file),
	 * or -1 if the file could not be read (due to a condition
	 * other than end of file).
	 * 
	 * FD 0 reads from the keyboard using input_getc(). (ref:man29-31)
	 */
	int ret = -1;
	unsigned i;
	if(fd == 0){
		// STDIN 0
		for(i = 0; i < size; i++){
			uint8_t buf = input_getc();
			memset((buffer + i * sizeof(uint8_t)), buf, sizeof(uint8_t));
		}
		ret = i;
	}
	return ret;
}

int syscall_write(int fd, const void *buffer, unsigned size){
	/* Writes size bytes from buffer to the open file fd.
	 *
	 * Returns the number of bytes actually written, which may be less than
	 * size if some bytes coudl not be written.
	 *
	 * Writing pas end-of-file would normally extend the file,
	 * but file growth is not implemented by the basic file system.
	 *
	 * The expected behaviour is to write as many bytes as possible
	 * up to end-of-file and return the actual number written,
	 * or 0 if no bytes could be written at all.
	 *
	 * Fd 1 writes to the console.
	 * 
	 * Your code to write to the console should write all of buffer
	 * in one call to putbuf(), at least as long as size is not bigger than
	 * a few hundred bytes. (It is reasonable to break up larger buffers.)
	 *
	 * Otherwise, lines of text output by different processes may end up
	 * interleaved on the console, confusing both human readers
	 * and our grading scripts.
	 * (ref:man29-31)
	 */
	int ret = -1;
	unsigned i = 0;
	if(fd == 1){
		// STDOUT 1
		for(i = 0; (i < size) && *(char *)(buffer + i * sizeof(char)); i++){}
		putbuf(buffer, i);
		ret = i;
	}
	return ret;
}

bool is_valid_ptr(const void *usr_ptr){
	if(!usr_ptr) return false;
	if(is_user_vaddr(usr_ptr)){
		struct thread *cur = thread_current();
		if(pagedir_get_page(cur->pagedir, usr_ptr))
			return true;
	}
	return false;
}
// XXX

// TODO : ADD 2 Custom System Call
int syscall_pibonacci (int n){
	int i=2, *f = (int*)malloc(sizeof(int)*n);
	f[0] = 1;
	f[1] = 1;
	while(i<n){
		f[i] = f[i-1]+f[i-2];
		i++;
	}
	return f[i-1];
}
int syscall_sum_of_four_integers(int a, int b, int c, int d){
	printf("%d %d %d %d\n", a, b, c, d);
	return a + b + c + d;
}
// XXX
