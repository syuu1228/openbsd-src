/*
 * OpenBSD/hppa machine-dependent thread macros
 *
 * $OpenBSD: src/lib/libc_r/arch/hppa/Attic/uthread_machdep.h,v 1.1 1998/12/21 22:54:56 mickey Exp $$
 */

/* save the floating point state of a thread */
#define _thread_machdep_save_float_state(thr) 		\
	{						\
		/* fsave privileged instr */		\
	}

/* restore the floating point state of a thread */
#define _thread_machdep_restore_float_state(thr) 	\
	{						\
		/* frestore privileged instr */		\
	}

/* initialise the jmpbuf stack frame so it continues from entry */

#define _thread_machdep_thread_create(thr, entry, pattr)	\
	{						\
	    /* entry */					\
	    (thr)->saved_jmp_buf[5] = (long) entry;	\
	    /* stack */					\
	    (thr)->saved_jmp_buf[2] = (long) (thr)->stack \
				+ (pattr)->stacksize_attr \
				- sizeof(double);	\
	}

#define	_thread_machdep_longjmp(a,v)	_longjmp(a,v)
#define	_thread_machdep_setjmp(a)	_setjmp(a)

struct _machdep_struct {
        /* char            saved_fp[108]; */
	int	dummy;
};

