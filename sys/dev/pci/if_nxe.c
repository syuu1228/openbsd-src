/*	$OpenBSD: src/sys/dev/pci/if_nxe.c,v 1.17 2007/08/15 00:54:41 dlg Exp $ */

/*
 * Copyright (c) 2007 David Gwynne <dlg@openbsd.org>
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

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#ifdef NXE_DEBUG
int nxedebug = 0;

#define DPRINTF(l, f...)	do { if (nxedebug & (l)) printf(f); } while (0)
#define DASSERT(_a)		assert(_a)
#else
#define DPRINTF(l, f...)
#define DASSERT(_a)
#endif


/*
 * PCI configuration space registers
 */

#define NXE_PCI_BAR_MEM		0x10 /* bar 0 */
#define NXE_PCI_BAR_MEM_128MB		(128 * 1024 * 1024)
#define NXE_PCI_BAR_DOORBELL	0x20 /* bar 4 */

/*
 * doorbell register space
 */

#define NXE_DB			0x00000000
#define  NXE_DB_PEGID			0x00000003
#define  NXE_DB_PEGID_RX		0x00000001 /* rx unit */
#define  NXE_DB_PEGID_TX		0x00000002 /* tx unit */
#define  NXE_DB_PRIVID			0x00000004 /* must be set */
#define  NXE_DB_COUNT(_c)		((_c)<<3) /* count */
#define  NXE_DB_CTXID(_c)		((_c)<<18) /* context id */
#define  NXE_DB_OPCODE_RX_PROD		0x00000000
#define  NXE_DB_OPCODE_RX_JUMBO_PROD	0x10000000
#define  NXE_DB_OPCODE_RX_LRO_PROD	0x20000000
#define  NXE_DB_OPCODE_CMD_PROD		0x30000000
#define  NXE_DB_OPCODE_UPD_CONS		0x40000000
#define  NXE_DB_OPCODE_RESET_CTX	0x50000000

/*
 * register space
 */

/* different PCI functions use different registers sometimes */
#define _F(_f)			((_f) * 0x20)

/*
 * driver ref section 4.2
 *
 * All the hardware registers are mapped in memory. Apart from the registers
 * for the individual hardware blocks, the memory map includes a large number
 * of software definable registers.
 *
 * The following table gives the memory map in the PCI address space.
 */

#define NXE_MAP_DDR_NET		0x00000000
#define NXE_MAP_DDR_MD		0x02000000
#define NXE_MAP_QDR_NET		0x04000000
#define NXE_MAP_DIRECT_CRB	0x04400000
#define NXE_MAP_OCM0		0x05000000
#define NXE_MAP_OCM1		0x05100000
#define NXE_MAP_CRB		0x06000000

/*
 * Since there are a large number of registers they do not fit in a single
 * PCI addressing range. Hence two windows are defined. The window starts at
 * NXE_MAP_CRB, and extends to the end of the register map. The window is set
 * using the NXE_REG_WINDOW_CRB register. The format of the NXE_REG_WINDOW_CRB
 * register is as follows:
 */

#define NXE_WIN_CRB(_f)		(0x06110210 + _F(_f))
#define  NXE_WIN_CRB_0			(0<<25)
#define  NXE_WIN_CRB_1			(1<<25)

/*
 * The memory map inside the register windows are divided into a set of blocks.
 * Each register block is owned by one hardware agent. The following table
 * gives the memory map of the various register blocks in window 0. These
 * registers are all in the CRB register space, so the offsets given here are
 * relative to the base of the CRB offset region (NXE_MAP_CRB).
 */

#define NXE_W0_PCIE		0x00100000 /* PCI Express */
#define NXE_W0_NIU		0x00600000 /* Network Interface Unit */
#define NXE_W0_PPE_0		0x01100000 /* Protocol Processing Engine 0 */
#define NXE_W0_PPE_1		0x01200000 /* Protocol Processing Engine 1 */
#define NXE_W0_PPE_2		0x01300000 /* Protocol Processing Engine 2 */
#define NXE_W0_PPE_3		0x01400000 /* Protocol Processing Engine 3 */
#define NXE_W0_PPE_D		0x01500000 /* PPE D-cache */
#define NXE_W0_PPE_I		0x01600000 /* PPE I-cache */

/*
 * These are the register blocks inside window 1.
 */

#define NXE_W1_PCIE		0x00100000
#define NXE_W1_SW		0x00200000
#define NXE_W1_SIR		0x01200000
#define NXE_W1_ROMUSB		0x01300000

/*
 * Global registers
 */
#define NXE_BOOTLD_START	0x00010000


/*
 * driver ref section 5
 *
 * CRB Window Register Descriptions
 */

/*
 * PCI Express Registers
 *
 * Despite being in the CRB window space, they can be accessed via both
 * windows. This means they are accessable "globally" without going relative
 * to the start of the CRB window space.
 */

/* lock registers (semaphores between chipset and driver) */
#define NXE_SEM_ROM_LOCK	0x0611c010 /* ROM access lock */
#define NXE_SEM_ROM_UNLOCK	0x0611c014
#define NXE_SEM_PHY_LOCK	0x0611c018 /* PHY access lock */
#define NXE_SEM_PHY_UNLOCK	0x0611c01c
#define  NXE_SEM_DONE			0x1

/*
 * Network Interface Unit (NIU) Registers
 */
 
#define NXE_0_NIU_MODE		0x00600000
#define  NXE_0_NIU_MODE_XGE		(1<<2) /* XGE interface enabled */
#define  NXE_0_NIU_MODE_GBE		(1<<1) /* 4 GbE interfaces enabled */

/*
 * Software Defined Registers
 */

/* chipset state registers */
#define NXE_1_SW_ROM_LOCK_ID	0x00202100
#define  NXE_1_SW_ROM_LOCK_ID_DRV	0x0d417340
#define NXE_1_SW_PHY_LOCK_ID	0x00202120
#define  NXE_1_SW_PHY_LOCK_ID_DRV	0x44524956

/*
 * ROMUSB Registers
 */
#define NXE_1_ROMUSB_STATUS	0x01300004 /* ROM Status */
#define  NXE_1_ROMUSB_STATUS_DONE	(1<<1)
#define NXE_1_ROMUSB_SW_RESET	0x01300008
#define NXE_1_ROMUSB_SW_RESET_DEF	0xffffffff

#define NXE_1_GLB_CHIPCLKCTL	0x013000a8
#define NXE_1_GLB_CHIPCLKCTL_ON		0x00003fff

/* ROM Registers */
#define NXE_1_ROM_CONTROL	0x01310000
#define NXE_1_ROM_OPCODE	0x01310004
#define  NXE_1_ROM_OPCODE_READ		0x0000000b
#define NXE_1_ROM_ADDR		0x01310008
#define NXE_1_ROM_WDATA		0x0131000c
#define NXE_1_ROM_ABYTE_CNT	0x01310010
#define NXE_1_ROM_DBYTE_CNT	0x01310014 /* dummy byte count */
#define NXE_1_ROM_RDATA		0x01310018
#define NXE_1_ROM_AGT_TAG	0x0131001c
#define NXE_1_ROM_TIME_PARM	0x01310020
#define NXE_1_ROM_CLK_DIV	0x01310024
#define NXE_1_ROM_MISS_INSTR	0x01310028

/*
 * flash memory layout
 *
 * These are offsets of memory accessable via the ROM Registers above
 */
#define NXE_FLASH_CRBINIT	0x00000000 /* crb init section */
#define NXE_FLASH_BRDCFG	0x00004000 /* board config */
#define NXE_FLASH_INITCODE	0x00006000 /* pegtune code */
#define NXE_FLASH_BOOTLD	0x00010000 /* boot loader */
#define NXE_FLASH_IMAGE		0x00043000 /* compressed image */
#define NXE_FLASH_SECONDARY	0x00200000 /* backup image */
#define NXE_FLASH_PXE		0x003d0000 /* pxe image */
#define NXE_FLASH_USER		0x003e8000 /* user region for new boards */
#define NXE_FLASH_VPD		0x003e8c00 /* vendor private data */
#define NXE_FLASH_LICENSE	0x003e9000 /* firmware license */
#define NXE_FLASH_FIXED		0x003f0000 /* backup of crbinit */


/*
 * misc hardware details
 */
#define NXE_MAX_PORTS		4
#define NXE_MAX_PORT_LLADDRS	32
#define NXE_MAX_PKTLEN		(64 * 1024)


/*
 * hardware structures
 */

struct nxe_info {
	u_int32_t		ni_hdrver;
#define NXE_INFO_HDRVER_1		0x00000001

	u_int32_t		ni_board_mfg;
	u_int32_t		ni_board_type;
#define NXE_BRDTYPE_P1_BD		0x0000
#define NXE_BRDTYPE_P1_SB		0x0001
#define NXE_BRDTYPE_P1_SMAX		0x0002
#define NXE_BRDTYPE_P1_SOCK		0x0003
#define NXE_BRDTYPE_P2_SOCK_31		0x0008
#define NXE_BRDTYPE_P2_SOCK_35		0x0009
#define NXE_BRDTYPE_P2_SB35_4G		0x000a
#define NXE_BRDTYPE_P2_SB31_10G		0x000b
#define NXE_BRDTYPE_P2_SB31_2G		0x000c
#define NXE_BRDTYPE_P2_SB31_10G_IMEZ	0x000d
#define NXE_BRDTYPE_P2_SB31_10G_HMEZ	0x000e
#define NXE_BRDTYPE_P2_SB31_10G_CX4	0x000f
	u_int32_t		ni_board_num;

	u_int32_t		ni_chip_id;
	u_int32_t		ni_chip_minor;
	u_int32_t		ni_chip_major;
	u_int32_t		ni_chip_pkg;
	u_int32_t		ni_chip_lot;

	u_int32_t		ni_port_mask;
	u_int32_t		ni_peg_mask;
	u_int32_t		ni_icache;
	u_int32_t		ni_dcache;
	u_int32_t		ni_casper;

	u_int32_t		ni_lladdr0_low;
	u_int32_t		ni_lladdr1_low;
	u_int32_t		ni_lladdr2_low;
	u_int32_t		ni_lladdr3_low;

	u_int32_t		ni_mnsync_mode;
	u_int32_t		ni_mnsync_shift_cclk;
	u_int32_t		ni_mnsync_shift_mclk;
	u_int32_t		ni_mnwb_enable;
	u_int32_t		ni_mnfreq_crystal;
	u_int32_t		ni_mnfreq_speed;
	u_int32_t		ni_mnorg;
	u_int32_t		ni_mndepth;
	u_int32_t		ni_mnranks0;
	u_int32_t		ni_mnranks1;
	u_int32_t		ni_mnrd_latency0;
	u_int32_t		ni_mnrd_latency1;
	u_int32_t		ni_mnrd_latency2;
	u_int32_t		ni_mnrd_latency3;
	u_int32_t		ni_mnrd_latency4;
	u_int32_t		ni_mnrd_latency5;
	u_int32_t		ni_mnrd_latency6;
	u_int32_t		ni_mnrd_latency7;
	u_int32_t		ni_mnrd_latency8;
	u_int32_t		ni_mndll[18];
	u_int32_t		ni_mnddr_mode;
	u_int32_t		ni_mnddr_extmode;
	u_int32_t		ni_mntiming0;
	u_int32_t		ni_mntiming1;
	u_int32_t		ni_mntiming2;

	u_int32_t		ni_snsync_mode;
	u_int32_t		ni_snpt_mode;
	u_int32_t		ni_snecc_enable;
	u_int32_t		ni_snwb_enable;
	u_int32_t		ni_snfreq_crystal;
	u_int32_t		ni_snfreq_speed;
	u_int32_t		ni_snorg;
	u_int32_t		ni_sndepth;
	u_int32_t		ni_sndll;
	u_int32_t		ni_snrd_latency;

	u_int32_t		ni_lladdr0_high;
	u_int32_t		ni_lladdr1_high;
	u_int32_t		ni_lladdr2_high;
	u_int32_t		ni_lladdr3_high;

	u_int32_t		ni_magic;
#define NXE_INFO_MAGIC			0x12345678

	u_int32_t		ni_mnrd_imm;
	u_int32_t		ni_mndll_override;
} __packed;

struct nxe_imageinfo {
	u_int32_t		nim_bootld_ver;
	u_int32_t		nim_bootld_size;

	u_int8_t		nim_img_ver_major;
	u_int8_t		nim_img_ver_minor;
	u_int16_t		nim_img_ver_build;

	u_int32_t		min_img_size;
} __packed;

struct nxe_lladdr {
	u_int8_t		pad[2];
	u_int8_t		lladdr[6];
} __packed;

struct nxe_userinfo {
	u_int8_t		nu_flash_md5[1024];

	struct nxe_imageinfo	nu_imageinfo;

	u_int32_t		nu_primary;
	u_int32_t		nu_secondary;

	u_int64_t		nu_lladdr[NXE_MAX_PORTS][NXE_MAX_PORT_LLADDRS];

	u_int32_t		nu_subsys_id;

	u_int8_t		nu_serial[32];

	u_int32_t		nu_bios_ver;
} __packed;

/*
 * driver definitions
 */

struct nxe_board {
	u_int32_t		brd_type;
	u_int			brd_mode;
};

/*
 * autoconf glue
 */

struct nxe_softc {
	struct device		sc_dev;

	bus_dma_tag_t		sc_dmat;

	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;
	bus_size_t		sc_mems;
	bus_space_handle_t	sc_crbh;
	bus_space_tag_t		sc_dbt;
	bus_space_handle_t	sc_dbh;
	bus_size_t		sc_dbs;

	void			*sc_ih;

	int			sc_function;
	int			sc_window;

	const struct nxe_board	*sc_board;

	struct arpcom		sc_ac;
	struct ifmedia		sc_media;
};

int			nxe_match(struct device *, void *, void *);
void			nxe_attach(struct device *, struct device *, void *);
int			nxe_intr(void *);

struct cfattach nxe_ca = {
	sizeof(struct nxe_softc),
	nxe_match,
	nxe_attach
};

struct cfdriver nxe_cd = {
	NULL,
	"nxe",
	DV_IFNET
};

/* init code */
int			nxe_pci_map(struct nxe_softc *,
			    struct pci_attach_args *);
void			nxe_pci_unmap(struct nxe_softc *);

int			nxe_board_info(struct nxe_softc *sc);

/* low level hardware access goo */
u_int32_t		nxe_read(struct nxe_softc *, bus_size_t);
void			nxe_write(struct nxe_softc *, bus_size_t, u_int32_t);
int			nxe_wait(struct nxe_softc *, bus_size_t, u_int32_t,
			    u_int32_t, u_int);

int			nxe_crb_set(struct nxe_softc *, int);
u_int32_t		nxe_crb_read(struct nxe_softc *, bus_size_t);
void			nxe_crb_write(struct nxe_softc *, bus_size_t,
			    u_int32_t);
int			nxe_crb_wait(struct nxe_softc *, bus_size_t,
			    u_int32_t, u_int32_t, u_int);

int			nxe_rom_lock(struct nxe_softc *);
void			nxe_rom_unlock(struct nxe_softc *);
int			nxe_rom_read(struct nxe_softc *, u_int32_t,
			    u_int32_t *);
int			nxe_rom_read_region(struct nxe_softc *, u_int32_t,
			    void *, size_t);


/* misc bits */
#define DEVNAME(_sc)	((_sc)->sc_dev.dv_xname)
#define sizeofa(_a)	(sizeof(_a) / sizeof((_a)[0]))

/* let's go! */

const struct pci_matchid nxe_devices[] = {
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_10GXxR },
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_10GCX4 },
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_4GCU },
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_IMEZ },
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_HMEZ },
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_IMEZ_2 },
	{ PCI_VENDOR_NETXEN, PCI_PRODUCT_NETXEN_NXB_HMEZ_2 }
};

const struct nxe_board nxe_boards[] = {
	{ NXE_BRDTYPE_P2_SB35_4G,	NXE_0_NIU_MODE_GBE },
	{ NXE_BRDTYPE_P2_SB31_10G,	NXE_0_NIU_MODE_XGE },
	{ NXE_BRDTYPE_P2_SB31_2G,	NXE_0_NIU_MODE_GBE },
	{ NXE_BRDTYPE_P2_SB31_10G_IMEZ,	NXE_0_NIU_MODE_XGE },
	{ NXE_BRDTYPE_P2_SB31_10G_HMEZ,	NXE_0_NIU_MODE_XGE },
	{ NXE_BRDTYPE_P2_SB31_10G_CX4,	NXE_0_NIU_MODE_XGE }
};

int
nxe_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args		*pa = aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_NETWORK)
		return (0);

	return (pci_matchbyid(pa, nxe_devices, sizeofa(nxe_devices)));
}

void
nxe_attach(struct device *parent, struct device *self, void *aux)
{
	struct nxe_softc		*sc = (struct nxe_softc *)self;
	struct pci_attach_args		*pa = aux;

	sc->sc_dmat = pa->pa_dmat;
	sc->sc_function = pa->pa_function;
	sc->sc_window = -1;

	if (nxe_pci_map(sc, pa) != 0) {
		/* error already printed by nxe_pci_map() */
		return;
	}

	nxe_crb_set(sc, 1);

	if (nxe_board_info(sc) != 0) {
		/* error already printed by nxe_board_info() */
		goto unmap;
	}

	printf("\n");
	return;
unmap:
	nxe_pci_unmap(sc);
}

int
nxe_pci_map(struct nxe_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t			memtype;

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, NXE_PCI_BAR_MEM);
	if (pci_mapreg_map(pa, NXE_PCI_BAR_MEM, memtype, 0, &sc->sc_memt,
	    &sc->sc_memh, NULL, &sc->sc_mems, 0) != 0) {
		printf(": unable to map host registers\n");
		return (1);
	}
	if (sc->sc_mems != NXE_PCI_BAR_MEM_128MB) {
		printf(": unexpected register map size\n");
		goto unmap_mem;
	}

	/* set up the CRB window */
	if (bus_space_subregion(sc->sc_memt, sc->sc_memh, NXE_MAP_CRB,
	    sc->sc_mems - NXE_MAP_CRB, &sc->sc_crbh) != 0) {
		printf(": unable to create CRB window\n");
		goto unmap_mem;
	}

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, NXE_PCI_BAR_DOORBELL);
	if (pci_mapreg_map(pa, NXE_PCI_BAR_DOORBELL, memtype, 0, &sc->sc_dbt,
	    &sc->sc_dbh, NULL, &sc->sc_dbs, 0) != 0) {
		printf(": unable to map doorbell registers\n");
		/* bus_space(9) says i dont have to unmap subregions */
		goto unmap_mem;
	}

	return (0);

unmap_mem:
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
	sc->sc_mems = 0;
	return (1);
}

void
nxe_pci_unmap(struct nxe_softc *sc)
{
	bus_space_unmap(sc->sc_dbt, sc->sc_dbh, sc->sc_dbs);
	sc->sc_dbs = 0;
	/* bus_space(9) says i dont have to unmap the crb subregion */
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
	sc->sc_mems = 0;
}

int
nxe_intr(void *xsc)
{
	return (0);
}

int
nxe_board_info(struct nxe_softc *sc)
{
	struct nxe_info			*ni;
	int				rv = 1;
	int				i;

	ni = malloc(sizeof(struct nxe_info), M_NOWAIT, M_TEMP);
	if (ni == NULL) {
		printf(": unable to allocate temporary memory\n");
		return (1);
	}

	if (nxe_rom_read_region(sc, NXE_FLASH_BRDCFG, ni,
	    sizeof(struct nxe_info)) != 0) {
		printf(": unable to read board info\n");
		goto out;
	}
                 
	if (ni->ni_hdrver != NXE_INFO_HDRVER_1) {
		printf(": unexpected board info header version 0x%08x\n",
		    ni->ni_hdrver);
		goto out;
	}
	if (ni->ni_magic != NXE_INFO_MAGIC) {
		printf(": board info magic is invalid\n");
		goto out;
	}

	for (i = 0; i < sizeofa(nxe_boards); i++) {
		if (ni->ni_board_type == nxe_boards[i].brd_type) {
			sc->sc_board = &nxe_boards[i];
			break;
		}
	}
	if (sc->sc_board == NULL) {
		printf(": unknown board type %04x\n", ni->ni_board_type);
		goto out;
	}

	rv = 0;
out:
	free(ni, M_TEMP);
	return (rv);
}

u_int32_t
nxe_read(struct nxe_softc *sc, bus_size_t r)
{
	bus_space_barrier(sc->sc_memt, sc->sc_memh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_memt, sc->sc_memh, r));
}

void
nxe_write(struct nxe_softc *sc, bus_size_t r, u_int32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_memh, r, v);
	bus_space_barrier(sc->sc_memt, sc->sc_memh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

int
nxe_wait(struct nxe_softc *sc, bus_size_t r, u_int32_t m, u_int32_t v,
    u_int timeout)
{
	while ((nxe_read(sc, r) & m) != v) {
		if (timeout == 0)
			return (0);

		delay(1000);
		timeout--;
	}

	return (1);
}


int
nxe_crb_set(struct nxe_softc *sc, int window)
{
	int			oldwindow = sc->sc_window;

	if (sc->sc_window != window) {
		sc->sc_window = window;

		nxe_write(sc, NXE_WIN_CRB(sc->sc_function),
		    window ? NXE_WIN_CRB_1 : NXE_WIN_CRB_0);
	}

	return (oldwindow);
}

u_int32_t
nxe_crb_read(struct nxe_softc *sc, bus_size_t r)
{
	bus_space_barrier(sc->sc_memt, sc->sc_crbh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_memt, sc->sc_crbh, r));
}

void
nxe_crb_write(struct nxe_softc *sc, bus_size_t r, u_int32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_crbh, r, v);
	bus_space_barrier(sc->sc_memt, sc->sc_crbh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

int
nxe_crb_wait(struct nxe_softc *sc, bus_size_t r, u_int32_t m, u_int32_t v,
    u_int timeout)
{
	while ((nxe_crb_read(sc, r) & m) != v) {
		if (timeout == 0)
			return (0);

		delay(1000);
		timeout--;
	}

	return (1);
}

int
nxe_rom_lock(struct nxe_softc *sc)
{
	if (!nxe_wait(sc, NXE_SEM_ROM_LOCK, 0xffffffff,
	    NXE_SEM_DONE, 10000))
		return (1);
	nxe_crb_write(sc, NXE_1_SW_ROM_LOCK_ID, NXE_1_SW_ROM_LOCK_ID);

	return (0);
}

void
nxe_rom_unlock(struct nxe_softc *sc)
{
	nxe_read(sc, NXE_SEM_ROM_UNLOCK);
}

int
nxe_rom_read(struct nxe_softc *sc, u_int32_t r, u_int32_t *v)
{
	int			rv = 1;

	DASSERT(sc->sc_window == 1);

	if (nxe_rom_lock(sc) != 0)
		return (1);

	/* set the rom address */
	nxe_crb_write(sc, NXE_1_ROM_ADDR, r);

	/* set the xfer len */
	nxe_crb_write(sc, NXE_1_ROM_ABYTE_CNT, 3);
	delay(100); /* used to prevent bursting on the chipset */
	nxe_crb_write(sc, NXE_1_ROM_DBYTE_CNT, 0);

	/* set opcode and wait for completion */
	nxe_crb_write(sc, NXE_1_ROM_OPCODE, NXE_1_ROM_OPCODE_READ);
	if (!nxe_crb_wait(sc, NXE_1_ROMUSB_STATUS, NXE_1_ROMUSB_STATUS_DONE,
	    NXE_1_ROMUSB_STATUS_DONE, 100))
		goto err;

	/* reset counters */
	nxe_crb_write(sc, NXE_1_ROM_ABYTE_CNT, 0);
	delay(100);
	nxe_crb_write(sc, NXE_1_ROM_DBYTE_CNT, 0);

	*v = nxe_crb_read(sc, NXE_1_ROM_RDATA);

	rv = 0;
err:
	nxe_rom_unlock(sc);
	return (rv);
}

int
nxe_rom_read_region(struct nxe_softc *sc, u_int32_t r, void *buf,
    size_t buflen)
{
	u_int32_t		*databuf = buf;
	int			i;

#ifdef NXE_DEBUG
	if ((buflen % 4) != 0)
		panic("nxe_read_rom_region: buflen is wrong (%d)", buflen);
#endif

	buflen = buflen / 4;
	for (i = 0; i < buflen; i++) {
		if (nxe_rom_read(sc, r, &databuf[i]) != 0)
			return (1);

		r += sizeof(u_int32_t);
	}

	return (0);
}
