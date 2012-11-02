#ifndef USERPROG_SYSCALL_EXIT_H
#define USERPROG_SYSCALL_EXIT_H

// XXX : Bridged between [syscall.c] and [exception.c].
//       We don't know why, but Collision of 'syscall_exit()' definition was happened.
void syscall_exit(int status);

#endif /* userprog/syscall_exit.h */
