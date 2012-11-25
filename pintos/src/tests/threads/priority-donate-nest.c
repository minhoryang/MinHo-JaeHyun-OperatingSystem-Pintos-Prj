/* Low-priority main thread L acquires lock A.  Medium-priority
   thread M then acquires lock B then blocks on acquiring lock A.
   High-priority thread H then blocks on acquiring lock B.  Thus,
   thread H donates its priority to M, which in turn donates it
   to thread L.
   
   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by Matt Franklin <startled@leland.stanford.edu>,
   Greg Hutchins <gmh@leland.stanford.edu>, Yu Ping Hu
   <yph@cs.stanford.edu>.  Modified by arens. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct locks 
  {
    struct lock *a;
    struct lock *b;
  };

static thread_func medium_thread_func;
static thread_func high_thread_func;

void
test_priority_donate_nest (void) 
{
  struct lock a, b;
  struct locks locks;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);
  // 0. Low(31)

  lock_init (&a);
  lock_init (&b);

  lock_acquire (&a);
  // 1. A락을 걸기 시도 A->semaphore=0. 성공

  locks.a = &a;
  locks.b = &b;
  thread_create ("medium", PRI_DEFAULT + 1, medium_thread_func, &locks);
     // 2. Medium(32)를 생성함
     //    thread_create()-thread_yield()-schedule()-nt2r()-DONATE()-HPL()
     // 3. HPL()은 검색을 시도
     //     1) 락A는 Low(31)이 걸고, 기다리는Thread없음.
     //     2) 락B는 Initialize
     //    락을 기다리는 Thread가 없으므로 Donate할 필요가 없음. 무시.
     // 4. DONATE()무시
     // 5. nt2r()은 Priority로 Medium(32)선택. schedule()
  // 14. Low(31->32)임.
  thread_yield (); // 15. schedule()-nt2r()-DONATE()-HPL()
                   // 16. HPL()은 검색을 시도.
                   //      1) 락A는 Low(31->32)걸고, Medium(32)가 기다림
                   //      2) 락B는 Medium(32)가 걸고, 기다리는놈X
                   //     락을 기다리는 Thread가 존재하지만,
                   //     기다리는놈과 Priority가 같기때문에 무시.
                   // 17. DONATE() 도 무시.
                   // 18. nt2r()은 RR로 Low(31->32)선택. schedule()
  // 19. Low(31->32)
  msg ("Low thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 1, thread_get_priority ());
  // 20. Low(31->32)

  thread_create ("high", PRI_DEFAULT + 2, high_thread_func, &b);
     // 21. High(33)을 생성함
     //    thread_create()-thread_yield()-schedule()-nt2r()-DONATE()-HPL()
     // 22. HPL()이 검색을 시도
     //    1) 락A는 Low(31->32)가 걸어 Medium(32)가 기다리는중
     //    2) 락B는 Medium(32)가 걸고 기다리는놈X.
     //   락을 기다리는 Thread가 존재하지만,
     //   기다리는놈과 Priority가 같기때문에 무시.
     // 23. DONATE()도 무시.
     // 24. nt2r()은 Priority로 HIGH(33)선택. schedule()
  // 34. Low(31->32->33)
  thread_yield (); // 35. schedule()-nt2r()-DONATE()-HPL()
                   // 36. HPL() 검색을 시도.
                   //    1) 락A,Low(31->32->33)짓,Medium(32->33)기달
                   //    2) 락B,Medium(32->33)짓, High(33)기달
                   //     락을 기다리는 Thread가 존재하지만,
                   //     다들 기다리는놈과 Priority가 같아 무시.
                   // 37. DONATE() 무시.
                   // 38. nt2r()은 RR로 Low(31->32->33)선택. schedule()
  // 39. Low(31->32->33)
  msg ("Low thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 2, thread_get_priority ());
  // 40. Low(31->32->33)

  lock_release (&a); // 41. 락A를 해제!-sema_up()-thread_unblock()
                     //     Medium(32->33)풀림, Ready_List에 삽입!
                     //     Low(31->32->33)은 Low(31->33)으로.
                     //     sema_up()-thread_yield()-schedule()-nt2r()
                     //            DONATE()-HPL()
                     // 42. HPL() 검색을 시도.
                     //    1) 락A, Low(31->33)이 걸고, 기다리는놈X
                     //    2) 락B, Medium(32->33)걸고, High(33)기다림.
                     //     락을 기다리는 Thread가 존재하지만,
                     //     기다리는놈과 Priority가 같아 무시.
                     // 43. DONATE() 무시.
                     // 44. nt2r()은 RR로 Medium(32->33)선택, schedule()
  // Low(31->33)
  thread_yield (); // E.
                   // 52. schedule()-nt2r()-DONATE()-HPL()
                   // 53. HPL() 검색을 시도.
                   //    1) 락A, Low(31->33)이 걸고, 기다리는놈X
                   //    2) 락B, Medium(32->33)이 걸고, High(33)이 기다림
                   //     락을 기다리는 Thread가 존재하지만,
                   //     기다리는놈과 Priority가 같아 무시.
                   // 54. DONATE() 무시.
                   // 55. nt2r()은 RR로 Medium(32->33)선택. schedule()
  // 59. Low(31->33)
  // ===============FAILED=============
  msg ("Medium thread should just have finished.");
  msg ("Low thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT, thread_get_priority ());
}

static void
medium_thread_func (void *locks_) 
{
  // 6. Medium(32) 시작!
  struct locks *locks = locks_;

  lock_acquire (locks->b);
  // 7. B락을 걸기 시도 B->semaphore=0. 성공
  lock_acquire (locks->a);  // 8. A락을걸기시도-sema_down()-thread_block()
							//              -schedule()-nt2r()-DONATE()
                            // 9. DONATE()가 HighestPriorityLockee()호출
                            // 10. HPL()이 검색을 시도
                            //    1) 락A는 Low(31)이 걸어 Medium(32)가기달
                            //    2) 락B는 Medium(32)가걸고, 기다리는놈X
                            //     락을 기다리는 Thread가 존재하고,
                            //     기다리는놈이 Priority가 높기때문에 선택
                            // 11. DONATE()는 Low(31)을 Low(31->32)로 세팅
                            // 12. nt2r()은 Low(31->32)선택
                            // 13. schedule()이 Low(31->32)로 보냄.

                            // 30. A락을걸기시도-sema_down()-thread_block)
                            //          -schedule()-nt2r()-DONATE()-HPL()
                            // 31. HPL()이 검색을 시도
                            //  1) 락A, Low(31->32)걸고 Medium(32->33)기달
                            //  2) 락B, Medium(32->33)걸고 High(33)기달
                            //   락을기다리고 Priority가 낮은 Low(31->32)
                            //   선택.
                            // 32. DONATE()는Low(31->32)를Low(31->32->33)
                            // 33. nt2r()은 Low(31->32->33)선택. schedule)

                            // 45. Medium(32->33)
							// 46. A락을걸기시도 A->semaphore=0. 성공!
  // 47. Medium(32->33)
  msg ("Medium thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 2, thread_get_priority ());
  /* 만약 이사이에 RR이 일어난다면 
    A. schedule()-nt2r()-DONATE()-HPL()
    B. HPL() 검색을 시도.
      1) 락A, Medium(32->33)이 걸고, Medium(32->33)기다림.
      2) 락B, Medium(32->33)이 걸고, High(33)기다림.
       락을 기다리는 Thread들이 존재하지만,
       다들 기다리는놈과 Priority가 같아 무시.
    C. DONATE() 무시.
    D. nt2r()은 RR로 Low(31->33)선택, schedule() 
  */
  msg ("Medium thread got the lock.");
  // 48. Medium(32->33)

  lock_release (locks->a); // 49. 락A를해제-sema_up()-thread_unblock()
                           //     자기가 건 락이 풀림. A->semaphore=1.
                           //     sema_up()-thread_yield()-schedule()
                           //       -nt2r()-DONATE()-HPL()
                           // 50. HPL() 검색을 시도.
                           //    1) 락A, Initialize
                           //    2) 락B, Medium(32->33)짓, High(33)기달.
                           //     락을 기다리는 Thread가 존재하지만,
                           //     기다리는놈과 Priority가 같아 무시.
                           // 51. DONATE() 무시.
                           // 52. nt2r()은 RR로 Low(31->33)선택,schedule) 
  thread_yield (); // 55. schedule()-nt2r()-DONATE()-HPL()
                   // 56. HPL() 검색을 시도.
                   //    1) 락A, Initialize
                   //    2) 락B, Medium(32->33)짓, High(33)기달.
				   //     락을 기다리는 Thread가 존재하지만,
                   //     기다리는놈과 Priority가 같아 무시.
                   // 57. DONATE() 무시.
                   // 58. nt2r()은 RR로 Low(31->33)선택, schedule()

  lock_release (locks->b);
  thread_yield ();

  msg ("High thread should have just finished.");
  msg ("Middle thread finished.");
}

static void
high_thread_func (void *lock_) 
{
  // 25. High(33) 시작!
  struct lock *lock = lock_;

  lock_acquire (lock); // 26. B락을 걸기시도-sema_down()-thread_block()
                       //             -schedule()-nt2r()-DONATE()-HPL()
                       // 27. HPL()이 검색을 시도
                       //    1) 락A는 Low(31->32)가 걸어 Medium(32)기달.
                       //    2) 락B는 Medium(32)가 걸어 High(33)이 기달.
                       //  락을 기다리는 Thread가 Medium(32), High(33)존재
                       //  기다리는놈보다 Priority가 낮은 놈은 Medium(32)
                       // 28. DONATE()가 Medium(32)를 Medium(32->33)세팅.
                       // 29. nt2r()이 Medium(32->33)선택, schedule()

  msg ("High thread got the lock.");
  lock_release (lock);
  msg ("High thread finished.");
}
