/*	$OpenBSD: src/sys/altq/Attic/altq_localq.c,v 1.2 2001/08/09 14:32:59 deraadt Exp $	*/
/*	$KAME: altq_localq.c,v 1.3 2000/10/18 09:15:23 kjc Exp $	*/

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <netinet/in.h>

#include <altq/altq.h>
#include <altq/altq_conf.h>

/*
 * localq device interface
 */
altqdev_decl(localq);

int
localqopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	/* everything will be done when the queueing scheme is attached. */
	return 0;
}

int
localqclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int error = 0;

	return error;
}

int
localqioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	ioctlcmd_t cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	int error = 0;
	
	return error;
}

#ifdef KLD_MODULE

static struct altqsw localq_sw =
	{"localq", localqopen, localqclose, localqioctl};

ALTQ_MODULE(altq_localq, ALTQT_LOCALQ, &localq_sw);

#endif /* KLD_MODULE */
