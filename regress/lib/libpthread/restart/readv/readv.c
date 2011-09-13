/*	$OpenBSD: src/regress/lib/libpthread/restart/readv/readv.c,v 1.1 2011/09/13 23:50:17 fgsch Exp $	*/
/*
 * Federico G. Schwindt <fgsch@openbsd.org>, 2011. Public Domain.
 */
#include <sys/types.h>
#include <sys/uio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "test.h"

volatile sig_atomic_t hits = 0;

void
handler(int sig)
{
	hits++;
}

void *
thr_readv(void *arg)
{
	struct iovec iov;
	int fds[2];
	char buf;

	CHECKe(pipe(fds));
	iov.iov_base = &buf;
	iov.iov_len = 1;
	ASSERT(readv(fds[0], &iov, 1) == -1);
	return ((caddr_t)NULL + errno);
}

int
main(int argc, char **argv)
{
	struct sigaction sa;
	pthread_t tid;
	void *retval;

	bzero(&sa, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	CHECKe(sigaction(SIGUSR1, &sa, NULL));
	sa.sa_flags = 0;
	CHECKe(sigaction(SIGUSR2, &sa, NULL));

	CHECKr(pthread_create(&tid, NULL, thr_readv, NULL));
	sleep(2);

	/* Should restart it. */
	CHECKr(pthread_kill(tid, SIGUSR1));
	sleep(1);

	/* Should interrupt it. */
	CHECKr(pthread_kill(tid, SIGUSR2));
	sleep(1);

	CHECKr(pthread_join(tid, &retval));
	ASSERT(retval == (void *)EINTR);
	ASSERT(hits == 2);
	SUCCEED;
}
