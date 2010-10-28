/*	$OpenBSD: src/sys/arch/octeon/dev/octeon_pcibusvar.h,v 1.1 2010/10/28 22:52:10 syuu Exp $	*/
/*	$NetBSD: octeon_pcibusvar.h,v 1.4 2008/04/28 20:23:28 martin Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OCTEON_PCIBUSVAR_H_
#define	_OCTEON_PCIBUSVAR_H_

struct octeon_pcibus_cfg_hook;
struct extent;

struct octeon_pcibus_config {
	int oc_adbase;	/* AD line base for config access */
};

struct octeon_pcibus_softc {
	struct device sc_dev;
	const struct octeon_pcibus_config *sc_octeon_pcibus;
	struct mips_pci_chipset sc_pc;
	struct obio_attach_args *sc_oba;

	/* PCI Configuration Space access hooks */
	SLIST_HEAD(, octeon_pcibus_cfg_hook) sc_hook;
};

#ifdef _KERNEL
void	 octeon_pcibus_intr_disestablish(void *);
void	*octeon_pcibus_intr_establish(int, int, int, int (*)(void *), void *,
	    const char *);
int	 octeon_pcibus_pci_hook(pci_chipset_tag_t, void *,
	    int (*)(void *, pci_chipset_tag_t, pcitag_t, int, pcireg_t *),
	    int (*)(void *, pci_chipset_tag_t, pcitag_t, int, pcireg_t));
int	 octeon_pcibus_print(void *, const char *);
struct extent
	*octeon_pcibus_get_resource_extent(pci_chipset_tag_t, int);
void	 octeon_pcibus_setintrmask(int);
#endif /* _KERNEL */

#endif /* _OCTEON_PCIBUSVAR_H_ */
