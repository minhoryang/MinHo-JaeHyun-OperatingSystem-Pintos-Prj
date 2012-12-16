#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/syscall_exit.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  // TODO 3. Pintos VM!
#ifdef VM
  if(false){
    printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  }
  // XXX : Check flags.
  switch(user){
	  case false:
		  syscall_exit(-1);  // Kernel  // XXX : TESTED by "pt-bad-addr", "pt-grow-bad", "pt-write-code2".
	  case true:
		  switch(not_present){
			  case false:
				  syscall_exit(-1);  // Tried to write read-only page.  // XXX : TESTED by "pt-write-code".
			  case true:
				  if(!write)
					  syscall_exit(-1);  // Tried to read at non-present page.  // XXX : TESTED by "pt-bad-read".
				  else
					break;
		  }
  }
  // TODO 0 .Is Valid Region?
  if(!fault_addr || !is_user_vaddr(fault_addr))
    syscall_exit(-1);
  // 1. Growable_Page Region인지 판단 by palloc_get_page(USER)
  {
    // XXX : 총 몇개의 Page를 만들어 줘야 하나?
	size_t Page_Needed_Cnt = (PHYS_BASE - pg_round_down(fault_addr)) / PGSIZE;
	{
      // PHYS_BASE부터 ~ fault_addr를 포함하는 PAGE주소에서까지!
	  void *Check_Here = PHYS_BASE - PGSIZE;
	  while(Check_Here >= pg_round_up(fault_addr))
		if(is_valid_ptr(Check_Here)){
			// 이미 이 위치에 만들어진 PAGE가 있다면, 만들필요가 없지.
			Page_Needed_Cnt--;
			Check_Here -= PGSIZE;  // 다음위치로!
		}else
			break;
	}
	//printf("HOW MANY? %u\n", Page_Needed_Cnt);

    // XXX : Page_Needed_Cnt개의 PAGE를 만들어보자! 
	void *New_Page_Here = pg_round_down(fault_addr);
    while(Page_Needed_Cnt--){
	  void *Growable_Page;
      // 만약 PAGE를 얻어올 수 있다면 == Growable_Page하다라고 한다.
      if((Growable_Page = palloc_get_page(PAL_USER | PAL_ZERO)) != NULL){
        // 만들어진 PAGE를 User의 PageDir에 넣자.
        // 만약 User의 PageDir에 VADDR을 등록해 준다면 == 커널이 알아서 KADDR로 연결해준다고 가정하자. (맞음ㅋ).
		if(pagedir_set_page( // ASSIGN USER2KERNEL.
				thread_current()->pagedir,
				New_Page_Here,  // HELL YEAH!
				Growable_Page /* GOT USER PAGE. */,
				true /* WRITABLE whatever asking read. */)){
			//printf("LET GROW! %u %p\n", Page_Needed_Cnt, New_Page_Here);
		}else{
			//printf("FAILED TO ASSIGN : pagedir_set_page.\n");
			kill(f);
		}
	  }else{
  // 2-b. NULL=> KILL
		// printf("NO MORE Growable_Page!\n");
		kill(f);
	  }
	  New_Page_Here += PGSIZE;
	}
  }
  // Restart Process.
  // printf("DONE!\n");
#else
  /* // XXX 2) User Memory Access!
  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  kill (f);
  */
  syscall_exit(-1);
  // XXX 2-2 UserProg.
  //  Tests: bad-read, bad-write, bad-read2, bad-write2, bad-jump.
  //
  //  We tried to check is_valid_ptr() when we handled whole syscalls which were main point of page_fault.
  //  So we removed commenting sections becaused of useless. 
  //  But, Those tests tried to use invalid pointer at the inside of userprog.
  //  Using invalid pointers inside of userprog code is hard to check is_valid_ptr();
  //  So we decided to ignore those page-fault sections, again.
  //  At this point, we couldn't find the better idea for handling this.
  //  If TA checked this, we need your comments. Thanks.
  // XXX
  //thread_exit();
#endif
  // XXX
}

bool is_valid_ptr(const void *usr_ptr){
	if(!usr_ptr) return false;
	//printf("%p - pass NULL\n", usr_ptr);
	if(is_user_vaddr(usr_ptr)){
		//printf("%p - pass User VADDR\n", usr_ptr);
		if(pagedir_get_page(thread_current()->pagedir, usr_ptr)){
			//printf("%p - pass User PageDir\n", usr_ptr);
			return true;
		}
	}
	return false;
}
