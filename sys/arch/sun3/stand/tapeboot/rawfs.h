/*	$OpenBSD: src/sys/arch/sun3/stand/tapeboot/Attic/rawfs.h,v 1.2 2001/07/04 08:33:57 niklas Exp $	*/
/*	$NetBSD: rawfs.h,v 1.1 1995/10/17 22:58:29 gwr Exp $	*/

/*
 * Raw file system - for stream devices like tapes.
 * No random access, only sequential read allowed.
 */

int	rawfs_open __P((char *path, struct open_file *f));
int	rawfs_close __P((struct open_file *f));
int	rawfs_read __P((struct open_file *f, void *buf,
		u_int size, u_int *resid));
int	rawfs_write __P((struct open_file *f, void *buf,
		u_int size, u_int *resid));
off_t	rawfs_seek __P((struct open_file *f, off_t offset, int where));
int	rawfs_stat __P((struct open_file *f, struct stat *sb));
