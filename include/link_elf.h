/*	$OpenBSD: src/include/link_elf.h,v 1.5 2004/10/14 10:02:28 kettenis Exp $	*/

/*
 * Public domain.
 */

#ifndef _LINK_ELF_H
#define _LINK_ELF_H

#include <elf_abi.h>

#ifndef DT_PROCNUM
#define DT_PROCNUM 0
#endif

/*
 * struct link_map is a part of the protocol between the debugger and
 * ld.so. ld.so may have additional fields in it's version of this
 * stucture but those are ld.so private fields.
 */
struct link_map {
	caddr_t		l_addr;		/* Base address of library */
	const char	*l_name;	/* Absolute path to library */
	void		*l_ld;		/* pointer to _DYNAMIC */
	struct link_map	*l_next;
	struct link_map	*l_prev;
};

struct dl_phdr_info {
	Elf_Addr	dlpi_addr;
	const char	*dlpi_name;
	const Elf_Phdr	*dlpi_phdr;
	Elf_Half	dlpi_phnum;
};

__BEGIN_DECLS
int	dl_iterate_phdr (int (*)(struct dl_phdr_info *, size_t, void *),
	   void *);
__END_DECLS

#endif /* !_LINK_ELF_H */
