/*	$OpenBSD: src/sys/dev/acpi/acpiprt.c,v 1.3 2006/11/27 12:39:04 kettenis Exp $	*/
/*
 * Copyright (c) 2006 Mark Kettenis <kettenis@openbsd.org>
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
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpidev.h>
#include <dev/acpi/amltypes.h>
#include <dev/acpi/dsdt.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>

#include <machine/i82093reg.h>
#include <machine/i82093var.h>

#include <machine/mpbiosvar.h>

int	acpiprt_match(struct device *, void *, void *);
void	acpiprt_attach(struct device *, struct device *, void *);

struct acpiprt_softc {
	struct device		sc_dev;

	struct acpi_softc	*sc_acpi;
	struct aml_node		*sc_devnode;

	int			sc_bus;
};

struct cfattach acpiprt_ca = {
	sizeof(struct acpiprt_softc), acpiprt_match, acpiprt_attach
};

struct cfdriver acpiprt_cd = {
	NULL, "acpiprt", DV_DULL
};

void	acpiprt_prt_add(struct acpiprt_softc *, struct aml_value *);
int	acpiprt_getpcibus(struct acpiprt_softc *, struct aml_node *);

int
acpiprt_match(struct device *parent, void *match, void *aux)
{
	struct acpi_attach_args	*aa = aux;
	struct cfdata  *cf = match;

	/* sanity */
	if (aa->aaa_name == NULL ||
	    strcmp(aa->aaa_name, cf->cf_driver->cd_name) != 0 ||
	    aa->aaa_table != NULL)
		return (0);

	return (1);
}

void
acpiprt_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpiprt_softc *sc = (struct acpiprt_softc *)self;
	struct acpi_attach_args *aa = aux;
	struct aml_value res;
	int i;

	sc->sc_acpi = (struct acpi_softc *)parent;
	sc->sc_devnode = aa->aaa_node->child;
	sc->sc_bus = acpiprt_getpcibus(sc, sc->sc_devnode);

	printf(": bus %d (%s)", sc->sc_bus, sc->sc_devnode->parent->name);

	if (aml_evalname(sc->sc_acpi, sc->sc_devnode, "_PRT", 0, NULL, &res)) {
		printf(": no PCI interrupt routing table\n");
		return;
	}

	if (res.type != AML_OBJTYPE_PACKAGE) {
		printf(": _PRT is not a package\n");
		return;
	}

	printf("\n");

	for (i = 0; i < res.length; i++)
		acpiprt_prt_add(sc, res.v_package[i]);
}

void
acpiprt_prt_add(struct acpiprt_softc *sc, struct aml_value *v)
{
	struct aml_node	*node;
	struct aml_value res;
	int addr, pin, irq;
	struct mp_intr_map *map;
	struct ioapic_softc *apic;

	if (v->type != AML_OBJTYPE_PACKAGE || v->length != 4) {
		printf("invalid mapping object\n");
		return;
	}

	addr = aml_val2int(v->v_package[0]);
	pin = aml_val2int(v->v_package[1]);

	if (v->v_package[2]->type == AML_OBJTYPE_NAMEREF ||
	    v->v_package[2]->type == AML_OBJTYPE_OBJREF) {
		if (v->v_package[2]->type == AML_OBJTYPE_NAMEREF)
			node = aml_searchname(sc->sc_devnode,
			    v->v_package[2]->v_nameref);
		else
			node = v->v_package[2]->v_objref.ref->node;
		if (node == NULL) {
			printf(" invalid node!\n");
			return;
		}

		if (aml_evalname(sc->sc_acpi, node, "_STA", 0, NULL, NULL)) {
			printf("no _STA method\n");
		}

		if (aml_evalname(sc->sc_acpi, node, "_CRS", 0, NULL, &res)) {
			printf("no _CRS method\n");
		}

		if (res.type != AML_OBJTYPE_BUFFER || res.length < 6) {
			printf("invalid _CRS object\n");
			return;
		}

		if ((res.v_buffer[0] >> 3) == 0x4) {
			irq = res.v_buffer[1] + (res.v_buffer[2] << 8);
			irq = ffs(irq) - 1;
		} else {
			printf("unexpected _CSR object\n");
			return;
		}
	} else
		irq = aml_val2int(v->v_package[3]);

	apic = ioapic_find_bybase(irq);

	map = malloc(sizeof (struct mp_intr_map), M_DEVBUF, M_NOWAIT);
	if (map == NULL)
		return;

#if ACPI_DEBUG
	printf("%s: addr 0x%x pin %d irq %d\n", DEVNAME(sc), addr, pin, irq);
#endif

	memset(map, 0, sizeof *map);
	map->ioapic = apic;
	map->ioapic_pin = irq - apic->sc_apic_vecbase;
	map->bus_pin = ((addr >> 14) & 0x7c) | (pin & 0x3);
	map->redir = IOAPIC_REDLO_ACTLO | IOAPIC_REDLO_LEVEL;
	map->redir |= (IOAPIC_REDLO_DEL_LOPRI << IOAPIC_REDLO_DEL_SHIFT);

	map->ioapic_ih = APIC_INT_VIA_APIC |
	    ((apic->sc_apicid << APIC_INT_APIC_SHIFT) |
	    (map->ioapic_pin << APIC_INT_PIN_SHIFT));

	apic->sc_pins[map->ioapic_pin].ip_map = map;

	map->next = mp_busses[sc->sc_bus].mb_intrs;
	mp_busses[sc->sc_bus].mb_intrs = map;
}

int
acpiprt_getpcibus(struct acpiprt_softc *sc, struct aml_node *node)
{
	struct aml_node *parent = node->parent;
	struct aml_value res;
	pci_chipset_tag_t pc = NULL;
	pcitag_t tag;
	pcireg_t reg;
	int bus, dev, func;

	if (parent == NULL)
		return 0;

	if (aml_evalname(sc->sc_acpi, parent, "_ADR", 0, NULL, &res) == 0) {
		bus = acpiprt_getpcibus(sc, parent);
		dev = ACPI_PCI_DEV(aml_val2int(&res) << 16);
		func = ACPI_PCI_FN(aml_val2int(&res) << 16);
		tag = pci_make_tag(pc, bus, dev, func);

		reg = pci_conf_read(pc, tag, PCI_CLASS_REG);
		if (PCI_CLASS(reg) == PCI_CLASS_BRIDGE &&
		    PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_PCI) {
			reg = pci_conf_read(pc, tag, PPB_REG_BUSINFO);
			return PPB_BUSINFO_SECONDARY(reg);
		}
	}

	if (aml_evalname(sc->sc_acpi, parent, "_BBN", 0, NULL, &res) == 0) {
		return aml_val2int(&res);
	}

	return 0;
}
