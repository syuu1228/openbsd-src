/*	$OpenBSD: src/sys/arch/mvme88k/stand/netboot/dev_net.h,v 1.3 2002/03/14 01:26:40 millert Exp $ */

int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl();
int	net_strategy();

