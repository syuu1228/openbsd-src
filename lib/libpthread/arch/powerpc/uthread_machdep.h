/*
 * OpenBSD/powerpc machine-dependent thread macros
 *
 * $OpenBSD: src/lib/libpthread/arch/powerpc/uthread_machdep.h,v 1.2 1999/01/17 23:49:49 d Exp $
 */

/* save the floating point state of a thread */
#define _thread_machdep_save_float_state(thr) 		\
	{						\
		/* rahnds to fill in */			\
	}

/* restore the floating point state of a thread */
#define _thread_machdep_restore_float_state(thr) 	\
	{						\
		/* rahnds to fill in */			\
	}

/* initialise the jmpbuf stack frame so it continues from entry */
#define _thread_machdep_thread_create(thr, entry, pattr)	\
	{						\
		/* rahnds to fill in */			\
	}

#define	_thread_machdep_longjmp(a,v)	longjmp(a,v)
#define	_thread_machdep_setjmp(a)	setjmp(a)

typedef jmp_buf _machdep_jmp_buf;

struct _machdep_struct {
        char            xxx;
};

