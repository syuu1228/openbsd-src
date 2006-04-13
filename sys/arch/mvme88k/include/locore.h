/*	$OpenBSD: src/sys/arch/mvme88k/include/Attic/locore.h,v 1.31 2006/04/13 21:16:15 miod Exp $	*/

#ifndef _MACHINE_LOCORE_H_
#define _MACHINE_LOCORE_H_

/*
 * C prototypes for various routines defined in locore_* and friends
 */

/* subr.S */

int badaddr(vaddr_t addr, int size);
#define badwordaddr(x) badaddr(x, 4)
void doboot(void);

/* machdep.c */

unsigned getipl(void);
int intr_findvec(int, int, int);
void myetheraddr(u_char *);
void set_cpu_number(cpuid_t);

/* eh.S */

void sigsys(void);
void sigtrap(void);
void stepbpt(void);
void userbpt(void);
void syscall_handler(void);
void cache_flush_handler(void);
void m88110_sigsys(void);
void m88110_sigtrap(void);
void m88110_stepbpt(void);
void m88110_userbpt(void);
void m88110_syscall_handler(void);
void m88110_cache_flush_handler(void);

#endif /* _MACHINE_LOCORE_H_ */
