/*	$OpenBSD: src/sys/arch/alpha/pci/apecs.c,v 1.14 2001/11/04 23:12:46 art Exp $	*/
/*	$NetBSD: apecs.c,v 1.16 1996/12/05 01:39:34 cgd Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>
#ifdef DEC_2100_A50
#include <alpha/pci/pci_2100_a50.h>
#endif
#ifdef DEC_EB64PLUS
#include <alpha/pci/pci_eb64plus.h>
#endif
#ifdef DEC_1000A
#include <alpha/pci/pci_1000a.h>
#endif
#ifdef DEC_1000
#include <alpha/pci/pci_1000.h>
#endif

int	apecsmatch __P((struct device *, void *, void *));
void	apecsattach __P((struct device *, struct device *, void *));

struct cfattach apecs_ca = {
	sizeof(struct apecs_softc), apecsmatch, apecsattach,
};

struct cfdriver apecs_cd = {
	NULL, "apecs", DV_DULL,
};

int	apecsprint __P((void *, const char *pnp));

/* There can be only one. */
int apecsfound;
struct apecs_config apecs_configuration;

int
apecsmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/* Make sure that we're looking for an APECS. */
	if (strcmp(ma->ma_name, apecs_cd.cd_name) != 0)
		return (0);

	if (apecsfound)
		return (0);

	return (1);
}

/*
 * Set up the chipset's function pointers.
 */
void
apecs_init(acp, mallocsafe)
	struct apecs_config *acp;
	int mallocsafe;
{
	acp->ac_comanche_pass2 =
	    (REGVAL(COMANCHE_ED) & COMANCHE_ED_PASS2) != 0;
	acp->ac_memwidth =
	    (REGVAL(COMANCHE_GCR) & COMANCHE_GCR_WIDEMEM) != 0 ? 128 : 64;
	acp->ac_epic_pass2 =
	    (REGVAL(EPIC_DCSR) & EPIC_DCSR_PASS2) != 0;

	acp->ac_haxr1 = REGVAL(EPIC_HAXR1);
	acp->ac_haxr2 = REGVAL(EPIC_HAXR2);

	if (!acp->ac_initted) {
		/* don't do these twice since they set up extents */
		acp->ac_iot = apecs_bus_io_init(acp);
		acp->ac_memt = apecs_bus_mem_init(acp);

#if 0
		/*
		 * We have two I/O windows and 3 MEM windows.
		 */
		alpha_bus_window_count[ALPHA_BUS_TYPE_PCI_IO] = 2;
		alpha_bus_window_count[ALPHA_BUS_TYPE_PCI_MEM] = 3;
		alpha_bus_get_window = apecs_bus_get_window;
#endif
	}
	acp->ac_mallocsafe = mallocsafe;

	apecs_pci_init(&acp->ac_pc, acp);
	alpha_pci_chipset = &acp->ac_pc;
	alpha_pci_chipset->pc_name = "apecs";
	alpha_pci_chipset->pc_mem = APECS_PCI_SPARSE;
	alpha_pci_chipset->pc_dense = APECS_PCI_DENSE;
	alpha_pci_chipset->pc_ports = APECS_PCI_SIO;
	alpha_pci_chipset->pc_bwx = 0;
	alpha_pci_chipset->pc_hae_mask = EPIC_HAXR1_EADDR;

	acp->ac_initted = 1;
}

void
apecsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct apecs_softc *sc = (struct apecs_softc *)self;
	struct apecs_config *acp;
	struct pcibus_attach_args pba;

	/* note that we've attached the chipset; can't have 2 APECSes. */
	apecsfound = 1;

	/*
	 * set up the chipset's info; done once at console init time
	 * (maybe), but doesn't hurt to do twice.
	 */
	acp = sc->sc_acp = &apecs_configuration;
	apecs_init(acp, 1);

	apecs_dma_init(acp);

	printf(": DECchip %s Core Logic chipset\n",
	    acp->ac_memwidth == 128 ? "21072" : "21071");
	printf("%s: DC21071-CA pass %d, %d-bit memory bus\n",
	    self->dv_xname, acp->ac_comanche_pass2 ? 2 : 1, acp->ac_memwidth);
	printf("%s: DC21071-DA pass %d\n", self->dv_xname,
	    acp->ac_epic_pass2 ? 2 : 1);
	/* XXX print bcache size */

	if (!acp->ac_epic_pass2)
		printf("WARNING: 21071-DA NOT PASS2... NO BETS...\n");

	switch (cputype) {
#ifdef DEC_2100_A50
	case ST_DEC_2100_A50:
		pci_2100_a50_pickintr(acp);
		break;
#endif

#ifdef DEC_EB64PLUS
	case ST_EB64P:
		pci_eb64plus_pickintr(acp);
		break;
#endif

#ifdef DEC_1000A
	case ST_DEC_1000A:
		pci_1000a_pickintr(acp, &acp->ac_iot, &acp->ac_memt,
			&acp->ac_pc);
		break;
#endif

#ifdef DEC_1000
	case ST_DEC_1000:
		pci_1000_pickintr(acp, &acp->ac_iot, &acp->ac_memt,
			&acp->ac_pc);
		break;
#endif

	default:
		panic("apecsattach: shouldn't be here, really...");
	}

	pba.pba_busname = "pci";
	pba.pba_iot = acp->ac_iot;
	pba.pba_memt = acp->ac_memt;
	pba.pba_dmat =
	    alphabus_dma_get_tag(&acp->ac_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_pc = &acp->ac_pc;
	pba.pba_bus = 0;
#ifdef notyet
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
#endif
	config_found(self, &pba, apecsprint);
}

int
apecsprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to APECSes; easy. */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

#if 0
int
apecs_bus_get_window(type, window, abst)
	int type, window;
	struct alpha_bus_space_translation *abst;
{
	struct apecs_config *acp = &apecs_configuration;
	bus_space_tag_t st;

	switch (type) {
	case ALPHA_BUS_TYPE_PCI_IO:
		st = &acp->ac_iot;
		break;

	case ALPHA_BUS_TYPE_PCI_MEM:
		st = &acp->ac_memt;
		break;

	default:
		panic("apecs_bus_get_window");
	}

	return (alpha_bus_space_get_window(st, window, abst));
}
#endif
