/*	$OpenBSD: src/sys/dev/pci/ahci.c,v 1.13 2006/12/11 05:47:53 dlg Exp $ */

/*
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#define AHCI_DEBUG

#ifdef AHCI_DEBUG
#define DPRINTF(m, f...) do { if (ahcidebug & (m)) printf(f); } while (0)
#define AHCI_D_VERBOSE		0x01
int ahcidebug = AHCI_D_VERBOSE;
#else
#define DPRINTF(m, f...)
#endif

#define AHCI_PCI_BAR		0x24

#define AHCI_REG_CAP		0x000 /* HBA Capabilities */
#define  AHCI_REG_CAP_NP(_r)		(((_r) & 0x1f)+1) /* Number of Ports */
#define  AHCI_REG_CAP_SXS		(1<<5) /* External SATA */
#define  AHCI_REG_CAP_EMS		(1<<6) /* Enclosure Mgmt */
#define  AHCI_REG_CAP_CCCS		(1<<7) /* Cmd Coalescing */
#define  AHCI_REG_CAP_NCS(_r)		((((_r) & 0x1f00)>>8)+1) /* NCmds*/
#define  AHCI_REG_CAP_PSC		(1<<13) /* Partial State Capable */
#define  AHCI_REG_CAP_SSC		(1<<14) /* Slumber State Capable */
#define  AHCI_REG_CAP_PMD		(1<<15) /* PIO Multiple DRQ Block */
#define  AHCI_REG_CAP_FBSS		(1<<16) /* FIS-Based Switching */
#define  AHCI_REG_CAP_SPM		(1<<17) /* Port Multiplier */
#define  AHCI_REG_CAP_SAM		(1<<18) /* AHCI Only mode */
#define  AHCI_REG_CAP_SNZO		(1<<19) /* Non Zero DMA Offsets */
#define  AHCI_REG_CAP_ISS		(0xf<<20) /* Interface Speed Support */
#define  AHCI_REG_CAP_ISS_G1		(0x1<<20) /* Gen 1 (1.5 Gbps) */
#define  AHCI_REG_CAP_ISS_G1_2		(0x2<<20) /* Gen 1 and 2 (3 Gbps) */
#define  AHCI_REG_CAP_SCLO		(1<<24) /* Cmd List Override */
#define  AHCI_REG_CAP_SAL		(1<<25) /* Activity LED */
#define  AHCI_REG_CAP_SALP		(1<<26) /* Aggresive Link Pwr Mgmt */
#define  AHCI_REG_CAP_SSS		(1<<27) /* Staggered Spinup */
#define  AHCI_REG_CAP_SMPS		(1<<28) /* Mech Presence Switch */
#define  AHCI_REG_CAP_SSNTF		(1<<29) /* SNotification Register */
#define  AHCI_REG_CAP_SNCQ		(1<<30) /* Native Cmd Queuing */
#define  AHCI_REG_CAP_S64A		(1<<31) /* 64bit Addressing */

#define  AHCI_FMT_CAP		"\020" "\006SXS" "\007EMS" "\010CCCS" \
				    "\016PSC" "\017SSC" "\020PMD" "\021FBSS" \
				    "\x12SPM" "\x13SAM" "\x14SNZO" "\x19SCLO" \
				    "\x1aSAL" "\x1bSALP" "\x1cSSS" "\x1dSMPS" \
				    "\036SSNTF" "\037NCQ" "\040S64A"

#define AHCI_REG_GHC		0x004 /* Global HBA Control */
#define  AHCI_REG_GHC_HR		(1<<0) /* HBA Reset */
#define  AHCI_REG_GHC_IE		(1<<1) /* Interrupt Enable */
#define  AHCI_REG_GHC_MRSM		(1<<2) /* MSI Revert to Single Msg */
#define  AHCI_REG_GHC_AE		(1<<31) /* AHCI Enable */
#define AHCI_REG_IS		0x008 /* Interrupt Status */
#define AHCI_REG_PI		0x00c /* Ports Implemented */
#define AHCI_REG_VS		0x010 /* AHCI Version */
#define  AHCI_REG_VS_0_95		0x00000905 /* 0.95 */
#define  AHCI_REG_VS_1_0		0x00010000 /* 1.0 */
#define  AHCI_REG_VS_1_1		0x00010100 /* 1.1 */
#define AHCI_REG_CCC_CTL	0x014 /* Coalescing Control */
#define AHCI_REG_CCC_PORTS	0x018 /* Coalescing Ports */
#define AHCI_REG_EM_LOC		0x01c /* Enclosure Mgmt Location */
#define AHCI_REG_EM_CTL		0x020 /* Enclosure Mgmt Control */

#define _PO(_p)			(0x100 + ((_p) * 0x80)) /* port offset */

#define AHCI_PREG_CLB(_p)	(_PO(_p)+0x00) /* Cmd List Base Addr */
#define AHCI_PREG_CLBU(_p)	(_PO(_p)+0x04) /* Cmd List Base Hi Addr */
#define AHCI_PREG_FB(_p)	(_PO(_p)+0x08) /* FIS Base Addr */
#define AHCI_PREG_FBU(_p)	(_PO(_p)+0x0c) /* FIS Base Hi Addr */
#define AHCI_PREG_IS(_p)	(_PO(_p)+0x10) /* Interrupt Status */
#define AHCI_PREG_IE(_p)	(_PO(_p)+0x14) /* Interrupt Enable */
#define AHCI_PREG_CMD(_p)	(_PO(_p)+0x18) /* Command and Status */
#define AHCI_PREG_TFD(_p)	(_PO(_p)+0x20) /* Task File Data*/
#define AHCI_PREG_SIG(_p)	(_PO(_p)+0x24) /* Signature */
#define AHCI_PREG_Status(_p)	(_PO(_p)+0x28) /* SATA Status */
#define AHCI_PREG_Control(_p)	(_PO(_p)+0x2c) /* SATA Control */
#define AHCI_PREG_Error(_p)	(_PO(_p)+0x30) /* SATA Error */
#define AHCI_PREG_Active(_p)	(_PO(_p)+0x34) /* SATA Active */
#define AHCI_PREG_CI(_p)	(_PO(_p)+0x38) /* Command Issue */

static const struct pci_matchid ahci_devices[] = {
	{ PCI_VENDOR_JMICRON,	PCI_PRODUCT_JMICRON_JMB361 }
};

int		ahci_match(struct device *, void *, void *);
void		ahci_attach(struct device *, struct device *, void *);

struct ahci_softc {
	struct device		sc_dev;

	void			*sc_ih;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	bus_dma_tag_t		sc_dmat;

	u_int32_t		sc_portmap;
};
#define DEVNAME(_s)	((_s)->sc_dev.dv_xname)

struct cfattach ahci_ca = {
	sizeof(struct ahci_softc), ahci_match, ahci_attach
};

struct cfdriver ahci_cd = {
	NULL, "ahci", DV_DULL
};

int		ahci_intr(void *);

int		ahci_map_regs(struct ahci_softc *, struct pci_attach_args *);
void		ahci_unmap_regs(struct ahci_softc *, struct pci_attach_args *);
int		ahci_init(struct ahci_softc *);
int		ahci_map_intr(struct ahci_softc *, struct pci_attach_args *);
void		ahci_unmap_intr(struct ahci_softc *, struct pci_attach_args *);

u_int32_t	ahci_read(struct ahci_softc *, bus_size_t);
void		ahci_write(struct ahci_softc *, bus_size_t, u_int32_t);
int		ahci_wait_eq(struct ahci_softc *, bus_size_t,
		    u_int32_t, u_int32_t);
int		ahci_wait_ne(struct ahci_softc *, bus_size_t,
		    u_int32_t, u_int32_t);

int
ahci_match(struct device *parent, void *match, void *aux)
{
	return (pci_matchbyid((struct pci_attach_args *)aux, ahci_devices,
	    sizeof(ahci_devices) / sizeof(ahci_devices[0])));
}

void
ahci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ahci_softc		*sc = (struct ahci_softc *)self;
	struct pci_attach_args		*pa = aux;

	if (ahci_map_regs(sc, pa) != 0) {
		/* error already printed by ahci_map_regs */
		return;
	}

	if (ahci_init(sc) != 0) {
		/* error already printed by ahci_init */
		goto unmap;
	}

	if (ahci_map_intr(sc, pa) != 0) {
		/* error already printed by ahci_map_intr */
		goto unmap;
	}

	printf("\n");

#ifdef AHCI_DEBUG
	if (ahcidebug & AHCI_D_VERBOSE) {
		u_int32_t reg = ahci_read(sc, AHCI_REG_CAP);
		const char *gen;

		switch (reg & AHCI_REG_CAP_ISS) {
		case AHCI_REG_CAP_ISS_G1:
			gen = "1 (1.5Gbps)";
			break;
		case AHCI_REG_CAP_ISS_G1_2:
			gen = "1 (1.5Gbps) and 2 (3Gbps)";
			break;
		default:
			gen = "unknown";
			break;
		}

		printf("%s: capabilities: 0x%b ports: %d ncmds: %d gen: %s\n",
		    DEVNAME(sc), reg, AHCI_FMT_CAP,
		    AHCI_REG_CAP_NP(reg), AHCI_REG_CAP_NCS(reg), gen);
		printf("%s: ports implemented: 0x%08x\n", DEVNAME(sc),
		    sc->sc_portmap);
	}
#endif

unmap:
	ahci_unmap_regs(sc, pa);
}

int
ahci_map_regs(struct ahci_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t			memtype;

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, AHCI_PCI_BAR);
	if (pci_mapreg_map(pa, AHCI_PCI_BAR, memtype, 0, &sc->sc_iot,
	    &sc->sc_ioh, NULL, &sc->sc_ios, 0) != 0) {
		printf(": unable to map registers\n");
		return (1);
	}

	return (0);
}

void
ahci_unmap_regs(struct ahci_softc *sc, struct pci_attach_args *pa)
{
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
}

int
ahci_map_intr(struct ahci_softc *sc, struct pci_attach_args *pa)
{
	pci_intr_handle_t		ih;
	const char			*intrstr;

	if (pci_intr_map(pa, &ih) != 0) {
		printf(": unable to map interrupt\n");
		return (1);
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
	    ahci_intr, sc, DEVNAME(sc));
	if (sc->sc_ih == NULL) {
		printf(": unable to map interrupt%s%s\n",
		    intrstr == NULL ? "" : " at ",
		    intrstr == NULL ? "" : intrstr);
		return (1);
	}
	printf(": %s", intrstr);

	return (0);
}

void
ahci_unmap_intr(struct ahci_softc *sc, struct pci_attach_args *pa)
{
	pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
}

int
ahci_init(struct ahci_softc *sc)
{
	u_int32_t			reg;
	const char			*revision;

	/* reset the controller */
	ahci_write(sc, AHCI_REG_GHC, AHCI_REG_GHC_HR);
	if (ahci_wait_ne(sc, AHCI_REG_GHC, AHCI_REG_GHC_HR,
	    AHCI_REG_GHC_HR) != 0) {
		printf(": unable to reset controller\n");
		return (1);
	}

	/* enable ahci */
	ahci_write(sc, AHCI_REG_GHC, AHCI_REG_GHC_AE);

	/* check the revision */
	reg = ahci_read(sc, AHCI_REG_VS);
	switch (reg) {
	case AHCI_REG_VS_0_95:
		revision = "0.95";
		break;
	case AHCI_REG_VS_1_0:
		revision = "1.0";
		break;
	case AHCI_REG_VS_1_1:
		revision = "1.1";
		break;

	default:
		printf(": unsupported AHCI revision 0x%08x\n", reg);
		return (1);
	}

	/* clean interrupts */
	reg = ahci_read(sc, AHCI_REG_IS);
	ahci_write(sc, AHCI_REG_IS, reg);

	printf(": AHCI %s", revision);

	sc->sc_portmap = ahci_read(sc, AHCI_REG_PI);

	return (0);
}

int
ahci_intr(void *arg)
{
	return (0);
}

u_int32_t
ahci_read(struct ahci_softc *sc, bus_size_t r)
{
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, r));
}

void
ahci_write(struct ahci_softc *sc, bus_size_t r, u_int32_t v)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, r, v);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

int
ahci_wait_eq(struct ahci_softc *sc, bus_size_t r, u_int32_t mask,
    u_int32_t target)
{ 
	int				i;

	for (i = 0; i < 1000; i++) {
		if ((ahci_read(sc, r) & mask) == target)
			return (0);
		delay(1000);
	}

	return (1);
}

int
ahci_wait_ne(struct ahci_softc *sc, bus_size_t r, u_int32_t mask,
    u_int32_t target)
{ 
	int				i;

	for (i = 0; i < 1000; i++) {
		if ((ahci_read(sc, r) & mask) != target)
			return (0);
		delay(1000);
	}

	return (1);
}
