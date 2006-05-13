/*	$OpenBSD: src/sys/dev/pci/if_xge.c,v 1.4 2006/05/13 05:02:37 brad Exp $	*/
/*	$NetBSD: if_xge.c,v 1.1 2005/09/09 10:30:27 ragge Exp $	*/

/*
 * Copyright (c) 2004, SUNET, Swedish University Computer Network.
 * All rights reserved.
 *
 * Written by Anders Magnusson for SUNET, Swedish University Computer Network.
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
 *      This product includes software developed for the NetBSD Project by
 *      SUNET, Swedish University Computer Network.
 * 4. The name of SUNET may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SUNET ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SUNET
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device driver for the Neterion Xframe Ten Gigabit Ethernet controller.
 *
 * TODO (in no specific order):
 *	HW VLAN support.
 *	IPv6 HW cksum.
 */

#include <sys/cdefs.h>
#if 0
__KERNEL_RCSID(0, "$NetBSD: if_xge.c,v 1.1 2005/09/09 10:30:27 ragge Exp $");
#endif

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sys/lock.h>
#include <sys/proc.h>

#include <dev/pci/if_xgereg.h>

/*
 * Some tunable constants, tune with care!
 */
#define RX_MODE		RX_MODE_1  /* Receive mode (buffer usage, see below) */
#define NRXDESCS	1016	   /* # of receive descriptors (requested) */
#define NTXDESCS	8192	   /* Number of transmit descriptors */
#define NTXFRAGS	100	   /* Max fragments per packet */

/*
 * Receive buffer modes; 1, 3 or 5 buffers.
 */
#define RX_MODE_1 1
#define RX_MODE_3 3
#define RX_MODE_5 5

/*
 * Use clever macros to avoid a bunch of #ifdef's.
 */
#define XCONCAT3(x,y,z) x ## y ## z
#define CONCAT3(x,y,z) XCONCAT3(x,y,z)
#define NDESC_BUFMODE CONCAT3(NDESC_,RX_MODE,BUFMODE)
#define rxd_4k CONCAT3(rxd,RX_MODE,_4k)
/* XXX */
#if 0
#define rxdesc ___CONCAT(rxd,RX_MODE)
#endif
#define rxdesc rxd1

#define NEXTTX(x)	(((x)+1) % NTXDESCS)
#define NRXFRAGS	RX_MODE /* hardware imposed frags */
#define NRXPAGES	((NRXDESCS/NDESC_BUFMODE)+1)
#define NRXREAL		(NRXPAGES*NDESC_BUFMODE)
#define RXMAPSZ		(NRXPAGES*PAGE_SIZE)

/*
 * Magics to fix a bug when the mac address can't be read correctly.
 * Comes from the Linux driver.
 */
static uint64_t fix_mac[] = {
	0x0060000000000000ULL, 0x0060600000000000ULL,
	0x0040600000000000ULL, 0x0000600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0060600000000000ULL,
	0x0020600000000000ULL, 0x0000600000000000ULL,
	0x0040600000000000ULL, 0x0060600000000000ULL,
};


struct xge_softc {
	struct device sc_dev;
	struct arpcom sc_arpcom;
#define sc_if sc_arpcom.ac_if
	bus_dma_tag_t sc_dmat;
	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;
	bus_space_tag_t sc_txt;
	bus_space_handle_t sc_txh;
	void *sc_ih;
	int xge_type;			/* chip type */

	struct ifmedia xena_media;
	pcireg_t sc_pciregs[16];

	/* Transmit structures */
	struct txd *sc_txd[NTXDESCS];	/* transmit frags array */
	bus_addr_t sc_txdp[NTXDESCS];	/* bus address of transmit frags */
	bus_dmamap_t sc_txm[NTXDESCS];	/* transmit frags map */
	struct mbuf *sc_txb[NTXDESCS];	/* transmit mbuf pointer */
	int sc_nexttx, sc_lasttx;
	bus_dmamap_t sc_txmap;		/* transmit descriptor map */

	/* Receive data */
	bus_dmamap_t sc_rxmap;		/* receive descriptor map */
	struct rxd_4k *sc_rxd_4k[NRXPAGES]; /* receive desc pages */
	bus_dmamap_t sc_rxm[NRXREAL];	/* receive buffer map */
	struct mbuf *sc_rxb[NRXREAL];	/* mbufs on receive descriptors */
	int sc_nextrx;			/* next descriptor to check */
};

int xge_match(struct device *, void *, void *);
void xge_attach(struct device *, struct device *, void *);
int xge_alloc_txmem(struct xge_softc *);
int xge_alloc_rxmem(struct xge_softc *);
void xge_start(struct ifnet *);
void xge_stop(struct ifnet *, int);
int xge_add_rxbuf(struct xge_softc *, int);
void xge_mcast_filter(struct xge_softc *);
int xge_setup_xgxs(struct xge_softc *);
int xge_ioctl(struct ifnet *, u_long, caddr_t);
int xge_init(struct ifnet *);
void xge_ifmedia_status(struct ifnet *, struct ifmediareq *);
int xge_xgmii_mediachange(struct ifnet *);
void xge_enable(struct xge_softc *);
int xge_intr(void  *);

/*
 * Helpers to address registers.
 */
#define PIF_WCSR(csr, val)	pif_wcsr(sc, csr, val)
#define PIF_RCSR(csr)		pif_rcsr(sc, csr)
#define TXP_WCSR(csr, val)	txp_wcsr(sc, csr, val)
#define PIF_WKEY(csr, val)	pif_wkey(sc, csr, val)

static inline void
pif_wcsr(struct xge_softc *sc, bus_size_t csr, uint64_t val)
{
	uint32_t lval, hval;

	lval = val&0xffffffff;
	hval = val>>32;
	bus_space_write_4(sc->sc_st, sc->sc_sh, csr, lval); 
	bus_space_write_4(sc->sc_st, sc->sc_sh, csr+4, hval);
}

static inline uint64_t
pif_rcsr(struct xge_softc *sc, bus_size_t csr)
{
	uint64_t val, val2;
	val = bus_space_read_4(sc->sc_st, sc->sc_sh, csr);
	val2 = bus_space_read_4(sc->sc_st, sc->sc_sh, csr+4);
	val |= (val2 << 32);
	return val;
}

static inline void
txp_wcsr(struct xge_softc *sc, bus_size_t csr, uint64_t val)
{
	uint32_t lval, hval;

	lval = val&0xffffffff;
	hval = val>>32;
	bus_space_write_4(sc->sc_txt, sc->sc_txh, csr, lval); 
	bus_space_write_4(sc->sc_txt, sc->sc_txh, csr+4, hval);
}


static inline void
pif_wkey(struct xge_softc *sc, bus_size_t csr, uint64_t val)
{
	uint32_t lval, hval;

	lval = val&0xffffffff;
	hval = val>>32;
	PIF_WCSR(RMAC_CFG_KEY, RMAC_KEY_VALUE);
	bus_space_write_4(sc->sc_st, sc->sc_sh, csr, lval); 
	PIF_WCSR(RMAC_CFG_KEY, RMAC_KEY_VALUE);
	bus_space_write_4(sc->sc_st, sc->sc_sh, csr+4, hval);
}

struct cfattach xge_ca = {
	sizeof(struct xge_softc), xge_match, xge_attach
};

struct cfdriver xge_cd = {
	0, "xge", DV_IFNET
};

#define XNAME sc->sc_dev.dv_xname

#define XGE_RXSYNC(desc, what) \
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxmap, \
	(desc/NDESC_BUFMODE) * XGE_PAGE + sizeof(struct rxdesc) * \
	(desc%NDESC_BUFMODE), sizeof(struct rxdesc), what)
#define XGE_RXD(desc)	&sc->sc_rxd_4k[desc/NDESC_BUFMODE]-> \
	r4_rxd[desc%NDESC_BUFMODE]

/*
 * Non-tunable constants.
 */
#define XGE_MAX_MTU		9600

#define XGE_TYPE_XENA		1	/* Xframe-I */
#define XGE_TYPE_HERC		2	/* Xframe-II */

#define XGE_PCISIZE_XENA	26
#define XGE_PCISIZE_HERC	64

const struct pci_matchid xge_devices[] = {
	{ PCI_VENDOR_NETERION, PCI_PRODUCT_NETERION_XFRAME },
	{ PCI_VENDOR_NETERION, PCI_PRODUCT_NETERION_XFRAMEII }
};

int
xge_match(struct device *parent, void *match, void *aux)
{
	return (pci_matchbyid((struct pci_attach_args *)aux, xge_devices,
	    sizeof(xge_devices)/sizeof(xge_devices[0])));
}

void
xge_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct xge_softc *sc;
	struct ifnet *ifp;
	pcireg_t memtype;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	pci_chipset_tag_t pc = pa->pa_pc;
	uint8_t enaddr[ETHER_ADDR_LEN];
	uint64_t val;
	int i, pcisize;

	sc = (struct xge_softc *)self;

	sc->sc_dmat = pa->pa_dmat;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NETERION_XFRAME) {
		sc->xge_type = XGE_TYPE_XENA;
		pcisize = XGE_PCISIZE_XENA;
	} else {
		sc->xge_type = XGE_TYPE_HERC;
		pcisize = XGE_PCISIZE_HERC;
	}

	/* Get BAR0 address */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, XGE_PIF_BAR);
	if (pci_mapreg_map(pa, XGE_PIF_BAR, memtype, 0,
	    &sc->sc_st, &sc->sc_sh, 0, 0, 0)) {
		printf(": unable to map PIF BAR registers\n");
		return;
	}

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, XGE_TXP_BAR);
	if (pci_mapreg_map(pa, XGE_TXP_BAR, memtype, 0,
	    &sc->sc_txt, &sc->sc_txh, 0, 0, 0)) {
		printf(": unable to map TXP BAR registers\n");
		return;
	}

	/* Save PCI config space */
	for (i = 0; i < pcisize; i += 4)
		sc->sc_pciregs[i/4] = pci_conf_read(pa->pa_pc, pa->pa_tag, i);

#if BYTE_ORDER == LITTLE_ENDIAN
	val = (uint64_t)0xFFFFFFFFFFFFFFFFULL;
	val &= ~(TxF_R_SE|RxF_W_SE);
	PIF_WCSR(SWAPPER_CTRL, val);
	PIF_WCSR(SWAPPER_CTRL, val);
#elif BYTE_ORDER == BIG_ENDIAN
	/* do nothing */
#else
#error bad endianness!
#endif

	if ((val = PIF_RCSR(PIF_RD_SWAPPER_Fb)) != SWAPPER_MAGIC) {
		printf(": failed configuring endian, %llx != %llx!\n",
		    (unsigned long long)val, SWAPPER_MAGIC);
		return;
	}

	/*
	 * The MAC addr may be all FF's, which is not good.
	 * Resolve it by writing some magics to GPIO_CONTROL and 
	 * force a chip reset to read in the serial eeprom again.
	 */
	for (i = 0; i < sizeof(fix_mac)/sizeof(fix_mac[0]); i++) {
		PIF_WCSR(GPIO_CONTROL, fix_mac[i]);
		PIF_RCSR(GPIO_CONTROL);
	}

	/*
	 * Reset the chip and restore the PCI registers.
	 */
	PIF_WCSR(SW_RESET, XGXS_RESET(0xA5));
	DELAY(500000);
	for (i = 0; i < pcisize; i += 4)
		pci_conf_write(pa->pa_pc, pa->pa_tag, i, sc->sc_pciregs[i/4]);

	/*
	 * Restore the byte order registers.
	 */
#if BYTE_ORDER == LITTLE_ENDIAN
	val = (uint64_t)0xFFFFFFFFFFFFFFFFULL;
	val &= ~(TxF_R_SE|RxF_W_SE);
	PIF_WCSR(SWAPPER_CTRL, val);
	PIF_WCSR(SWAPPER_CTRL, val);
#elif BYTE_ORDER == BIG_ENDIAN
	/* do nothing */
#else
#error bad endianness!
#endif

	if ((val = PIF_RCSR(PIF_RD_SWAPPER_Fb)) != SWAPPER_MAGIC) {
		printf(": failed configuring endian2, %llx != %llx!\n",
		    (unsigned long long)val, SWAPPER_MAGIC);
		return;
	}

	/*
	 * XGXS initialization.
	 */
	/* 29, reset */
	PIF_WCSR(SW_RESET, 0);
	DELAY(500000);

	/* 30, configure XGXS transceiver */
	xge_setup_xgxs(sc);

	/* 33, program MAC address (not needed here) */
	/* Get ethernet address */
	PIF_WCSR(RMAC_ADDR_CMD_MEM,
	    RMAC_ADDR_CMD_MEM_STR|RMAC_ADDR_CMD_MEM_OFF(0));
	while (PIF_RCSR(RMAC_ADDR_CMD_MEM) & RMAC_ADDR_CMD_MEM_STR)
		;
	val = PIF_RCSR(RMAC_ADDR_DATA0_MEM);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		enaddr[i] = (uint8_t)(val >> (56 - (8*i)));

	/*
	 * Get memory for transmit descriptor lists.
	 */
	if (xge_alloc_txmem(sc)) {
		printf(": failed allocating txmem.\n");
		return;
	}

	/* 9 and 10 - set FIFO number/prio */
	PIF_WCSR(TX_FIFO_P0, TX_FIFO_LEN0(NTXDESCS));
	PIF_WCSR(TX_FIFO_P1, 0ULL);
	PIF_WCSR(TX_FIFO_P2, 0ULL);
	PIF_WCSR(TX_FIFO_P3, 0ULL);

	/* 11, XXX set round-robin prio? */

	/* 12, enable transmit FIFO */
	val = PIF_RCSR(TX_FIFO_P0);
	val |= TX_FIFO_ENABLE;
	PIF_WCSR(TX_FIFO_P0, val);

	/* 13, disable some error checks */
	PIF_WCSR(TX_PA_CFG,
	    TX_PA_CFG_IFR|TX_PA_CFG_ISO|TX_PA_CFG_ILC|TX_PA_CFG_ILE);

	/* Create transmit DMA maps */
	for (i = 0; i < NTXDESCS; i++) {
		if (bus_dmamap_create(sc->sc_dmat, XGE_MAX_MTU,
		    NTXFRAGS, MCLBYTES, 0, 0, &sc->sc_txm[i])) {
			printf(": cannot create TX DMA maps\n");
			return;
		}
	}

	sc->sc_lasttx = NTXDESCS-1;

	/*
	 * RxDMA initialization.
	 * Only use one out of 8 possible receive queues.
	 */
	/* allocate rx descriptor memory */
	if (xge_alloc_rxmem(sc)) {
		printf(": failed allocating rxmem\n");
		return;
	}

	/* Create receive buffer DMA maps */
	for (i = 0; i < NRXREAL; i++) {
		if (bus_dmamap_create(sc->sc_dmat, XGE_MAX_MTU,
		    NRXFRAGS, MCLBYTES, 0, 0, &sc->sc_rxm[i])) {
			printf(": cannot create RX DMA maps\n");
			return;
		}
	}

	/* allocate mbufs to receive descriptors */
	for (i = 0; i < NRXREAL; i++)
		if (xge_add_rxbuf(sc, i))
			panic("out of mbufs too early");

	/* 14, setup receive ring priority */
	PIF_WCSR(RX_QUEUE_PRIORITY, 0ULL); /* only use one ring */

	/* 15, setup receive ring round-robin calendar */
	PIF_WCSR(RX_W_ROUND_ROBIN_0, 0ULL); /* only use one ring */
	PIF_WCSR(RX_W_ROUND_ROBIN_1, 0ULL);
	PIF_WCSR(RX_W_ROUND_ROBIN_2, 0ULL);
	PIF_WCSR(RX_W_ROUND_ROBIN_3, 0ULL);
	PIF_WCSR(RX_W_ROUND_ROBIN_4, 0ULL);

	/* 16, write receive ring start address */
	PIF_WCSR(PRC_RXD0_0, (uint64_t)sc->sc_rxmap->dm_segs[0].ds_addr);
	/* PRC_RXD0_[1-7] are not used */

	/* 17, Setup alarm registers */
	PIF_WCSR(PRC_ALARM_ACTION, 0ULL); /* Default everything to retry */

	/* 18, init receive ring controller */
#if RX_MODE == RX_MODE_1
	val = RING_MODE_1;
#elif RX_MODE == RX_MODE_3
	val = RING_MODE_3;
#else /* RX_MODE == RX_MODE_5 */
	val = RING_MODE_5;
#endif
	PIF_WCSR(PRC_CTRL_0, RC_IN_SVC|val);
	/* leave 1-7 disabled */
	/* XXXX snoop configuration? */

	/* 19, set chip memory assigned to the queue */
	if (sc->xge_type == XGE_TYPE_XENA)
		PIF_WCSR(RX_QUEUE_CFG, MC_QUEUE(0, 64)); /* all 64M to queue 0 */
	else
		PIF_WCSR(RX_QUEUE_CFG, MC_QUEUE(0, 32)); /* all 32M to queue 0 */

	/* 20, setup RLDRAM parameters */
	/* do not touch it for now */

	/* 21, setup pause frame thresholds */
	/* so not touch the defaults */
	/* XXX - must 0xff be written as stated in the manual? */

	/* 22, configure RED */
	/* we do not want to drop packets, so ignore */

	/* 23, initiate RLDRAM */
	val = PIF_RCSR(MC_RLDRAM_MRS);
	val |= MC_QUEUE_SIZE_ENABLE|MC_RLDRAM_MRS_ENABLE;
	PIF_WCSR(MC_RLDRAM_MRS, val);
	DELAY(1000);

	/*
	 * Setup interrupt policies.
	 */
	/* 40, Transmit interrupts */
	PIF_WCSR(TTI_DATA1_MEM, TX_TIMER_VAL(0x1ff) | TX_TIMER_AC |
	    TX_URNG_A(5) | TX_URNG_B(20) | TX_URNG_C(48));
	PIF_WCSR(TTI_DATA2_MEM,
	    TX_UFC_A(25) | TX_UFC_B(64) | TX_UFC_C(128) | TX_UFC_D(512));
	PIF_WCSR(TTI_COMMAND_MEM, TTI_CMD_MEM_WE | TTI_CMD_MEM_STROBE);
	while (PIF_RCSR(TTI_COMMAND_MEM) & TTI_CMD_MEM_STROBE)
		;

	/* 41, Receive interrupts */
	PIF_WCSR(RTI_DATA1_MEM, RX_TIMER_VAL(0x800) | RX_TIMER_AC |
	    RX_URNG_A(5) | RX_URNG_B(20) | RX_URNG_C(50));
	PIF_WCSR(RTI_DATA2_MEM,
	    RX_UFC_A(64) | RX_UFC_B(128) | RX_UFC_C(256) | RX_UFC_D(512));
	PIF_WCSR(RTI_COMMAND_MEM, RTI_CMD_MEM_WE | RTI_CMD_MEM_STROBE);
	while (PIF_RCSR(RTI_COMMAND_MEM) & RTI_CMD_MEM_STROBE)
		;

	/*
	 * Setup media stuff.
	 */
	ifmedia_init(&sc->xena_media, IFM_IMASK, xge_xgmii_mediachange,
	    xge_ifmedia_status);
	ifmedia_add(&sc->xena_media, IFM_ETHER|IFM_1000_SX, 0, NULL);
	ifmedia_set(&sc->xena_media, IFM_ETHER|IFM_1000_SX);

	ifp = &sc->sc_arpcom.ac_if;
	strlcpy(ifp->if_xname, XNAME, IFNAMSIZ);
	strlcpy(sc->sc_arpcom.ac_enaddr, enaddr, ETHER_ADDR_LEN);
	ifp->if_baudrate = 1000000000;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = xge_ioctl;
	ifp->if_start = xge_start;
	IFQ_SET_MAXLEN(&ifp->if_snd, NTXDESCS - 1);
	IFQ_SET_READY(&ifp->if_snd);

	ifp->if_capabilities = IFCAP_VLAN_MTU;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp);

	/*
	 * Setup interrupt vector before initializing.
	 */
	if (pci_intr_map(pa, &ih)) {
		printf(": unable to map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	if ((sc->sc_ih =
	    pci_intr_establish(pc, ih, IPL_NET, xge_intr, sc, XNAME)) == NULL) {
		printf(": unable to establish interrupt at %s\n",
		    intrstr ? intrstr : "<unknown>");
		return;
	    }
	printf(": %s, address %s\n", intrstr, ether_sprintf(enaddr));
}

void
xge_ifmedia_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct xge_softc *sc = ifp->if_softc;
	uint64_t reg;

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER|IFM_1000_SX;

	reg = PIF_RCSR(ADAPTER_STATUS);
	if ((reg & (RMAC_REMOTE_FAULT|RMAC_LOCAL_FAULT)) == 0)	
		ifmr->ifm_status |= IFM_ACTIVE;
}

int
xge_xgmii_mediachange(struct ifnet *ifp)
{
	return 0;
}

void
xge_enable(struct xge_softc *sc)
{
	uint64_t val;

	/* 2, enable adapter */
	val = PIF_RCSR(ADAPTER_CONTROL);
	val |= ADAPTER_EN;
	PIF_WCSR(ADAPTER_CONTROL, val);

	/* 3, light the card enable led */
	val = PIF_RCSR(ADAPTER_CONTROL);
	val |= LED_ON;
	PIF_WCSR(ADAPTER_CONTROL, val);
	printf("%s: link up\n", XNAME);

}

int 
xge_init(struct ifnet *ifp)
{
	struct xge_softc *sc = ifp->if_softc;
	uint64_t val;

	/* 31+32, setup MAC config */
	PIF_WKEY(MAC_CFG, TMAC_EN|RMAC_EN|TMAC_APPEND_PAD|RMAC_STRIP_FCS|
	    RMAC_BCAST_EN|RMAC_DISCARD_PFRM|RMAC_PROM_EN);

	DELAY(1000);

	/* 54, ensure that the adapter is 'quiescent' */
	val = PIF_RCSR(ADAPTER_STATUS);
	if ((val & QUIESCENT) != QUIESCENT) {
#if 0
		char buf[200];
#endif
		printf("%s: adapter not quiescent, aborting\n", XNAME);
		val = (val & QUIESCENT) ^ QUIESCENT;
#if 0
		bitmask_snprintf(val, QUIESCENT_BMSK, buf, sizeof buf);
		printf("%s: ADAPTER_STATUS missing bits %s\n", XNAME, buf);
#endif
		return 1;
	}

	/* 56, enable the transmit laser */
	val = PIF_RCSR(ADAPTER_CONTROL);
	val |= EOI_TX_ON;
	PIF_WCSR(ADAPTER_CONTROL, val);

	xge_enable(sc);
	/*
	 * Enable all interrupts
	 */
	PIF_WCSR(TX_TRAFFIC_MASK, 0);
	PIF_WCSR(RX_TRAFFIC_MASK, 0);
	PIF_WCSR(GENERAL_INT_MASK, 0);
	PIF_WCSR(TXPIC_INT_MASK, 0);
	PIF_WCSR(RXPIC_INT_MASK, 0);
	PIF_WCSR(MAC_INT_MASK, MAC_TMAC_INT); /* only from RMAC */
	PIF_WCSR(MAC_RMAC_ERR_MASK, ~RMAC_LINK_STATE_CHANGE_INT);

	/* Done... */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;
}

void
xge_stop(struct ifnet *ifp, int disable)
{
	struct xge_softc *sc = ifp->if_softc;
	uint64_t val;

	val = PIF_RCSR(ADAPTER_CONTROL);
	val &= ~ADAPTER_EN;
	PIF_WCSR(ADAPTER_CONTROL, val);

	while ((PIF_RCSR(ADAPTER_STATUS) & QUIESCENT) != QUIESCENT)
		;
}

int
xge_intr(void *pv)
{
	struct xge_softc *sc = pv;
	struct txd *txd;
	struct ifnet *ifp = &sc->sc_if;
	bus_dmamap_t dmp;
	uint64_t val;
	int i, lasttx, plen;

	val = PIF_RCSR(GENERAL_INT_STATUS);
	if (val == 0)
		return 0; /* no interrupt here */

	PIF_WCSR(GENERAL_INT_STATUS, val);

	if ((val = PIF_RCSR(MAC_RMAC_ERR_REG)) & RMAC_LINK_STATE_CHANGE_INT) {
		/* Wait for quiescence */
		printf("%s: link down\n", XNAME);
		while ((PIF_RCSR(ADAPTER_STATUS) & QUIESCENT) != QUIESCENT)
			;
		PIF_WCSR(MAC_RMAC_ERR_REG, RMAC_LINK_STATE_CHANGE_INT);
			
		val = PIF_RCSR(ADAPTER_STATUS);
		if ((val & (RMAC_REMOTE_FAULT|RMAC_LOCAL_FAULT)) == 0)
			xge_enable(sc); /* Only if link restored */
	}

	if ((val = PIF_RCSR(TX_TRAFFIC_INT)))
		PIF_WCSR(TX_TRAFFIC_INT, val); /* clear interrupt bits */
	/*
	 * Collect sent packets.
	 */
	lasttx = sc->sc_lasttx;
	while ((i = NEXTTX(sc->sc_lasttx)) != sc->sc_nexttx) {
		txd = sc->sc_txd[i];
		dmp = sc->sc_txm[i];

		bus_dmamap_sync(sc->sc_dmat, dmp, 0,
		    dmp->dm_mapsize,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		if (txd->txd_control1 & TXD_CTL1_OWN) {
			bus_dmamap_sync(sc->sc_dmat, dmp, 0,
			    dmp->dm_mapsize, BUS_DMASYNC_PREREAD);
			break;
		}
		bus_dmamap_unload(sc->sc_dmat, dmp);
		m_freem(sc->sc_txb[i]);
		ifp->if_opackets++;
		sc->sc_lasttx = i;
	}

	if (sc->sc_lasttx != lasttx)
		ifp->if_flags &= ~IFF_OACTIVE;

	/* Try to get more packets on the wire */
	xge_start(ifp);

	/* clear interrupt bits */
	if ((val = PIF_RCSR(RX_TRAFFIC_INT)))
		PIF_WCSR(RX_TRAFFIC_INT, val);

	for (;;) {
		struct rxdesc *rxd;
		struct mbuf *m;

		XGE_RXSYNC(sc->sc_nextrx,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxd = XGE_RXD(sc->sc_nextrx);
		if (rxd->rxd_control1 & RXD_CTL1_OWN) {
			XGE_RXSYNC(sc->sc_nextrx, BUS_DMASYNC_PREREAD);
			break;
		}

		/* got a packet */
		m = sc->sc_rxb[sc->sc_nextrx];
#if RX_MODE == RX_MODE_1
		plen = m->m_len = RXD_CTL2_BUF0SIZ(rxd->rxd_control2);
#elif RX_MODE == RX_MODE_3
#error Fix rxmodes in xge_intr
#elif RX_MODE == RX_MODE_5
		plen = m->m_len = RXD_CTL2_BUF0SIZ(rxd->rxd_control2);
		plen += m->m_next->m_len = RXD_CTL2_BUF1SIZ(rxd->rxd_control2);
		plen += m->m_next->m_next->m_len =
		    RXD_CTL2_BUF2SIZ(rxd->rxd_control2);
		plen += m->m_next->m_next->m_next->m_len =
		    RXD_CTL3_BUF3SIZ(rxd->rxd_control3);
		plen += m->m_next->m_next->m_next->m_next->m_len =
		    RXD_CTL3_BUF4SIZ(rxd->rxd_control3);
#endif
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = plen;

		val = rxd->rxd_control1;

		if (xge_add_rxbuf(sc, sc->sc_nextrx)) {
			/* Failed, recycle this mbuf */
#if RX_MODE == RX_MODE_1
			rxd->rxd_control2 = RXD_MKCTL2(MCLBYTES, 0, 0);
			rxd->rxd_control1 = RXD_CTL1_OWN;
#elif RX_MODE == RX_MODE_3
#elif RX_MODE == RX_MODE_5
#endif
			XGE_RXSYNC(sc->sc_nextrx,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			ifp->if_ierrors++;
			break;
		}

		ifp->if_ipackets++;

#if XGE_CKSUM
		if (RXD_CTL1_PROTOS(val) & (RXD_CTL1_P_IPv4|RXD_CTL1_P_IPv6)) {
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			if (RXD_CTL1_L3CSUM(val) != 0xffff)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
		}
		if (RXD_CTL1_PROTOS(val) & RXD_CTL1_P_TCP) {
			m->m_pkthdr.csum_flags |= M_CSUM_TCPv4|M_CSUM_TCPv6;
			if (RXD_CTL1_L4CSUM(val) != 0xffff)
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}
		if (RXD_CTL1_PROTOS(val) & RXD_CTL1_P_UDP) {
			m->m_pkthdr.csum_flags |= M_CSUM_UDPv4|M_CSUM_UDPv6;
			if (RXD_CTL1_L4CSUM(val) != 0xffff)
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}
#endif

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif /* NBPFILTER > 0 */

		ether_input_mbuf(ifp, m);

		if (++sc->sc_nextrx == NRXREAL)
			sc->sc_nextrx = 0;

	}

	return 0;
}

int 
xge_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct xge_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	struct ifaddr  *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	if ((error = ether_ioctl(ifp, &sc->sc_arpcom, cmd, data)) > 0) {
		splx(s);
		return (error);
	}

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if (!(ifp->if_flags & IFF_RUNNING))
			xge_init(ifp);
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&sc->sc_arpcom, ifa);
#endif /* INET */
		break;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > XGE_MAX_MTU) {
			error = EINVAL;
		} else if (ifp->if_mtu != ifr->ifr_mtu) {
			PIF_WCSR(RMAC_MAX_PYLD_LEN,
			    RMAC_PYLD_LEN(ifr->ifr_mtu));
			ifp->if_mtu = ifr->ifr_mtu;
		}
		break;
	case SIOCSIFFLAGS:
		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */
		if (ifp->if_flags & IFF_UP) {
			xge_init(ifp);
                } else {
			if (ifp->if_flags & IFF_RUNNING)
				xge_stop(ifp, 0);
		}
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI)
			? ether_addmulti(ifr, &sc->sc_arpcom)
			: ether_delmulti(ifr, &sc->sc_arpcom);

                if (error == ENETRESET) {
                        if (ifp->if_flags & IFF_RUNNING)
				xge_mcast_filter(sc);
			error = 0;
		}
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->xena_media, cmd);
		break;
	default:
		error = EINVAL;
	}

	splx(s);
	return(error);
}

void
xge_mcast_filter(struct xge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct arpcom *ac = &sc->sc_arpcom;
	struct ether_multi *enm;
	struct ether_multistep step;
	int i, numaddr = 1; /* first slot used for card unicast address */
	uint64_t val;

	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/* Skip ranges */
			goto allmulti;
		}
		if (numaddr == MAX_MCAST_ADDR)
			goto allmulti;
		for (val = 0, i = 0; i < ETHER_ADDR_LEN; i++) {
			val <<= 8;
			val |= enm->enm_addrlo[i];
		}
		PIF_WCSR(RMAC_ADDR_DATA0_MEM, val << 16);
		PIF_WCSR(RMAC_ADDR_DATA1_MEM, 0xFFFFFFFFFFFFFFFFULL);
		PIF_WCSR(RMAC_ADDR_CMD_MEM, RMAC_ADDR_CMD_MEM_WE|
		    RMAC_ADDR_CMD_MEM_STR|RMAC_ADDR_CMD_MEM_OFF(numaddr));
		while (PIF_RCSR(RMAC_ADDR_CMD_MEM) & RMAC_ADDR_CMD_MEM_STR)
			;
		numaddr++;
		ETHER_NEXT_MULTI(step, enm);
	}
	/* set the remaining entries to the broadcast address */
	for (i = numaddr; i < MAX_MCAST_ADDR; i++) {
		PIF_WCSR(RMAC_ADDR_DATA0_MEM, 0xffffffffffff0000ULL);
		PIF_WCSR(RMAC_ADDR_DATA1_MEM, 0xFFFFFFFFFFFFFFFFULL);
		PIF_WCSR(RMAC_ADDR_CMD_MEM, RMAC_ADDR_CMD_MEM_WE|
		    RMAC_ADDR_CMD_MEM_STR|RMAC_ADDR_CMD_MEM_OFF(i));
		while (PIF_RCSR(RMAC_ADDR_CMD_MEM) & RMAC_ADDR_CMD_MEM_STR)
			;
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	return;

allmulti:
	/* Just receive everything with the multicast bit set */
	ifp->if_flags |= IFF_ALLMULTI;
	PIF_WCSR(RMAC_ADDR_DATA0_MEM, 0x8000000000000000ULL);
	PIF_WCSR(RMAC_ADDR_DATA1_MEM, 0xF000000000000000ULL);
	PIF_WCSR(RMAC_ADDR_CMD_MEM, RMAC_ADDR_CMD_MEM_WE|
	    RMAC_ADDR_CMD_MEM_STR|RMAC_ADDR_CMD_MEM_OFF(1));
	while (PIF_RCSR(RMAC_ADDR_CMD_MEM) & RMAC_ADDR_CMD_MEM_STR)
		;
}

void 
xge_start(struct ifnet *ifp)
{
	struct xge_softc *sc = ifp->if_softc;
	struct txd *txd = NULL; /* XXX - gcc */
	bus_dmamap_t dmp;
	struct	mbuf *m;
	uint64_t par, lcr;
	int nexttx = 0, ntxd, error, i;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	par = lcr = 0;
	for (;;) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;	/* out of packets */

		if (sc->sc_nexttx == sc->sc_lasttx)
			break;	/* No more space */

		nexttx = sc->sc_nexttx;
		dmp = sc->sc_txm[nexttx];

		if ((error = bus_dmamap_load_mbuf(sc->sc_dmat, dmp, m,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT)) != 0) {
			printf("%s: bus_dmamap_load_mbuf error %d\n",
			    XNAME, error);
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);

		bus_dmamap_sync(sc->sc_dmat, dmp, 0, dmp->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		txd = sc->sc_txd[nexttx];
		sc->sc_txb[nexttx] = m;
		for (i = 0; i < dmp->dm_nsegs; i++) {
			if (dmp->dm_segs[i].ds_len == 0)
				continue;
			txd->txd_control1 = dmp->dm_segs[i].ds_len;
			txd->txd_control2 = 0;
			txd->txd_bufaddr = dmp->dm_segs[i].ds_addr;
			txd++;
		}
		ntxd = txd - sc->sc_txd[nexttx] - 1;
		txd = sc->sc_txd[nexttx];
		txd->txd_control1 |= TXD_CTL1_OWN|TXD_CTL1_GCF;
		txd->txd_control2 = TXD_CTL2_UTIL;

#ifdef XGE_CKSUM
		if (m->m_pkthdr.csum_flags & M_CSUM_IPv4)
			txd->txd_control2 |= TXD_CTL2_CIPv4;
		if (m->m_pkthdr.csum_flags & M_CSUM_TCPv4)
			txd->txd_control2 |= TXD_CTL2_CTCP;
		if (m->m_pkthdr.csum_flags & M_CSUM_UDPv4)
			txd->txd_control2 |= TXD_CTL2_CUDP;
#endif
		txd[ntxd].txd_control1 |= TXD_CTL1_GCL;

		bus_dmamap_sync(sc->sc_dmat, dmp, 0, dmp->dm_mapsize,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		par = sc->sc_txdp[nexttx];
		lcr = TXDL_NUMTXD(ntxd) | TXDL_LGC_FIRST | TXDL_LGC_LAST;
		TXP_WCSR(TXDL_PAR, par);
		TXP_WCSR(TXDL_LCR, lcr);

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif /* NBPFILTER > 0 */

		sc->sc_nexttx = NEXTTX(nexttx);
	}
}

/*
 * Allocate DMA memory for transmit descriptor fragments.
 * Only one map is used for all descriptors.
 */
int
xge_alloc_txmem(struct xge_softc *sc)
{
	struct txd *txp;
	bus_dma_segment_t seg;
	bus_addr_t txdp;
	caddr_t kva;
	int i, rseg, state;

#define TXMAPSZ (NTXDESCS*NTXFRAGS*sizeof(struct txd))
	state = 0;
	if (bus_dmamem_alloc(sc->sc_dmat, TXMAPSZ, PAGE_SIZE, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT))
		goto err;
	state++;
	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, TXMAPSZ, &kva,
	    BUS_DMA_NOWAIT))
		goto err;

	state++;
	if (bus_dmamap_create(sc->sc_dmat, TXMAPSZ, 1, TXMAPSZ, 0,
	    BUS_DMA_NOWAIT, &sc->sc_txmap))
		goto err;
	state++;
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_txmap,
	    kva, TXMAPSZ, NULL, BUS_DMA_NOWAIT))
		goto err;

	/* setup transmit array pointers */
	txp = (struct txd *)kva;
	txdp = seg.ds_addr;
	for (txp = (struct txd *)kva, i = 0; i < NTXDESCS; i++) {
		sc->sc_txd[i] = txp;
		sc->sc_txdp[i] = txdp;
		txp += NTXFRAGS;
		txdp += (NTXFRAGS * sizeof(struct txd));
	}

	return 0;

err:
	if (state > 2)
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_txmap);
	if (state > 1)
		bus_dmamem_unmap(sc->sc_dmat, kva, TXMAPSZ);
	if (state > 0)
		bus_dmamem_free(sc->sc_dmat, &seg, rseg);
	return ENOBUFS;
}

/*
 * Allocate DMA memory for receive descriptor,
 * only one map is used for all descriptors.
 * link receive descriptor pages together.
 */
int
xge_alloc_rxmem(struct xge_softc *sc)
{
	struct rxd_4k *rxpp;
	bus_dma_segment_t seg;
	caddr_t kva;
	int i, rseg, state;

	/* sanity check */
	if (sizeof(struct rxd_4k) != XGE_PAGE) {
		printf("bad compiler struct alignment, %d != %d\n",
		    (int)sizeof(struct rxd_4k), XGE_PAGE);
		return EINVAL;
	}

	state = 0;
	if (bus_dmamem_alloc(sc->sc_dmat, RXMAPSZ, PAGE_SIZE, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT))
		goto err;
	state++;
	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, RXMAPSZ, &kva,
	    BUS_DMA_NOWAIT))
		goto err;

	state++;
	if (bus_dmamap_create(sc->sc_dmat, RXMAPSZ, 1, RXMAPSZ, 0,
	    BUS_DMA_NOWAIT, &sc->sc_rxmap))
		goto err;
	state++;
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_rxmap,
	    kva, RXMAPSZ, NULL, BUS_DMA_NOWAIT))
		goto err;

	/* setup receive page link pointers */
	for (rxpp = (struct rxd_4k *)kva, i = 0; i < NRXPAGES; i++, rxpp++) {
		sc->sc_rxd_4k[i] = rxpp;
		rxpp->r4_next = (uint64_t)sc->sc_rxmap->dm_segs[0].ds_addr +
		    (i*sizeof(struct rxd_4k)) + sizeof(struct rxd_4k);
	}
	sc->sc_rxd_4k[NRXPAGES-1]->r4_next = 
	    (uint64_t)sc->sc_rxmap->dm_segs[0].ds_addr;

	return 0;

err:
	if (state > 2)
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_txmap);
	if (state > 1)
		bus_dmamem_unmap(sc->sc_dmat, kva, TXMAPSZ);
	if (state > 0)
		bus_dmamem_free(sc->sc_dmat, &seg, rseg);
	return ENOBUFS;
}


/*
 * Add a new mbuf chain to descriptor id.
 */
int
xge_add_rxbuf(struct xge_softc *sc, int id)
{
	struct rxdesc *rxd;
	struct mbuf *m[5];
	int page, desc, error;
#if RX_MODE == RX_MODE_5
	int i;
#endif

	page = id/NDESC_BUFMODE;
	desc = id%NDESC_BUFMODE;

	rxd = &sc->sc_rxd_4k[page]->r4_rxd[desc];

	/*
	 * Allocate mbufs.
	 * Currently five mbufs and two clusters are used,
	 * the hardware will put (ethernet, ip, tcp/udp) headers in
	 * their own buffer and the clusters are only used for data.
	 */
#if RX_MODE == RX_MODE_1
	MGETHDR(m[0], M_DONTWAIT, MT_DATA);
	if (m[0] == NULL)
		return ENOBUFS;
	MCLGET(m[0], M_DONTWAIT);
	if ((m[0]->m_flags & M_EXT) == 0) {
		m_freem(m[0]);
		return ENOBUFS;
	}
	m[0]->m_len = m[0]->m_pkthdr.len = m[0]->m_ext.ext_size;
#elif RX_MODE == RX_MODE_3
#error missing rxmode 3.
#elif RX_MODE == RX_MODE_5
	MGETHDR(m[0], M_DONTWAIT, MT_DATA);
	for (i = 1; i < 5; i++) {
		MGET(m[i], M_DONTWAIT, MT_DATA);
	}
	if (m[3])
		MCLGET(m[3], M_DONTWAIT);
	if (m[4])
		MCLGET(m[4], M_DONTWAIT);
	if (!m[0] || !m[1] || !m[2] || !m[3] || !m[4] || 
	    ((m[3]->m_flags & M_EXT) == 0) || ((m[4]->m_flags & M_EXT) == 0)) {
		/* Out of something */
		for (i = 0; i < 5; i++)
			if (m[i] != NULL)
				m_free(m[i]);
		return ENOBUFS;
	}
	/* Link'em together */
	m[0]->m_next = m[1];
	m[1]->m_next = m[2];
	m[2]->m_next = m[3];
	m[3]->m_next = m[4];
#else
#error bad mode RX_MODE
#endif

	if (sc->sc_rxb[id])
		bus_dmamap_unload(sc->sc_dmat, sc->sc_rxm[id]);
	sc->sc_rxb[id] = m[0];

	error = bus_dmamap_load_mbuf(sc->sc_dmat, sc->sc_rxm[id], m[0],
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error)
		return error;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxm[id], 0,
	    sc->sc_rxm[id]->dm_mapsize, BUS_DMASYNC_PREREAD);

#if RX_MODE == RX_MODE_1
	rxd->rxd_control2 = RXD_MKCTL2(m[0]->m_len, 0, 0);
	rxd->rxd_buf0 = (uint64_t)sc->sc_rxm[id]->dm_segs[0].ds_addr;
	rxd->rxd_control1 = RXD_CTL1_OWN;
#elif RX_MODE == RX_MODE_3
#elif RX_MODE == RX_MODE_5
	rxd->rxd_control3 = RXD_MKCTL3(0, m[3]->m_len, m[4]->m_len);
	rxd->rxd_control2 = RXD_MKCTL2(m[0]->m_len, m[1]->m_len, m[2]->m_len);
	rxd->rxd_buf0 = (uint64_t)sc->sc_rxm[id]->dm_segs[0].ds_addr;
	rxd->rxd_buf1 = (uint64_t)sc->sc_rxm[id]->dm_segs[1].ds_addr;
	rxd->rxd_buf2 = (uint64_t)sc->sc_rxm[id]->dm_segs[2].ds_addr;
	rxd->rxd_buf3 = (uint64_t)sc->sc_rxm[id]->dm_segs[3].ds_addr;
	rxd->rxd_buf4 = (uint64_t)sc->sc_rxm[id]->dm_segs[4].ds_addr;
	rxd->rxd_control1 = RXD_CTL1_OWN;
#endif

	XGE_RXSYNC(id, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	return 0;
}

/*
 * These magics comes from the FreeBSD driver.
 */
int
xge_setup_xgxs(struct xge_softc *sc)
{
	/* The magic numbers are described in the users guide */

	/* Writing to MDIO 0x8000 (Global Config 0) */
	PIF_WCSR(DTX_CONTROL, 0x8000051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80000515000000E0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80000515D93500E4ULL); DELAY(50);

	/* Writing to MDIO 0x8000 (Global Config 1) */
	PIF_WCSR(DTX_CONTROL, 0x8001051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80010515000000e0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80010515001e00e4ULL); DELAY(50);

	/* Reset the Gigablaze */
	PIF_WCSR(DTX_CONTROL, 0x8002051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80020515000000E0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80020515F21000E4ULL); DELAY(50);

	/* read the pole settings */
	PIF_WCSR(DTX_CONTROL, 0x8000051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80000515000000e0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80000515000000ecULL); DELAY(50);

	PIF_WCSR(DTX_CONTROL, 0x8001051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80010515000000e0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80010515000000ecULL); DELAY(50);

	PIF_WCSR(DTX_CONTROL, 0x8002051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80020515000000e0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x80020515000000ecULL); DELAY(50);

	/* Workaround for TX Lane XAUI initialization error.
	   Read Xpak PHY register 24 for XAUI lane status */
	PIF_WCSR(DTX_CONTROL, 0x0018040000000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x00180400000000e0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x00180400000000ecULL); DELAY(50);

	/* 
	 * Reading the MDIO control with value 0x1804001c0F001c
	 * means the TxLanes were already in sync
	 * Reading the MDIO control with value 0x1804000c0x001c
	 * means some TxLanes are not in sync where x is a 4-bit
	 * value representing each lanes
	 */
#if 0
	val = PIF_RCSR(MDIO_CONTROL);
	if (val != 0x1804001c0F001cULL) {
		printf("%s: MDIO_CONTROL: %llx != %llx\n", 
		    XNAME, val, 0x1804001c0F001cULL);
		return 1;
	}
#endif

	/* Set and remove the DTE XS INTLoopBackN */
	PIF_WCSR(DTX_CONTROL, 0x0000051500000000ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x00000515604000e0ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x00000515604000e4ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x00000515204000e4ULL); DELAY(50);
	PIF_WCSR(DTX_CONTROL, 0x00000515204000ecULL); DELAY(50);

#if 0
	/* Reading the DTX control register Should be 0x5152040001c */
	val = PIF_RCSR(DTX_CONTROL);
	if (val != 0x5152040001cULL) {
		printf("%s: DTX_CONTROL: %llx != %llx\n", 
		    XNAME, val, 0x5152040001cULL);
		return 1;
	}
#endif

	PIF_WCSR(MDIO_CONTROL, 0x0018040000000000ULL); DELAY(50);
	PIF_WCSR(MDIO_CONTROL, 0x00180400000000e0ULL); DELAY(50);
	PIF_WCSR(MDIO_CONTROL, 0x00180400000000ecULL); DELAY(50);

#if 0
	/* Reading the MIOD control should be 0x1804001c0f001c */
	val = PIF_RCSR(MDIO_CONTROL);
	if (val != 0x1804001c0f001cULL) {
		printf("%s: MDIO_CONTROL2: %llx != %llx\n",
		    XNAME, val, 0x1804001c0f001cULL);
		return 1;
	}
#endif
	return 0;
}
