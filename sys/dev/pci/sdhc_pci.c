/*	$OpenBSD: src/sys/dev/pci/sdhc_pci.c,v 1.12 2011/12/23 21:58:47 kettenis Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
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
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

/*
 * 8-bit PCI configuration register that tells us how many slots there
 * are and which BAR entry corresponds to the first slot.
 */
#define SDHC_PCI_CONF_SLOT_INFO		0x40
#define SDHC_PCI_NUM_SLOTS(info)	((((info) >> 4) & 0x7) + 1)
#define SDHC_PCI_FIRST_BAR(info)	((info) & 0x7)

/* TI specific register */
#define SDHC_PCI_GENERAL_CTL		0x4c
#define  MMC_SD_DIS			0x02

/* RICOH specific registers */
#define SDHC_PCI_MODE_KEY		0xf9
#define SDHC_PCI_MODE			0x150
#define  SDHC_PCI_MODE_SD20		0x10
#define SDHC_PCI_BASE_FREQ_KEY		0xfc
#define SDHC_PCI_BASE_FREQ		0xe1

struct sdhc_pci_softc {
	struct sdhc_softc sc;
	void *sc_ih;
};

int	sdhc_pci_match(struct device *, void *, void *);
void	sdhc_pci_attach(struct device *, struct device *, void *);
void	sdhc_takecontroller(struct pci_attach_args *);
void	sdhc_pci_conf_write(struct pci_attach_args *, int, uint8_t);

struct cfattach sdhc_pci_ca = {
	sizeof(struct sdhc_pci_softc), sdhc_pci_match, sdhc_pci_attach,
	NULL, sdhc_activate
};

int
sdhc_pci_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SYSTEM &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SYSTEM_SDHC)
		return 1;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RICOH &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RICOH_R5U822 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RICOH_R5U823))
		return 1;

	return 0;
}

void
sdhc_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct sdhc_pci_softc *sc = (struct sdhc_pci_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_intr_handle_t ih;
	char const *intrstr;
	int slotinfo;
	int nslots;
	int usedma;
	int reg;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t size;
	u_int32_t caps = 0;

	/* Some TI controllers needs special treatment. */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_TI &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_TI_PCI7XX1_SD &&
            pa->pa_function == 4)
		sdhc_takecontroller(pa);

	/* ENE controllers break if set to 0V bus power. */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_ENE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ENE_SDCARD)
		sc->sc.sc_flags |= SDHC_F_NOPWR0;

	/* Some RICOH controllers need to be bumped into the right mode. */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RICOH &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RICOH_R5U823) {
		/* Enable SD2.0 mode. */
		sdhc_pci_conf_write(pa, SDHC_PCI_MODE_KEY, 0xfc);
		sdhc_pci_conf_write(pa, SDHC_PCI_MODE, SDHC_PCI_MODE_SD20);
		sdhc_pci_conf_write(pa, SDHC_PCI_MODE_KEY, 0x00);

		/*
		 * Some SD/MMC cards don't work with the default base
		 * clock frequency of 200MHz.  Lower it to 50Hz.
		 */
		sdhc_pci_conf_write(pa, SDHC_PCI_BASE_FREQ_KEY, 0x01);
		sdhc_pci_conf_write(pa, SDHC_PCI_BASE_FREQ, 50);
		sdhc_pci_conf_write(pa, SDHC_PCI_BASE_FREQ_KEY, 0x00);
	}

	if (pci_intr_map(pa, &ih)) {
		printf(": can't map interrupt\n");
		return;
	}

	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_SDMMC,
	    sdhc_intr, sc, sc->sc.sc_dev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf(": can't establish interrupt\n");
		return;
	}
	printf(": %s\n", intrstr);

	/* Enable use of DMA if supported by the interface. */
	usedma = PCI_INTERFACE(pa->pa_class) == SDHC_PCI_INTERFACE_DMA;

	/*
	 * Map and attach all hosts supported by the host controller.
	 */
	slotinfo = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    SDHC_PCI_CONF_SLOT_INFO);
	nslots = SDHC_PCI_NUM_SLOTS(slotinfo);

	/* Allocate an array big enough to hold all the possible hosts */
	sc->sc.sc_host = malloc(sizeof(struct sdhc_host *) * nslots, M_DEVBUF,
	    M_WAITOK);

	/* XXX: handle 64-bit BARs */
	for (reg = SDHC_PCI_BAR_START + SDHC_PCI_FIRST_BAR(slotinfo) *
		 sizeof(u_int32_t);
	     reg < SDHC_PCI_BAR_END && nslots > 0;
	     reg += sizeof(u_int32_t), nslots--) {

		if (pci_mem_find(pa->pa_pc, pa->pa_tag, reg,
		    NULL, NULL, NULL) != 0)
			continue;

		if (pci_mapreg_map(pa, reg, PCI_MAPREG_TYPE_MEM, 0,
		    &iot, &ioh, NULL, &size, 0)) {
			printf("%s at 0x%x: can't map registers\n",
			    sc->sc.sc_dev.dv_xname, reg);
			continue;
		}

		if (sdhc_host_found(&sc->sc, iot, ioh, size, usedma, caps) != 0)
			/* XXX: sc->sc_host leak */
			printf("%s at 0x%x: can't initialize host\n",
			    sc->sc.sc_dev.dv_xname, reg);
	}

	/*
	 * Establish shutdown hooks.
	 */
	(void)shutdownhook_establish(sdhc_shutdown, &sc->sc);
}

void
sdhc_takecontroller(struct pci_attach_args *pa)
{
	pcitag_t tag;
	pcireg_t id, reg;

	/* Look at func 3 for the flash device */
	tag = pci_make_tag(pa->pa_pc, pa->pa_bus, pa->pa_device, 3);
	id = pci_conf_read(pa->pa_pc, tag, PCI_ID_REG);
	if (PCI_PRODUCT(id) != PCI_PRODUCT_TI_PCI7XX1_FLASH)
		return;

	/*
	 * Disable MMC/SD on the flash media controller so the
	 * SD host takes over.
	 */
	reg = pci_conf_read(pa->pa_pc, tag, SDHC_PCI_GENERAL_CTL);
	reg |= MMC_SD_DIS;
	pci_conf_write(pa->pa_pc, tag, SDHC_PCI_GENERAL_CTL, reg);
}

void
sdhc_pci_conf_write(struct pci_attach_args *pa, int reg, uint8_t val)
{
	pcireg_t tmp;

	tmp = pci_conf_read(pa->pa_pc, pa->pa_tag, reg & ~0x3);
	tmp &= ~(0xff << ((reg & 0x3) * 8));
	tmp |= (val << ((reg & 0x3) * 8));
	pci_conf_write(pa->pa_pc, pa->pa_tag, reg & ~0x3, tmp);
}
