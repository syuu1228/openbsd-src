/*	$OpenBSD: src/sys/dev/pci/if_txp.c,v 1.5 2001/04/08 21:47:45 jason Exp $	*/

/*
 * Copyright (c) 2001
 *	Jason L. Wright <jason@thought.net> and
 *	Aaron Campbell <aaron@monkey.org>.  All rights reserved.
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
 *	This product includes software developed by Jason L. Wright and
 *	Aaron Campbell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for 3c990 (Typhoon) Ethernet ASIC
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/timeout.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#include <net/if_media.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <vm/vm.h>              /* for vtophys */
#include <vm/pmap.h>            /* for vtophys */
#include <vm/vm_kern.h>
#include <vm/vm_extern.h>
#include <machine/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_txpreg.h>
#include <dev/microcode/typhoon/typhoon_image.h>

int txp_probe		__P((struct device *, void *, void *));
void txp_attach		__P((struct device *, struct device *, void *));
int txp_intr		__P((void *));
void txp_tick		__P((void *));
void txp_shutdown	__P((void *));
int txp_ioctl		__P((struct ifnet *, u_long, caddr_t));
void txp_start		__P((struct ifnet *));
void txp_stop		__P((struct txp_softc *));
void txp_init		__P((struct txp_softc *));
void txp_watchdog	__P((struct ifnet *));

int txp_chip_init __P((struct txp_softc *));
int txp_reset_adapter __P((struct txp_softc *));
int txp_download_fw __P((struct txp_softc *));
int txp_download_fw_wait __P((struct txp_softc *));
int txp_download_fw_section __P((struct txp_softc *,
    struct txp_fw_section_header *, int));
int txp_alloc_rings __P((struct txp_softc *));
void txp_dma_free __P((struct txp_softc *, struct txp_dma_alloc *));
int txp_dma_malloc __P((struct txp_softc *, bus_size_t, struct txp_dma_alloc *));

int
txp_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_3COM)
		return (0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_3COM_3CR990TX95:
	case PCI_PRODUCT_3COM_3CR990TX97:
	case PCI_PRODUCT_3COM_3CR990SVR95:
	case PCI_PRODUCT_3COM_3CR990SVR97:
		return (1);
	}

	return (0);
}

void
txp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct txp_softc *sc = (struct txp_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	bus_size_t iosize;
	u_int32_t command;

	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);

	if (!(command & PCI_COMMAND_MASTER_ENABLE)) {
		printf(": failed to enable bus mastering\n");
		return;
	}

	if (!(command & PCI_COMMAND_MEM_ENABLE)) {
		printf(": failed to enable memory mapping\n");
		return;
	}
	if (pci_mapreg_map(pa, TXP_PCI_LOMEM, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_bt, &sc->sc_bh, NULL, &iosize)) {
		printf(": can't map mem space %d\n", 0);
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	/*
	 * Allocate our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf(": couldn't map interrupt\n");
		return;
	}

	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, txp_intr, sc,
	    self->dv_xname);
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf(": %s", intrstr);

	if (txp_chip_init(sc))
		return;

	if (txp_download_fw(sc))
		return;

	if (txp_alloc_rings(sc))
		return;

	printf("\n");

	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = txp_ioctl;
	ifp->if_output = ether_output;
	ifp->if_start = txp_start;
	ifp->if_watchdog = txp_watchdog;
	ifp->if_baudrate = 10000000;
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);

	/*
	 * Attach us everywhere
	 */
	if_attach(ifp);
	ether_ifattach(ifp);

	shutdownhook_establish(txp_shutdown, sc);
}

int
txp_chip_init(sc)
	struct txp_softc *sc;
{
	/* disable interrupts */
	WRITE_REG(sc, TXP_IER, 0);
	WRITE_REG(sc, TXP_IMR,
	    TXP_INT_SELF | TXP_INT_PCI_TABORT | TXP_INT_PCI_MABORT |
	    TXP_INT_DMA3 | TXP_INT_DMA2 | TXP_INT_DMA1 | TXP_INT_DMA0 |
	    TXP_INT_LATCH);

	/* ack all interrupts */
	WRITE_REG(sc, TXP_ISR, TXP_INT_RESERVED | TXP_INT_LATCH |
	    TXP_INT_A2H_7 | TXP_INT_A2H_6 | TXP_INT_A2H_5 | TXP_INT_A2H_4 |
	    TXP_INT_SELF | TXP_INT_PCI_TABORT | TXP_INT_PCI_MABORT |
	    TXP_INT_DMA3 | TXP_INT_DMA2 | TXP_INT_DMA1 | TXP_INT_DMA0 |
	    TXP_INT_A2H_3 | TXP_INT_A2H_2 | TXP_INT_A2H_1 | TXP_INT_A2H_0);

	if (txp_reset_adapter(sc))
		return (-1);

	/* disable interrupts */
	WRITE_REG(sc, TXP_IER, 0);
	WRITE_REG(sc, TXP_IMR,
	    TXP_INT_SELF | TXP_INT_PCI_TABORT | TXP_INT_PCI_MABORT |
	    TXP_INT_DMA3 | TXP_INT_DMA2 | TXP_INT_DMA1 | TXP_INT_DMA0 |
	    TXP_INT_LATCH);

	/* ack all interrupts */
	WRITE_REG(sc, TXP_ISR, TXP_INT_RESERVED | TXP_INT_LATCH |
	    TXP_INT_A2H_7 | TXP_INT_A2H_6 | TXP_INT_A2H_5 | TXP_INT_A2H_4 |
	    TXP_INT_SELF | TXP_INT_PCI_TABORT | TXP_INT_PCI_MABORT |
	    TXP_INT_DMA3 | TXP_INT_DMA2 | TXP_INT_DMA1 | TXP_INT_DMA0 |
	    TXP_INT_A2H_3 | TXP_INT_A2H_2 | TXP_INT_A2H_1 | TXP_INT_A2H_0);

	return (0);
}

int
txp_reset_adapter(sc)
	struct txp_softc *sc;
{
	u_int32_t r;
	int i;

	WRITE_REG(sc, TXP_SRR, TXP_SRR_ALL);
	DELAY(1000);
	WRITE_REG(sc, TXP_SRR, 0);

	/* Should wait max 6 seconds */
	for (i = 0; i < 6000; i++) {
		r = READ_REG(sc, TXP_A2H_0);
		if (r == STAT_WAITING_FOR_HOST_REQUEST)
			break;
		DELAY(1000);
	}

	if (r != STAT_WAITING_FOR_HOST_REQUEST) {
		printf(": reset hung\n");
		return (-1);
	}

	return (0);
}

int
txp_download_fw(sc)
	struct txp_softc *sc;
{
	struct txp_fw_file_header *fileheader;
	struct txp_fw_section_header *secthead;
	int sect;
	u_int32_t r, i, ier, imr;

	ier = READ_REG(sc, TXP_IER);
	WRITE_REG(sc, TXP_IER, ier | TXP_INT_A2H_0);

	imr = READ_REG(sc, TXP_IMR);
	WRITE_REG(sc, TXP_IMR, imr | TXP_INT_A2H_0);

	for (i = 0; i < 10000; i++) {
		r = READ_REG(sc, TXP_A2H_0);
		if (r == STAT_WAITING_FOR_HOST_REQUEST)
			break;
		DELAY(50);
	}
	if (r != STAT_WAITING_FOR_HOST_REQUEST) {
		printf(": not waiting for host request\n");
		return (-1);
	}

	/* Ack the status */
	WRITE_REG(sc, TXP_ISR, TXP_INT_A2H_0);

	fileheader = (struct txp_fw_file_header *)TyphoonImage;
	if (strncmp("TYPHOON", fileheader->magicid, sizeof(fileheader->magicid))) {
		printf(": fw invalid magic\n");
		return (-1);
	}

	/* Tell boot firmware to get ready for image */
	WRITE_REG(sc, TXP_H2A_1, fileheader->addr);
	WRITE_REG(sc, TXP_H2A_0, TXP_BOOTCMD_RUNTIME_IMAGE);

	if (txp_download_fw_wait(sc)) {
		printf(": fw wait failed, initial\n");
		return (-1);
	}

	secthead = (struct txp_fw_section_header *)(TyphoonImage +
	    sizeof(struct txp_fw_file_header));

	for (sect = 0; sect < fileheader->nsections; sect++) {
		if (txp_download_fw_section(sc, secthead, sect))
			return (-1);
		secthead = (struct txp_fw_section_header *)
		    (((u_int8_t *)secthead) + secthead->nbytes + sizeof(*secthead));
	}

	WRITE_REG(sc, TXP_H2A_0, TXP_BOOTCMD_DOWNLOAD_COMPLETE);

	for (i = 0; i < 10000; i++) {
		r = READ_REG(sc, TXP_A2H_0);
		if (r == STAT_WAITING_FOR_BOOT)
			break;
		DELAY(50);
	}
	if (r != STAT_WAITING_FOR_BOOT) {
		printf(": not waiting for boot\n");
		return (-1);
	}

	WRITE_REG(sc, TXP_IER, ier);
	WRITE_REG(sc, TXP_IMR, imr);

	return (0);
}

int
txp_download_fw_wait(sc)
	struct txp_softc *sc;
{
	u_int32_t i, r;

	for (i = 0; i < 10000; i++) {
		r = READ_REG(sc, TXP_ISR);
		if (r & TXP_INT_A2H_0)
			break;
		DELAY(50);
	}

	if (!(r & TXP_INT_A2H_0)) {
		printf(": fw wait failed comm0\n", sc->sc_dev.dv_xname);
		return (-1);
	}

	WRITE_REG(sc, TXP_ISR, TXP_INT_A2H_0);

	r = READ_REG(sc, TXP_A2H_0);
	if (r != STAT_WAITING_FOR_SEGMENT) {
		printf(": fw not waiting for segment\n", sc->sc_dev.dv_xname);
		return (-1);
	}
	return (0);
}

int
txp_download_fw_section(sc, sect, sectnum)
	struct txp_softc *sc;
	struct txp_fw_section_header *sect;
	int sectnum;
{
	struct txp_dma_alloc dma;
	int rseg, err = 0;
	struct mbuf m;
	u_int16_t csum;

	/* Skip zero length sections */
	if (sect->nbytes == 0)
		return (0);

	/* Make sure we aren't past the end of the image */
	rseg = ((u_int8_t *)sect) - ((u_int8_t *)TyphoonImage);
	if (rseg >= sizeof(TyphoonImage)) {
		printf(": fw invalid section address, section %d\n", sectnum);
		return (-1);
	}

	/* Make sure this section doesn't go past the end */
	rseg += sect->nbytes;
	if (rseg >= sizeof(TyphoonImage)) {
		printf(": fw truncated section %d\n", sectnum);
		return (-1);
	}

	/* map a buffer, copy segment to it, get physaddr */
	if (txp_dma_malloc(sc, sect->nbytes, &dma)) {
		printf(": fw dma malloc failed, section %d\n", sectnum);
		return (-1);
	}

	bcopy(((u_int8_t *)sect) + sizeof(*sect), dma.dma_vaddr, sect->nbytes);

	/*
	 * dummy up mbuf and verify section checksum
	 */
	m.m_type = MT_DATA;
	m.m_next = m.m_nextpkt = NULL;
	m.m_len = sect->nbytes;
	m.m_data = dma.dma_vaddr;
	m.m_flags = 0;
	csum = in_cksum(&m, sect->nbytes);
	if (csum != sect->cksum) {
		printf(": fw section %d, bad cksum (expected 0x%x got 0x%x)\n",
		    sectnum, sect->cksum, csum);
		err = -1;
		goto bail;
	}

	bus_dmamap_sync(sc->sc_dmat, dma.dma_map,
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	WRITE_REG(sc, TXP_H2A_1, sect->nbytes);
	WRITE_REG(sc, TXP_H2A_2, sect->cksum);
	WRITE_REG(sc, TXP_H2A_3, sect->addr);
	WRITE_REG(sc, TXP_H2A_4, dma.dma_paddr >> 32);
	WRITE_REG(sc, TXP_H2A_5, dma.dma_paddr & 0xffffffff);
	WRITE_REG(sc, TXP_H2A_0, TXP_BOOTCMD_SEGMENT_AVAILABLE);

	if (txp_download_fw_wait(sc)) {
		printf(": fw wait failed, section %d\n", sectnum);
		err = -1;
		goto bail;
	}

	bus_dmamap_sync(sc->sc_dmat, dma.dma_map,
	    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

bail:
	txp_dma_free(sc, &dma);

	return (err);
}

int
txp_intr(vsc)
	void *vsc;
{
	int claimed = 0;

	return (claimed);
}


void
txp_shutdown(vsc)
	void *vsc;
{
	struct txp_softc *sc = (struct txp_softc *)vsc;

	txp_stop(sc);
}

int
txp_alloc_rings(sc)
	struct txp_softc *sc;
{
	struct txp_boot_record *boot;
	u_int32_t r;
	int i;

	/* boot record */
	if (txp_dma_malloc(sc, sizeof(struct txp_boot_record), &sc->sc_boot_dma)) {
		printf(": can't allocate boot record\n");
		return (-1);
	}
	boot = (struct txp_boot_record *)sc->sc_boot_dma.dma_vaddr;
	bzero(boot, sizeof(*boot));

	/* host ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_hostring), &sc->sc_host_dma)) {
		printf(": can't allocate host ring\n");
		goto bail_boot;
	}
	bzero(sc->sc_host_dma.dma_vaddr, sc->sc_host_dma.dma_siz);
	boot->br_hostring_lo = sc->sc_host_dma.dma_paddr & 0xffffffff;
	boot->br_hostring_hi = sc->sc_host_dma.dma_paddr >> 32;

	/* high priority tx ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_tx_desc) * TX_ENTRIES,
	    &sc->sc_txhiring_dma)) {
		printf(": can't allocate high tx ring\n");
		goto bail_host;
	}
	bzero(sc->sc_txhiring_dma.dma_vaddr, sc->sc_txhiring_dma.dma_siz);
	boot->br_txhipri_lo = sc->sc_txhiring_dma.dma_paddr & 0xffffffff;
	boot->br_txhipri_hi = sc->sc_txhiring_dma.dma_paddr >> 32;
	boot->br_txhipri_siz = TX_ENTRIES * sizeof(struct txp_tx_desc);

	/* low priority tx ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_tx_desc) * TX_ENTRIES,
	    &sc->sc_txloring_dma)) {
		printf(": can't allocate low tx ring\n");
		goto bail_txhiring;
	}
	bzero(sc->sc_txloring_dma.dma_vaddr, sc->sc_txloring_dma.dma_siz);
	boot->br_txlopri_lo = sc->sc_txloring_dma.dma_paddr & 0xffffffff;
	boot->br_txlopri_hi = sc->sc_txloring_dma.dma_paddr >> 32;
	boot->br_txlopri_siz = TX_ENTRIES * sizeof(struct txp_tx_desc);

	/* high priority rx ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_rx_desc) * RX_ENTRIES,
	    &sc->sc_rxhiring_dma)) {
		printf(": can't allocate high rx ring\n");
		goto bail_txloring;
	}
	bzero(sc->sc_rxhiring_dma.dma_vaddr, sc->sc_rxhiring_dma.dma_siz);
	boot->br_rxhipri_lo = sc->sc_rxhiring_dma.dma_paddr & 0xffffffff;
	boot->br_rxhipri_hi = sc->sc_rxhiring_dma.dma_paddr >> 32;
	boot->br_rxhipri_siz = RX_ENTRIES * sizeof(struct txp_rx_desc);

	/* low priority rx ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_rx_desc) * RX_ENTRIES,
	    &sc->sc_rxloring_dma)) {
		printf(": can't allocate low rx ring\n");
		goto bail_rxhiring;
	}
	bzero(sc->sc_rxloring_dma.dma_vaddr, sc->sc_rxloring_dma.dma_siz);
	boot->br_rxlopri_lo = sc->sc_rxloring_dma.dma_paddr & 0xffffffff;
	boot->br_rxlopri_hi = sc->sc_rxloring_dma.dma_paddr >> 32;
	boot->br_rxlopri_siz = RX_ENTRIES * sizeof(struct txp_rx_desc);

	/* command ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_cmd_desc) * CMD_ENTRIES,
	    &sc->sc_cmdring_dma)) {
		printf(": can't allocate command ring\n");
		goto bail_rxloring;
	}
	bzero(sc->sc_cmdring_dma.dma_vaddr, sc->sc_cmdring_dma.dma_siz);
	boot->br_cmd_lo = sc->sc_cmdring_dma.dma_paddr & 0xffffffff;
	boot->br_cmd_hi = sc->sc_cmdring_dma.dma_paddr >> 32;
	boot->br_cmd_siz = CMD_ENTRIES * sizeof(struct txp_cmd_desc);

	/* response ring */
	if (txp_dma_malloc(sc, sizeof(struct txp_resp_desc) * RESP_ENTRIES,
	    &sc->sc_respring_dma)) {
		printf(": can't allocate response ring\n");
		goto bail_cmdring;
	}
	bzero(sc->sc_respring_dma.dma_vaddr, sc->sc_respring_dma.dma_siz);
	boot->br_resp_lo = sc->sc_respring_dma.dma_paddr & 0xffffffff;
	boot->br_resp_hi = sc->sc_respring_dma.dma_paddr >> 32;
	boot->br_resp_siz = CMD_ENTRIES * sizeof(struct txp_resp_desc);

	/* zero dma */
	if (txp_dma_malloc(sc, sizeof(u_int32_t), &sc->sc_zero_dma)) {
		printf(": can't allocate response ring\n");
		goto bail_respring;
	}
	bzero(sc->sc_zero_dma.dma_vaddr, sc->sc_zero_dma.dma_siz);
	boot->br_zero_lo = sc->sc_zero_dma.dma_paddr & 0xffffffff;
	boot->br_zero_hi = sc->sc_zero_dma.dma_paddr >> 32;

	/* See if it's waiting for boot, and try to boot it */
	for (i = 0; i < 10000; i++) {
		r = READ_REG(sc, TXP_A2H_0);
		if (r == STAT_WAITING_FOR_BOOT)
			break;
		DELAY(50);
	}
	if (r != STAT_WAITING_FOR_BOOT) {
		printf(": not waiting for boot\n");
		goto bail;
	}
	WRITE_REG(sc, TXP_H2A_2, sc->sc_boot_dma.dma_paddr >> 32);
	WRITE_REG(sc, TXP_H2A_1, sc->sc_boot_dma.dma_paddr & 0xffffffff);
	WRITE_REG(sc, TXP_H2A_0, TXP_BOOTCMD_REGISTER_BOOT_RECORD);

	/* See if it booted */
	for (i = 0; i < 10000; i++) {
		r = READ_REG(sc, TXP_A2H_0);
		if (r == STAT_RUNNING)
			break;
		DELAY(50);
	}
	if (r != STAT_RUNNING) {
		printf(": fw not running\n");
		goto bail;
	}

	/* Clear TX and CMD ring write registers */
	WRITE_REG(sc, TXP_H2A_1, TXP_BOOTCMD_NULL);
	WRITE_REG(sc, TXP_H2A_2, TXP_BOOTCMD_NULL);
	WRITE_REG(sc, TXP_H2A_3, TXP_BOOTCMD_NULL);
	WRITE_REG(sc, TXP_H2A_0, TXP_BOOTCMD_NULL);

	return (0);

bail:
	txp_dma_free(sc, &sc->sc_zero_dma);
bail_respring:
	txp_dma_free(sc, &sc->sc_respring_dma);
bail_cmdring:
	txp_dma_free(sc, &sc->sc_cmdring_dma);
bail_rxloring:
	txp_dma_free(sc, &sc->sc_rxloring_dma);
bail_rxhiring:
	txp_dma_free(sc, &sc->sc_rxhiring_dma);
bail_txloring:
	txp_dma_free(sc, &sc->sc_txloring_dma);
bail_txhiring:
	txp_dma_free(sc, &sc->sc_txhiring_dma);
bail_host:
	txp_dma_free(sc, &sc->sc_host_dma);
bail_boot:
	txp_dma_free(sc, &sc->sc_boot_dma);
	return (-1);
}

int
txp_dma_malloc(sc, size, dma)
	struct txp_softc *sc;
	bus_size_t size;
	struct txp_dma_alloc *dma;
{
	int r;

	if ((r = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0,
	    &dma->dma_seg, 1, &dma->dma_nseg, BUS_DMA_NOWAIT)) != 0) {
		return (r);
	}
	if (dma->dma_nseg != 1) {
		bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nseg);
		return (-1);
	}

	if ((r = bus_dmamem_map(sc->sc_dmat, &dma->dma_seg, dma->dma_nseg,
	    size, &dma->dma_vaddr, BUS_DMA_NOWAIT)) != 0) {
		bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nseg);
		return (r);
	}

	if ((r = bus_dmamap_create(sc->sc_dmat, size, dma->dma_nseg,
	    size, 0, BUS_DMA_NOWAIT, &dma->dma_map)) != 0) {
		bus_dmamem_unmap(sc->sc_dmat, dma->dma_vaddr, size);
		bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nseg);
		return (r);
	}

	if ((r = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_vaddr,
	    size, NULL, BUS_DMA_NOWAIT)) != 0) {
		bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
		bus_dmamem_unmap(sc->sc_dmat, dma->dma_vaddr, size);
		bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nseg);
		return (r);
	}

	dma->dma_paddr = dma->dma_map->dm_segs[0].ds_addr;
	dma->dma_siz = size;

	return (0);
}

void
txp_dma_free(sc, dma)
	struct txp_softc *sc;
	struct txp_dma_alloc *dma;
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_vaddr, dma->dma_siz);
	bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nseg);
}

void
txp_tick(vsc)
	void *vsc;
{
	struct txp_softc *sc = vsc;

	timeout_add(&sc->sc_tick_tmo, hz);
}

int
txp_ioctl(ifp, command, data)
	struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct txp_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splimp();

	if ((error = ether_ioctl(ifp, &sc->sc_arpcom, command, data)) > 0) {
		splx(s);
		return error;
	}

	switch(command) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			txp_init(sc);
			arp_ifinit(&sc->sc_arpcom, ifa);
			break;
#endif /* INET */
		default:
			txp_init(sc);
			break;
		}
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			txp_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				txp_stop(sc);
		}
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_arpcom) :
		    ether_delmulti(ifr, &sc->sc_arpcom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware
			 * filter accordingly.
			 */
			/* XXX TODO: set multicast list */
			error = 0;
		}
		break;
	default:
		error = EINVAL;
		break;
	}

	(void)splx(s);

	return(error);
}

void
txp_init(sc)
	struct txp_softc *sc;
{
}

void
txp_start(ifp)
	struct ifnet *ifp;
{
	struct mbuf *m;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
		/* XXX enqueue and send the packet */
		m_freem(m);
	}
}

void
txp_stop(sc)
	struct txp_softc *sc;
{
}

void
txp_watchdog(ifp)
	struct ifnet *ifp;
{
}

struct cfattach txp_ca = {
	sizeof(struct txp_softc), txp_probe, txp_attach,
};

struct cfdriver txp_cd = {
	0, "txp", DV_IFNET
};
