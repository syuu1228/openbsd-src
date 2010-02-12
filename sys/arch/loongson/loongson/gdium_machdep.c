/*	$OpenBSD: src/sys/arch/loongson/loongson/gdium_machdep.c,v 1.4 2010/02/12 19:37:31 miod Exp $	*/

/*
 * Copyright (c) 2010 Miodrag Vallat.
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

/*
 * Gdium Liberty specific code and configuration data.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips64/archtype.h>
#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <loongson/dev/bonitoreg.h>
#include <loongson/dev/bonitovar.h>
#include <loongson/dev/bonito_irq.h>

int	gdium_revision = 0;

void	gdium_attach_hook(pci_chipset_tag_t);
int	gdium_intr_map(int, int, int);
void	gdium_powerdown(void);

const struct bonito_config gdium_bonito = {
	.bc_adbase = 11,

	.bc_gpioIE = LOONGSON_INTRMASK_GPIO,
	.bc_intEdge = LOONGSON_INTRMASK_PCI_SYSERR |
	    LOONGSON_INTRMASK_PCI_PARERR,
	.bc_intSteer = 0,
	.bc_intPol = LOONGSON_INTRMASK_DRAM_PARERR |
	    LOONGSON_INTRMASK_PCI_SYSERR | LOONGSON_INTRMASK_PCI_PARERR,

	.bc_legacy_pic = 0,

	.bc_attach_hook = gdium_attach_hook,
	.bc_intr_map = gdium_intr_map
};

const struct platform gdium_platform = {
	.system_type = LOONGSON_GDIUM,
	.vendor = "EMTEC",
	.product = "Gdium",

	.bonito_config = &gdium_bonito,
	.legacy_io_ranges = NULL,

	.setup = NULL,
	.powerdown = gdium_powerdown,
	.reset = NULL
};

void
gdium_attach_hook(pci_chipset_tag_t pc)
{
	pcireg_t id;
	pcitag_t tag;
#ifdef notyet
	int bar;
#endif
#if 0
	pcireg_t reg;
	int dev, func;
#endif

#ifdef notyet
	/*
	 * Clear all BAR of the mini PCI slot; PMON did not initialize
	 * it, and we do not want it to conflict with anything.
	 */
	tag = pci_make_tag(pc, 0, 13, 0);
	for (bar = PCI_MAPREG_START; bar < PCI_MAPREG_END; bar += 4)
		pci_conf_write(pc, tag, bar, 0);
#else
	/*
	 * Force a non conflicting BAR for the wireless controller,
	 * until proper resource configuration code is added to
	 * bonito (work in progress).
	 * XXX The card does not work correctly anyway at the moment.
	 */
	tag = pci_make_tag(pc, 0, 13, 0);
	pci_conf_write(pc, tag, PCI_MAPREG_START, 0x06228000);
#endif

	/*
	 * Figure out which motherboard we are running on.
	 * Might not be good enough...
	 */
	tag = pci_make_tag(pc, 0, 17, 0);
	id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id == PCI_ID_CODE(PCI_VENDOR_NEC, PCI_PRODUCT_NEC_USB))
		gdium_revision = 1;
	
#if 0
	/*
	 * Tweak the usb controller capabilities.
	 */
	for (dev = pci_bus_maxdevs(pc, 0); dev >= 0; dev--) {
		tag = pci_make_tag(pc, 0, dev, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id != PCI_ID_CODE(PCI_VENDOR_NEC, PCI_PRODUCT_NEC_USB))
			continue;
		if (gdium_revision != 0) {
			reg = pci_conf_read(pc, tag, 0xe0);
			/* enable ports 1 and 2 */
			reg |= 0x00000003;
			pci_conf_write(pc, tag, 0xe0, reg);
		} else {
			for (func = 0; func < 2; func++) {
				tag = pci_make_tag(pc, 0, dev, func);
				id = pci_conf_read(pc, tag, PCI_ID_REG);
				if (PCI_VENDOR(id) != PCI_VENDOR_NEC)
					continue;
				if (PCI_PRODUCT(id) != PCI_PRODUCT_NEC_USB &&
				    PCI_PRODUCT(id) != PCI_PRODUCT_NEC_USB2)
					continue;

				reg = pci_conf_read(pc, tag, 0xe0);
				/* enable ports 1 and 3, disable port 2 */
				reg &= ~0x00000007;
				reg |= 0x00000005;
				pci_conf_write(pc, tag, 0xe0, reg);
				pci_conf_write(pc, tag, 0xe4, 0x00000020);
			}
		}
	}
#endif
}

int
gdium_intr_map(int dev, int fn, int pin)
{
	switch (dev) {
	/* Wireless */
	case 13:	/* C D A B */
		return BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA + (pin + 1) % 4);
	/* Frame buffer */
	case 14:
		return BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA);
	/* USB */
	case 15:
		if (gdium_revision == 0)
			return BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA +
			    (pin - 1));
		else
			return BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIB);
	/* Ethernet */
	case 16:
		return BONITO_DIRECT_IRQ(LOONGSON_INTR_PCID);
	/* USB, not present in old motherboard revision */
	case 17:
		if (gdium_revision != 0)
			return BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIC);
		else
			break;
	default:
		break;
	}

	return -1;
}

void
gdium_powerdown()
{
	REGVAL(BONITO_GPIODATA) |= 0x00000002;
	REGVAL(BONITO_GPIOIE) &= ~0x00000002;
}
