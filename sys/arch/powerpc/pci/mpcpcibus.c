/*	$OpenBSD: src/sys/arch/powerpc/pci/Attic/mpcpcibus.c,v 1.25 2001/03/29 20:05:04 drahn Exp $ */

/*
 * Copyright (c) 1997 Per Fogelstrom
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
 *	This product includes software developed under OpenBSD for RTMX Inc
 *      by Per Fogelstrom, Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Generic PCI BUS Bridge driver.
 * specialized hooks for different config methods.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/bat.h>

#if 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <powerpc/pci/pcibrvar.h>
#include <powerpc/pci/mpc106reg.h>

extern vm_map_t phys_map;
extern ofw_eth_addr[];

int	 mpcpcibrmatch __P((struct device *, void *, void *));
void	 mpcpcibrattach __P((struct device *, struct device *, void *));

void	 mpc_attach_hook __P((struct device *, struct device *,
				struct pcibus_attach_args *));
int	 mpc_bus_maxdevs __P((void *, int));
pcitag_t mpc_make_tag __P((void *, int, int, int));
void	 mpc_decompose_tag __P((void *, pcitag_t, int *, int *, int *));
pcireg_t mpc_conf_read __P((void *, pcitag_t, int));
void	 mpc_conf_write __P((void *, pcitag_t, int, pcireg_t));

int      mpc_intr_map __P((void *, pcitag_t, int, int, pci_intr_handle_t *));
const char *mpc_intr_string __P((void *, pci_intr_handle_t));
void     *mpc_intr_establish __P((void *, pci_intr_handle_t,
            int, int (*func)(void *), void *, char *));
void     mpc_intr_disestablish __P((void *, void *));
int      mpc_ether_hw_addr __P((struct ppc_pci_chipset *, u_int8_t *));
int      of_ether_hw_addr __P((struct ppc_pci_chipset *, u_int8_t *));

struct cfattach mpcpcibr_ca = {
        sizeof(struct pcibr_softc), mpcpcibrmatch, mpcpcibrattach,
};

struct cfdriver mpcpcibr_cd = {
	NULL, "mpcpcibr", DV_DULL,
};

static int      mpcpcibrprint __P((void *, const char *pnp));

struct pcibr_config mpc_config;

/*
 * config types
 * bit meanings
 * 0 - standard cf8/cfc type configurations,
 *     sometimes the base addresses for these are different
 * 1 - Config Method #2 configuration - uni-north
 *
 * 2 - 64 bit config bus, data for accesses &4 is at daddr+4;
 */
struct {
	char * compat;
	u_int32_t addr;	 /* offset */
	u_int32_t data;	 /* offset */
	int config_type;
} config_offsets[] = {
	{"grackle",		0x00c00cf8, 0x00e00cfc, 0 },
	{"bandit",		0x00800000, 0x00c00000, 0 },
	{"uni-north",		0x00800000, 0x00c00000, 3 },
	{"legacy",		0x00000cf8, 0x00000cfc, 0 },
	{"IBM,27-82660",	0x00000cf8, 0x00000cfc, 0 },
	{NULL,			0x00000000, 0x00000000, 0 },
};

struct powerpc_bus_dma_tag pci_bus_dma_tag = {
	NULL,
	_dmamap_create,
	_dmamap_destroy,
	_dmamap_load,
	_dmamap_load_mbuf,
	_dmamap_load_uio,
	_dmamap_load_raw,
	_dmamap_unload,
	_dmamap_sync,
	_dmamem_alloc,
	_dmamem_free,
	_dmamem_map,
	_dmamem_unmap,
	_dmamem_mmap
};

/*
 * Code from "pci/if_de.c" used to calculate crc32 of ether rom data.
 */
#define      TULIP_CRC32_POLY  0xEDB88320UL
static __inline__ unsigned
srom_crc32(
    const unsigned char *databuf,
    size_t datalen)
{
    u_int idx, bit, data, crc = 0xFFFFFFFFUL;

    for (idx = 0; idx < datalen; idx++)
        for (data = *databuf++, bit = 0; bit < 8; bit++, data >>= 1)
            crc = (crc >> 1) ^ (((crc ^ data) & 1) ? TULIP_CRC32_POLY : 0);
    return crc;
}

int
mpcpcibrmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct confargs *ca = aux;
	int handle; 
	int found = 0;
	int err;
	unsigned int val;
	int qhandle;
	char type[32];

	if (strcmp(ca->ca_name, mpcpcibr_cd.cd_name) != 0)
		return (found);

	found = 1;

	return found;
}

void fix_node_irq(int node, struct pcibus_attach_args *pba);

int pci_map_a = 0;
void
mpcpcibrattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibr_softc *sc = (struct pcibr_softc *)self;
	struct confargs *ca = aux;
	struct pcibr_config *lcp;
	struct pcibus_attach_args pba;
	int map, node;
	char *bridge;
	int of_node = 0;
	u_int32_t base;
	u_int32_t size;

	switch(system_type) {
	case POWER4e:
		lcp = sc->sc_pcibr = &mpc_config;

		addbatmap(MPC106_V_PCI_MEM_SPACE,
			  MPC106_P_PCI_MEM_SPACE, BAT_I);

		sc->sc_membus_space.bus_base = MPC106_V_PCI_MEM_SPACE;
		sc->sc_membus_space.bus_reverse = 1;
		sc->sc_iobus_space.bus_base = MPC106_V_PCI_IO_SPACE;
		sc->sc_iobus_space.bus_reverse = 1;

		lcp->lc_pc.pc_conf_v = lcp;
		lcp->lc_pc.pc_attach_hook = mpc_attach_hook;
		lcp->lc_pc.pc_bus_maxdevs = mpc_bus_maxdevs;
		lcp->lc_pc.pc_make_tag = mpc_make_tag;
		lcp->lc_pc.pc_decompose_tag = mpc_decompose_tag;
		lcp->lc_pc.pc_conf_read = mpc_conf_read;
		lcp->lc_pc.pc_conf_write = mpc_conf_write;
		lcp->lc_pc.pc_ether_hw_addr = mpc_ether_hw_addr;
		lcp->lc_iot = &sc->sc_iobus_space;
		lcp->lc_memt = &sc->sc_membus_space;

	        lcp->lc_pc.pc_intr_v = lcp;
		lcp->lc_pc.pc_intr_map = mpc_intr_map;
		lcp->lc_pc.pc_intr_string = mpc_intr_string;
		lcp->lc_pc.pc_intr_establish = mpc_intr_establish;
		lcp->lc_pc.pc_intr_disestablish = mpc_intr_disestablish;

		printf(": MPC106, Revision 0x%x.\n", 
				mpc_cfg_read_1(lcp, MPC106_PCI_REVID));
#if 0
		mpc_cfg_write_2(lcp, MPC106_PCI_STAT, 0xff80); /* Reset status */
#endif
		bridge = "MPC106";
		break;

	case OFWMACH:
	case PWRSTK:
		{
			int handle; 
			int err;
			unsigned int val;
			handle = ppc_open_pci_bridge();
			/* if open fails something odd has happened,
			 * we did this before during probe...
			 */
			err = OF_call_method("config-l@", handle, 1, 1,
				0x80000000, &val);
			if (err == 0) {
				switch (val) {
				/* supported ppc-pci bridges */
				case (PCI_VENDOR_MOT | ( PCI_PRODUCT_MOT_MPC105 <<16)):
					bridge = "MPC105";
					break;
				case (PCI_VENDOR_MOT | ( PCI_PRODUCT_MOT_MPC106 <<16)):
					bridge = "MPC106";
					break;
				default:
					;
				}

			}
			
			/* read the PICR1 register to find what 
			 * address map is being used
			 */
			err = OF_call_method("config-l@", handle, 1, 1,
				0x800000a8, &val);
			if (val & 0x00010000) {
				map = 1; /* map A */
				pci_map_a = 1;
			} else {
				map = 0; /* map B */
				pci_map_a = 0;
			}

			ppc_close_pci_bridge(handle);
		}
		if (map == 1) {
			sc->sc_membus_space.bus_base = MPC106_P_PCI_MEM_SPACE;
			sc->sc_membus_space.bus_reverse = 1;
			sc->sc_iobus_space.bus_base = MPC106_P_PCI_IO_SPACE;
			sc->sc_iobus_space.bus_reverse = 1;
			if ( bus_space_map(&(sc->sc_iobus_space), 0, NBPG, 0,
				&lcp->ioh_cf8) != 0 )
			{
				panic("mpcpcibus: unable to map self\n");
			}
			lcp->ioh_cfc = lcp->ioh_cf8;
		} else {
			sc->sc_membus_space.bus_base =
				MPC106_P_PCI_MEM_SPACE_MAP_B;
			sc->sc_membus_space.bus_reverse = 1;
			sc->sc_iobus_space.bus_base =
				MPC106_P_PCI_IO_SPACE_MAP_B;
			sc->sc_iobus_space.bus_reverse = 1;
			if ( bus_space_map(&(sc->sc_iobus_space), 0xfec00000,
				NBPG, 0, &lcp->ioh_cf8) != 0 )
			{
				panic("mpcpcibus: unable to map self\n");
			}
			if ( bus_space_map(&(sc->sc_iobus_space), 0xfee00000,
				NBPG, 0, &lcp->ioh_cfc) != 0 )
			{
				panic("mpcpcibus: unable to map self\n");
			}
		}

		lcp->lc_pc.pc_conf_v = lcp;
		lcp->lc_pc.pc_attach_hook = mpc_attach_hook;
		lcp->lc_pc.pc_bus_maxdevs = mpc_bus_maxdevs;
		lcp->lc_pc.pc_make_tag = mpc_make_tag;
		lcp->lc_pc.pc_decompose_tag = mpc_decompose_tag;
		lcp->lc_pc.pc_conf_read = mpc_conf_read;
		lcp->lc_pc.pc_conf_write = mpc_conf_write;
		lcp->lc_pc.pc_ether_hw_addr = mpc_ether_hw_addr;
		lcp->lc_iot = &sc->sc_iobus_space;
		lcp->lc_memt = &sc->sc_membus_space;

	        lcp->lc_pc.pc_intr_v = lcp;
		lcp->lc_pc.pc_intr_map = mpc_intr_map;
		lcp->lc_pc.pc_intr_string = mpc_intr_string;
		lcp->lc_pc.pc_intr_establish = mpc_intr_establish;
		lcp->lc_pc.pc_intr_disestablish = mpc_intr_disestablish;


		printf(": %s, Revision 0x%x, ", bridge, 
			mpc_cfg_read_1(lcp, MPC106_PCI_REVID));
		if (map == 1) {
			printf("Using Map A\n");
		} else  {
			printf("Using Map B\n");
		}
#if 0
		/* Reset status */
		mpc_cfg_write_2(lcp, MPC106_PCI_STAT, 0xff80);
#endif
		break;

	case APPL:
		/* scan the children of the root of the openfirmware
		 * tree to locate all nodes with device_type of "pci"
		 */

		if (ca->ca_node == 0) {
			printf("invalid node on mpcpcibr config\n");
			return;
		}
		{
			char compat[32];
			u_int32_t addr_offset;
			u_int32_t data_offset;
			struct pci_reserve_mem null_reserve = {
				0,
				0,
				0
			};
			int i;
			int len;
			int rangelen;

			struct ranges_new {
				u_int32_t flags;
				u_int32_t pad1;
				u_int32_t pad2;
				u_int32_t base;
				u_int32_t pad3;
				u_int32_t size;
			};
			u_int32_t range_store[32];
			struct ranges_new *prange = (void *)&range_store;

			len=OF_getprop(ca->ca_node, "compatible", compat,
				sizeof (compat));
			if (len <= 0 ) {
				len=OF_getprop(ca->ca_node, "name", compat,
					sizeof (compat));
				if (len <= 0) {
					printf(" compatible and name not"
						" found\n");
					return;
				}
				compat[len] = 0; 
				if (strcmp (compat, "bandit") != 0) {
					printf(" compatible not found and name"
						" %s found\n", compat);
					return;
				}
			}
			compat[len] = 0; 
			if ((rangelen = OF_getprop(ca->ca_node, "ranges",
				range_store,
				sizeof (range_store))) <= 0)
			{
				printf("range lookup failed, node %x\n",
				ca->ca_node);
			}
			/* translate byte(s) into item count/*/
			rangelen /= sizeof(struct ranges_new);

			lcp = sc->sc_pcibr = &sc->pcibr_config;

			{
				int found;
				unsigned int base = 0;
				unsigned int size = 0;

				/* mac configs */

				sc->sc_membus_space.bus_base = 0;
				sc->sc_membus_space.bus_reverse = 1;
				sc->sc_iobus_space.bus_base = 0;
				sc->sc_iobus_space.bus_reverse = 1;

				/* find io(config) base, flag == 0x01000000 */
				found = 0;
				for (i = 0; i < rangelen ; i++)
				{
					if (prange[i].flags == 0x01000000) {
						/* find last? */
						found = i;
					}
				}
				/* found the io space ranges */
				if (prange[found].flags == 0x01000000) {
					sc->sc_iobus_space.bus_base =
						prange[found].base;
					sc->sc_iobus_space.bus_size =
						prange[found].size;
				}

				/* the mem space ranges 
				 * apple openfirmware always puts full
				 * addresses in config information,
				 * it is not necessary to have correct bus
				 * base address, but since 0 is reserved
				 * and all IO and device memory will be in
				 * upper 2G of address space, set to
				 * 0x80000000
				 * start with segment 1 not 0, 0 is config.
				 */
				for (i = 0; i < rangelen ; i++)
				{
					if (prange[i].flags == 0x02000000) {
#if 0
						printf("\nfound mem %x %x",
							prange[i].base,
							prange[i].size);
#endif
							
						if (base != 0) {
							if ((base + size) ==
							    prange[i].base)
							{
							    size +=
								prange[i].size;
							} else {
								base =
								 prange[i].base;
								size =
								 prange[i].size;
							}
						} else {
							base = prange[i].base;
							size = prange[i].size;
						}
					}
				}
				sc->sc_membus_space.bus_base = base;
				sc->sc_membus_space.bus_size = size;

			}
#if 0
			printf("membase %x size %x iobase %x size %x\n",
				sc->sc_membus_space.bus_base,
				sc->sc_membus_space.bus_size,
				sc->sc_iobus_space.bus_base,
				sc->sc_iobus_space.bus_size);
#endif
			
			addr_offset = 0;
			for (i = 0; config_offsets[i].compat != NULL; i++) {
				if (strcmp(config_offsets[i].compat, compat)
					== 0)
				{
					addr_offset = config_offsets[i].addr; 
					data_offset = config_offsets[i].data; 
					lcp->config_type =
						config_offsets[i].config_type;
					break;
				}
			}
			if (addr_offset == 0) {
				printf("unable to find match for"
					" compatible %s\n", compat);
				return;
			}
#if 0
			printf(" mem base %x io base %x config addr %x"
				" config data %x\n",
				sc->sc_membus_space.bus_base,
				sc->sc_iobus_space.bus_base,
				addr_offset, data_offset);
#endif



			if ( bus_space_map(&(sc->sc_iobus_space), addr_offset,
				NBPG, 0, &lcp->ioh_cf8) != 0 )
			{
				panic("mpcpcibus: unable to map self\n");
			}
			if ( bus_space_map(&(sc->sc_iobus_space), data_offset,
				NBPG, 0, &lcp->ioh_cfc) != 0 )
			{
				panic("mpcpcibus: unable to map self\n");
			}
			of_node = ca->ca_node;


			lcp->node = ca->ca_node;
			lcp->lc_pc.pc_conf_v = lcp;
			lcp->lc_pc.pc_attach_hook = mpc_attach_hook;
			lcp->lc_pc.pc_bus_maxdevs = mpc_bus_maxdevs;
			lcp->lc_pc.pc_make_tag = mpc_make_tag;
			lcp->lc_pc.pc_decompose_tag = mpc_decompose_tag;
			lcp->lc_pc.pc_conf_read = mpc_conf_read;
			lcp->lc_pc.pc_conf_write = mpc_conf_write;
			lcp->lc_pc.pc_ether_hw_addr = of_ether_hw_addr;
			lcp->lc_iot = &sc->sc_iobus_space;
			lcp->lc_memt = &sc->sc_membus_space;

			lcp->lc_pc.pc_intr_v = lcp;
			lcp->lc_pc.pc_intr_map = mpc_intr_map;
			lcp->lc_pc.pc_intr_string = mpc_intr_string;
			lcp->lc_pc.pc_intr_establish = mpc_intr_establish;
			lcp->lc_pc.pc_intr_disestablish = mpc_intr_disestablish;

			printf(": %s, Revision 0x%x\n", compat, 
				mpc_cfg_read_1(lcp, MPC106_PCI_REVID));

#if 0
			pci_addr_fixup(sc, &lcp->lc_pc, 32, &null_reserve);
#endif
		}
		break;

	default:
		printf("unknown system_type %d\n",system_type);
		return;
	}

	pba.pba_dmat = &pci_bus_dma_tag;
		

	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_iobus_space;
	pba.pba_memt = &sc->sc_membus_space;
	pba.pba_pc = &lcp->lc_pc;
	pba.pba_bus = 0;

	/* we want to check pci irq settings */
	if (of_node != 0) {
		int nn;

		for (node = OF_child(of_node); node; node = nn)
		{
			{
				char name[32];
				int len;
				len = OF_getprop(node, "name", name,
					sizeof(name));
				name[len] = 0;
#if 0
				printf("checking node %s\n", name);
#endif
			}
			fix_node_irq(node, &pba);

			/* iterate section */
			if ((nn = OF_child(node)) != 0) {
				continue;
			}
			while ((nn = OF_peer(node)) == 0) {
				node = OF_parent(node);
				if (node == of_node) {
					nn = 0; /* done */
					break;
				}
			}
		}
	}

	config_found(self, &pba, mpcpcibrprint);

}
                 
#define       OFW_PCI_PHYS_HI_BUSMASK         0x00ff0000
#define       OFW_PCI_PHYS_HI_BUSSHIFT        16
#define       OFW_PCI_PHYS_HI_DEVICEMASK      0x0000f800
#define       OFW_PCI_PHYS_HI_DEVICESHIFT     11
#define       OFW_PCI_PHYS_HI_FUNCTIONMASK    0x00000700
#define       OFW_PCI_PHYS_HI_FUNCTIONSHIFT   8

#define pcibus(x) \
	(((x) & OFW_PCI_PHYS_HI_BUSMASK) >> OFW_PCI_PHYS_HI_BUSSHIFT)
#define pcidev(x) \
	(((x) & OFW_PCI_PHYS_HI_DEVICEMASK) >> OFW_PCI_PHYS_HI_DEVICESHIFT)
#define pcifunc(x) \
	(((x) & OFW_PCI_PHYS_HI_FUNCTIONMASK) >> OFW_PCI_PHYS_HI_FUNCTIONSHIFT)

/* 
 * Find PCI IRQ from OF
 */
int
find_node_intr(node, addr, intr)
	int node;
	u_int32_t *addr, *intr;
{
	int parent, iparent, len, mlen;
	int match, i;
	u_int32_t map[64], *mp;
	u_int32_t imask[8], maskedaddr[8];
	u_int32_t icells;
	char name [32];

	len = OF_getprop(node, "AAPL,interrupts", intr, 4);
	if (len == 4)
		return 1;

	parent = OF_parent(node);
	len = OF_getprop(parent, "interrupt-map", map, sizeof(map));
	mlen = OF_getprop(parent, "interrupt-map-mask", imask, sizeof(imask));

	if ((len == -1) || (mlen == -1))
		goto nomap;
	for (i = 0; i < (mlen / 4); i++) {
		maskedaddr[i] = addr[i] & imask[i];
	}
	mp = map;
	while (len > mlen) {
		match = bcmp(maskedaddr, mp, mlen);
		mp += mlen / 4;
		len -= mlen;
		iparent = *mp++;
		if (OF_getprop(iparent, "#interrupt-cells", &icells, 4) != 4)
			return -1;

		if (match == 0) {
			/* multiple irqs? */
			*intr = *mp;
			return 1;
		}
		mp += icells;
		len -= icells * 4;
	}
	return -1;
nomap:
	return -1;
}

void
fix_node_irq(node, pba)
	int node;
	struct pcibus_attach_args *pba;
{
	struct { 
		u_int32_t phys_hi, phys_mid, phys_lo;
		u_int32_t size_hi, size_lo;
	} addr [8];
	int len;
	pcitag_t tag;
	u_int32_t irq;
	u_int32_t intr;

	pci_chipset_tag_t pc = pba->pba_pc;

	len = OF_getprop(node, "assigned-addresses", addr, sizeof(addr));
	if (len < sizeof(addr[0])) {
		return;
	}
	tag = pci_make_tag(pc, pcibus(addr[0].phys_hi),
		pcidev(addr[0].phys_hi),
		pcifunc(addr[0].phys_hi));
	/* program the interrupt line register with the value
	 * found in openfirmware
	 */
	if (find_node_intr(node, &addr[0].phys_hi, &irq) == -1)
		return;

	intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
#if 0
	printf("changing interrupt from %d to %d\n",
		intr & PCI_INTERRUPT_LINE_MASK,
		irq & PCI_INTERRUPT_LINE_MASK);
#endif
	intr &= ~PCI_INTERRUPT_LINE_MASK;
	intr |= irq & PCI_INTERRUPT_LINE_MASK;
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
}

static int
mpcpcibrprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pba = aux;

	if(pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return(UNCONF);
}

/*
 *  Get PCI physical address from given viritual address.
 *  XXX Note that cross page boundarys are *not* garantueed to work!
 */

vm_offset_t
vtophys(p)
	void *p;
{
	vm_offset_t pa;
	vm_offset_t va;

	va = (vm_offset_t)p;
	if((vm_offset_t)va < VM_MIN_KERNEL_ADDRESS) {
		pa = va;
	}
	else {
		pa = pmap_extract(vm_map_pmap(phys_map), va);
	}
	return(pa | ((pci_map_a == 1) ? MPC106_PCI_CPUMEM : 0 ));
}

void
mpc_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
}

int
of_ether_hw_addr(struct ppc_pci_chipset *lcpc, u_int8_t *oaddr)
{
	u_int8_t laddr[6];
	struct pcibr_config *lcp = lcpc->pc_conf_v;
	int of_node = lcp->node;
	int node, nn;
	for (node = OF_child(of_node); node; node = nn)
	{
		char name[32];
		int len;
		len = OF_getprop(node, "name", name,
			sizeof(name));
		name[len] = 0;
		if (sizeof (laddr) ==
			OF_getprop(node, "local-mac-address", laddr,
				sizeof laddr))
		{
			bcopy (laddr, oaddr, sizeof laddr);
			return 1;
			
		}

		/* iterate section */
		if ((nn = OF_child(node)) != 0) {
			continue;
		}
		while ((nn = OF_peer(node)) == 0) {
			node = OF_parent(node);
			if (node == of_node) {
				nn = 0; /* done */
				break;
			}
		}
	}
	oaddr[0] = oaddr[1] = oaddr[2] = 0xff;
	oaddr[3] = oaddr[4] = oaddr[5] = 0xff;
	return 0;
}

int
mpc_ether_hw_addr(p, s)
	struct ppc_pci_chipset *p;
	u_int8_t *s;
{
	printf("mpc_ether_hw_addr not supported\n");
	return(0);
}

int
mpc_bus_maxdevs(cpv, busno)
	void *cpv;
	int busno;
{
	return(32);
}

#define BUS_SHIFT 16
#define DEVICE_SHIFT 11
#define FNC_SHIFT 8

pcitag_t
mpc_make_tag(cpv, bus, dev, fnc)
	void *cpv;
	int bus, dev, fnc;
{
	return (bus << BUS_SHIFT) | (dev << DEVICE_SHIFT) | (fnc << FNC_SHIFT);
}

void
mpc_decompose_tag(cpv, tag, busp, devp, fncp)
	void *cpv;
	pcitag_t tag;
	int *busp, *devp, *fncp;
{
	if (busp != NULL)
		*busp = (tag >> BUS_SHIFT) & 0xff;
	if (devp != NULL)
		*devp = (tag >> DEVICE_SHIFT) & 0x1f;
	if (fncp != NULL)
		*fncp = (tag >> FNC_SHIFT) & 0x7;
}

static u_int32_t
mpc_gen_config_reg(cpv, tag, offset)
	void *cpv;
	pcitag_t tag;
	int offset;
{
	struct pcibr_config *cp = cpv;
	unsigned int bus, dev, fcn;
	u_int32_t reg;
	/*
	static int spin = 0;
	while (spin > 85);
	spin++;
	*/

	mpc_decompose_tag(cpv, tag, &bus, &dev, &fcn);

	if (cp->config_type & 1) {
		/* Config Mechanism #2 */
		if (bus == 0) {
			if (dev < 11) {
				return 0xffffffff;
			}
			/*
			 * Need to do config type 0 operation
			 *  1 << (11?+dev) | fcn << 8 | reg
			 * 11? is because pci spec states
			 * that 11-15 is reserved.
			 */
			reg = 1 << (dev) | fcn << 8 | offset;
			
		} else {
			if (dev > 15) {
			 return 0xffffffff;
			}
			/*
			 * config type 1 
			 */
			reg =  tag  | offset | 1;

		}
	} else {
		/* config mechanism #2, type 0
		/* standard cf8/cfc config */
		reg =  0x80000000 | tag  | offset;

	}
	return reg;
}

/* #define DEBUG_CONFIG  */
pcireg_t
mpc_conf_read(cpv, tag, offset)
	void *cpv;
	pcitag_t tag;
	int offset;
{
	struct pcibr_config *cp = cpv;

	pcireg_t data;
	u_int32_t reg;
	int device;
	int s;
	int handle; 
	int daddr = 0;

	if(offset & 3 || offset < 0 || offset >= 0x100) {
#ifdef DEBUG_CONFIG 
		printf ("pci_conf_read: bad reg %x\n", offset);
#endif /* DEBUG_CONFIG */
		return(~0);
	}

	reg = mpc_gen_config_reg(cpv, tag, offset);
	/* if invalid tag, return -1 */
	if (reg == 0xffffffff) {
		return 0xffffffff;
	}

	if ((cp->config_type & 2) && (offset & 0x04)) {
		daddr += 4;
	}

	s = splhigh();

	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, reg);
	bus_space_read_4(cp->lc_iot, cp->ioh_cf8, 0); /* XXX */
	data = bus_space_read_4(cp->lc_iot, cp->ioh_cfc, daddr);
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, 0); /* disable */
	bus_space_read_4(cp->lc_iot, cp->ioh_cf8, 0); /* XXX */

	splx(s);
#ifdef DEBUG_CONFIG
	if (!((offset == 0) && (data == 0xffffffff))) {
		unsigned int bus, dev, fcn;
		mpc_decompose_tag(cpv, tag, &bus, &dev, &fcn);
		printf("mpc_conf_read bus %x dev %x fcn %x offset %x", bus, dev, fcn,
			offset);
		printf(" daddr %x reg %x",daddr, reg);
		printf(" data %x\n", data);
	}
#endif

	return(data);
}

void
mpc_conf_write(cpv, tag, offset, data)
	void *cpv;
	pcitag_t tag;
	int offset;
	pcireg_t data;
{
	struct pcibr_config *cp = cpv;
	u_int32_t reg;
	int s;
	int handle; 
	int daddr = 0;

	reg = mpc_gen_config_reg(cpv, tag, offset);

	/* if invalid tag, return ??? */
	if (reg == 0xffffffff) {
		return;
	}
	if ((cp->config_type & 2) && (offset & 0x04)) {
		daddr += 4;
	}
#ifdef DEBUG_CONFIG
	{
		unsigned int bus, dev, fcn;
		mpc_decompose_tag(cpv, tag, &bus, &dev, &fcn);
		printf("mpc_conf_write bus %x dev %x fcn %x offset %x", bus,
			dev, fcn, offset);
		printf(" daddr %x reg %x",daddr, reg);
		printf(" data %x\n", data);
	}
#endif

	s = splhigh();

	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, reg);
	bus_space_read_4(cp->lc_iot, cp->ioh_cf8, 0); /* XXX */
	bus_space_write_4(cp->lc_iot, cp->ioh_cfc, daddr, data);
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, 0); /* disable */
	bus_space_read_4(cp->lc_iot, cp->ioh_cf8, 0); /* XXX */

	splx(s);
}


int
mpc_intr_map(lcv, bustag, buspin, line, ihp)
	void *lcv;
	pcitag_t bustag;
	int buspin, line;
	pci_intr_handle_t *ihp;
{
	struct pcibr_config *lcp = lcv;
	pci_chipset_tag_t pc = &lcp->lc_pc; 
	int error = 0;
	int route;
	int lvl;
	int device;

	*ihp = -1;
        if (buspin == 0) {
                /* No IRQ used. */
                error = 1;
        }
        else if (buspin > 4) {
                printf("mpc_intr_map: bad interrupt pin %d\n", buspin);
                error = 1;
        }

#if 0
	/* this hack belongs elsewhere */
	if(system_type == POWER4e) {
		pci_decompose_tag(pc, bustag, NULL, &device, NULL);
		route = in32rb(MPC106_PCI_CONF_SPACE + 0x860);
		switch(device) {
		case 1:			/* SCSI */
			line = 6;
			route &= ~0x0000ff00;
			route |= line << 8;
			break;

		case 2:			/* Ethernet */
			line = 14;
			route &= ~0x00ff0000;
			route |= line << 16;
			break;

		case 3:			/* Tundra VME */
			line = 15;
			route &= ~0xff000000;
			route |= line << 24;
			break;

		case 4:			/* PMC Slot */
			line = 9;
			route &= ~0x000000ff;
			route |= line;
			break;

		default:
			printf("mpc_intr_map: bad dev slot %d!\n", device);
			error = 1;
			break;
		}

		lvl = isa_inb(0x04d0);
		lvl |= isa_inb(0x04d1) << 8;
		lvl |= 1L << line;
		isa_outb(0x04d0, lvl);
		isa_outb(0x04d1, lvl >> 8);
		out32rb(MPC106_PCI_CONF_SPACE + 0x860, route);
	}
#endif

	if(!error)
		*ihp = line;
	return error;
}

const char *
mpc_intr_string(lcv, ih)
	void *lcv;
	pci_intr_handle_t ih;
{
	static char str[16];

	sprintf(str, "irq %d", ih);
	return(str);
}

typedef void     *(intr_establish_t) __P((void *, pci_intr_handle_t,
            int, int, int (*func)(void *), void *, char *));
typedef void     (intr_disestablish_t) __P((void *, void *));
extern intr_establish_t *intr_establish_func;
extern intr_disestablish_t *intr_disestablish_func;

void *
mpc_intr_establish(lcv, ih, level, func, arg, name)
	void *lcv;
	pci_intr_handle_t ih;
	int level;
	int (*func) __P((void *));
	void *arg;
	char *name;
{
	return (*intr_establish_func)(lcv, ih, IST_LEVEL, level, func, arg,
		name);
#if 0
	return isabr_intr_establish(NULL, ih, IST_LEVEL, level, func, arg,
		name);
#endif
}

void
mpc_intr_disestablish(lcv, cookie)
	void *lcv, *cookie;
{
	/* XXX We should probably do something clever here.... later */
}

#if 0
void
mpc_print_pci_stat()
{
	u_int32_t stat;

	stat = mpc_cfg_read_4(cp, MPC106_PCI_CMD);
	printf("pci: status 0x%08x.\n", stat);
	stat = mpc_cfg_read_2(cp, MPC106_PCI_STAT);
	printf("pci: status 0x%04x.\n", stat);
}
#endif
u_int32_t
pci_iack()
{
	/* do pci IACK cycle */
	/* this should be bus allocated. */
	volatile u_int8_t *iack = (u_int8_t *)0xbffffff0;
	u_int8_t val;

	val = *iack;
	return val;
}

void
mpc_cfg_write_1(cp, reg, val)
	struct pcibr_config *cp;
	u_int32_t reg;
	u_int8_t val;
{
	int s;
	s = splhigh();
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0,
		MPC106_REGOFFS(reg));
	bus_space_write_1(cp->lc_iot, cp->ioh_cfc, 0, val);
	splx(s);
}

void
mpc_cfg_write_2(cp, reg, val)
	struct pcibr_config *cp;
	u_int32_t reg;
	u_int16_t val;
{
	int s;
	s = splhigh();
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, MPC106_REGOFFS(reg));
	bus_space_write_2(cp->lc_iot, cp->ioh_cfc, 0, val);
	splx(s);
}

void
mpc_cfg_write_4(cp, reg, val)
	struct pcibr_config *cp;
	u_int32_t reg;
	u_int32_t val;
{

	int s;
	s = splhigh();
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, MPC106_REGOFFS(reg));
	bus_space_write_4(cp->lc_iot, cp->ioh_cfc, 0, val);
	splx(s);
}

u_int8_t
mpc_cfg_read_1(cp, reg)
	struct pcibr_config *cp;
	u_int32_t reg;
{
	u_int8_t _v_;

	int s;
	s = splhigh();
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, MPC106_REGOFFS(reg));
	_v_ = bus_space_read_1(cp->lc_iot, cp->ioh_cfc, 0);
	splx(s);
	return(_v_);
}

u_int16_t
mpc_cfg_read_2(cp, reg)
	struct pcibr_config *cp;
	u_int32_t reg;
{
	u_int16_t _v_;

	int s;
	s = splhigh();
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, MPC106_REGOFFS(reg));
	_v_ = bus_space_read_2(cp->lc_iot, cp->ioh_cfc, 0);
	splx(s);
	return(_v_);
}

u_int32_t
mpc_cfg_read_4(cp, reg)
	struct pcibr_config *cp;
	u_int32_t reg;
{
	u_int32_t _v_;

	int s;
	s = splhigh();
	bus_space_write_4(cp->lc_iot, cp->ioh_cf8, 0, MPC106_REGOFFS(reg));
	_v_ = bus_space_read_4(cp->lc_iot, cp->ioh_cfc, 0);
	splx(s);
	return(_v_);
}
