/* $OpenBSD: src/lib/libc/include/thread_private.h,v 1.14 2002/11/05 22:19:55 marc Exp $ */

#ifndef _THREAD_PRIVATE_H_
#define _THREAD_PRIVATE_H_

#include <pthread.h>

/*
 * This variable is initially 0 when there is exactly one thread.
 * It should never decrease.
 */
extern int __isthreaded;

/*
 * Weak symbols are used in libc so that the thread library can
 * efficiently wrap libc functions.
 * 
 * Use WEAK_NAME(n) to get a libc-private name for n (_weak_n),
 *     WEAK_ALIAS(n) to generate the weak symbol n pointing to _weak_n,
 *     WEAK_PROTOTYPE(n) to generate a prototype for _weak_n (based on n).
 */
#define WEAK_NAME(name)			__CONCAT(_weak_,name)
#define WEAK_ALIAS(name)		__weak_alias(name, WEAK_NAME(name))
#ifdef __GNUC__
#define WEAK_PROTOTYPE(name)		__typeof__(name) WEAK_NAME(name)
#else
#define WEAK_PROTOTYPE(name)		/* typeof() only in gcc */
#endif

/*
 * These macros help in making persistent storage thread-specific.
 * Libc makes extensive use of private static data structures
 * that hold state across function invocation, and these macros
 * are no-ops when running single-threaded.
 *
 * Linking against the user-thread library causes these macros to
 * allocate storage on a per-thread basis.
 */
	
#define __THREAD_MUTEX_NAME(name)	__CONCAT(_libc_storage_mutex_,name)
#define __THREAD_KEY_NAME(name)		__CONCAT(_libc_storage_key_,name)

struct _thread_private_key_struct {
	pthread_once_t		once;
	void			(*cleanfn)(void *);
	pthread_key_t		key;
};

void	_libc_private_storage_lock(pthread_mutex_t *);
void	_libc_private_storage_unlock(pthread_mutex_t *);
void *	_libc_private_storage(volatile struct _thread_private_key_struct *,
			      void *, size_t, void *);

/* Declare a module mutex. */
#define _THREAD_PRIVATE_MUTEX(name)					\
	static pthread_mutex_t __THREAD_MUTEX_NAME(name) = 		\
		PTHREAD_MUTEX_INITIALIZER			
		
/* Lock a module mutex against use by any other threads. */
#define _THREAD_PRIVATE_MUTEX_LOCK(name) 				\
	_libc_private_storage_lock(&__THREAD_MUTEX_NAME(name))
		
/* Unlock a module mutex. */
#define _THREAD_PRIVATE_MUTEX_UNLOCK(name) 				\
	_libc_private_storage_unlock(&__THREAD_MUTEX_NAME(name))

/* Declare a thread-private storage key. */
#define _THREAD_PRIVATE_KEY(name)					\
	static volatile struct _thread_private_key_struct		\
	__THREAD_KEY_NAME(name) = {					\
		PTHREAD_ONCE_INIT, 					\
		0							\
	}

/*
 * In threaded mode, return a pointer to thread-private memory of
 * the same size as, and (initially) with the same contents as 'storage'. If
 * an error occurs, the 'error' parameter is returned.
 * In single-threaded mode, no storage is allocated. Instead, a pointer
 * to storage is always returned.
 * The 'cleanfn' function of the key structure is called to free the storage.
 * If 'cleanfn' is NULL, then free() is used. This hook can be useful for
 * getting rid of memory leaks.
 */
#define _THREAD_PRIVATE(keyname, storage, error) 			\
	_libc_private_storage(&__THREAD_KEY_NAME(keyname),		\
			      &(storage), sizeof (storage), error)

/*
 * File descriptor locking definitions.
 */
#define FD_READ		    0x1
#define FD_WRITE	    0x2
#define FD_RDWR		    (FD_READ | FD_WRITE)

#define _FD_LOCK(_fd,_type,_ts)						\
		_thread_fd_lock(_fd, _type, _ts, __FILE__, __LINE__)
#define _FD_UNLOCK(_fd,_type)						\
		_thread_fd_unlock(_fd, _type, __FILE__, __LINE__)

int	_thread_fd_lock(int, int, struct timespec *, const char *, int);
void	_thread_fd_unlock(int, int, const char *, int);

/*
 * malloc lock/unlock definitions
 */
# define _MALLOC_LOCK()		do {					\
					if (__isthreaded)		\
						_thread_malloc_lock();	\
				} while (0)
# define _MALLOC_UNLOCK()	do {					\
					if (__isthreaded)		\
						_thread_malloc_unlock();\
				} while (0)
# define _MALLOC_LOCK_INIT()do {					\
					if (__isthreaded)		\
						_thread_malloc_init();\
				} while (0)


void	_thread_malloc_init(void);
void	_thread_malloc_lock(void);
void	_thread_malloc_unlock(void);

#endif /* _THREAD_PRIVATE_H_ */
