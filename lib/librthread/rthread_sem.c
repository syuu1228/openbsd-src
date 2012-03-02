/*	$OpenBSD: src/lib/librthread/rthread_sem.c,v 1.5 2012/03/02 08:07:43 guenther Exp $ */
/*
 * Copyright (c) 2004,2005 Ted Unangst <tedu@openbsd.org>
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include "rthread.h"

/*
 * Internal implementation of semaphores
 */
int
_sem_wait(sem_t sem, int tryonly, int *delayed_cancel)
{
	int r;

	_spinlock(&sem->lock);
	if (sem->value) {
		sem->value--;
		r = 1;
	} else if (tryonly) {
		r = 0;
	} else {
		sem->waitcount++;
		do {
			r = __thrsleep(&sem->waitcount, 0, NULL, &sem->lock,
			    delayed_cancel) == 0;
			_spinlock(&sem->lock);
		} while (r && sem->value == 0);
		sem->waitcount--;
		if (r)
			sem->value--;
	}
	_spinunlock(&sem->lock);
	return (r);
}

/* always increment count */
int
_sem_post(sem_t sem)
{
	int rv = 0;

	_spinlock(&sem->lock);
	sem->value++;
	if (sem->waitcount) {
		__thrwakeup(&sem->waitcount, 1);
		rv = 1;
	}
	_spinunlock(&sem->lock);
	return (rv);
}

/*
 * exported semaphores
 */
int
sem_init(sem_t *semp, int pshared, unsigned int value)
{
	sem_t sem;

	if (pshared) {
		errno = EPERM;
		return (-1);
	}

	if (value > SEM_VALUE_MAX) {
		errno = EINVAL;
		return (-1);
	}

	sem = calloc(1, sizeof(*sem));
	if (!sem) {
		errno = ENOSPC;
		return (-1);
	}
	sem->lock = _SPINLOCK_UNLOCKED;
	sem->value = value;
	*semp = sem;

	return (0);
}

int
sem_destroy(sem_t *semp)
{
	if (!semp || !*semp) {
		errno = EINVAL;
		return (-1);
	}

	if ((*semp)->waitcount) {
#define MSG "sem_destroy on semaphore with waiters!\n"
		write(2, MSG, sizeof(MSG) - 1);
#undef MSG
		errno = EBUSY;
		return (-1);
	}
	free(*semp);
	*semp = NULL;

	return (0);
}

int
sem_getvalue(sem_t *semp, int *sval)
{
	sem_t sem = *semp;

	if (!semp || !*semp) {
		errno = EINVAL;
		return (-1);
	}

	_spinlock(&sem->lock);
	*sval = sem->value;
	_spinunlock(&sem->lock);

	return (0);
}

int
sem_post(sem_t *semp)
{
	sem_t sem = *semp;

	if (!semp || !*semp) {
		errno = EINVAL;
		return (-1);
	}

	_sem_post(sem);

	return (0);
}

int
sem_wait(sem_t *semp)
{
	sem_t sem = *semp;
	pthread_t self = pthread_self();
	int r;

	if (!semp || !*semp) {
		errno = EINVAL;
		return (-1);
	}

	_enter_delayed_cancel(self);
	r = _sem_wait(sem, 0, &self->delayed_cancel);
	_leave_delayed_cancel(self, !r);

	return (0);
}

int
sem_trywait(sem_t *semp)
{
	sem_t sem = *semp;
	int rv;

	if (!semp || !*semp) {
		errno = EINVAL;
		return (-1);
	}

	rv = _sem_wait(sem, 1, NULL);

	if (!rv) {
		errno = EAGAIN;
		return (-1);
	}

	return (0);
}

/* ARGSUSED */
sem_t *
sem_open(const char *name __unused, int oflag __unused, ...)
{
	errno = ENOSYS;
	return (SEM_FAILED);
}

/* ARGSUSED */
int
sem_close(sem_t *sem __unused)
{
	errno = ENOSYS;
	return (-1);
}

/* ARGSUSED */
int
sem_unlink(const char *name __unused)
{
	errno = ENOSYS;
	return (-1);
}

