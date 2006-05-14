/*	$OpenBSD: src/sys/dev/pci/if_wpivar.h,v 1.1 2006/05/14 19:00:48 damien Exp $	*/

/*-
 * Copyright (c) 2006
 *	Damien Bergamini <damien.bergamini@free.fr>
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

struct wpi_rx_radiotap_header {
	struct ieee80211_radiotap_header wr_ihdr;
	uint8_t		wr_flags;
	uint16_t	wr_chan_freq;
	uint16_t	wr_chan_flags;
	uint8_t		wr_antsignal;
} __packed;

#define WPI_RX_RADIOTAP_PRESENT						\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |				\
	 (1 << IEEE80211_RADIOTAP_DB_ANTSIGNAL))

struct wpi_tx_radiotap_header {
	struct ieee80211_radiotap_header wt_ihdr;
	uint8_t		wt_flags;
	uint16_t	wt_chan_freq;
	uint16_t	wt_chan_flags;
} __packed;

#define WPI_TX_RADIOTAP_PRESENT						\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL))

struct wpi_tx_data {
	bus_dmamap_t		map;
	struct mbuf		*m;
	struct ieee80211_node	*ni;
};

struct wpi_tx_ring {
	bus_dmamap_t		map;
	bus_dma_segment_t	seg;
	bus_dmamap_t		cmd_map;
	bus_dma_segment_t	cmd_seg;
	struct wpi_tx_desc	*desc;
	struct wpi_tx_cmd	*cmd;
	struct wpi_tx_data	*data;
	int			qid;
	int			count;
	int			queued;
	int			cur;
};

struct wpi_rx_data {
	bus_dmamap_t	map;
	struct mbuf	*m;
};

struct wpi_rx_ring {
	bus_dmamap_t		map;
	bus_dma_segment_t	seg;
	uint32_t		*desc;
	struct wpi_rx_data	data[WPI_RX_RING_COUNT];
	int			cur;
};

struct wpi_softc {
	struct device		sc_dev;

	struct ieee80211com	sc_ic;
	int			(*sc_newstate)(struct ieee80211com *,
				    enum ieee80211_state, int);

	uint32_t		flags;
#define WPI_FLAG_FW_INITED	(1 << 0)

	bus_dma_tag_t		sc_dmat;

	/* shared area */
	bus_dmamap_t		shmap;
	bus_dma_segment_t	shseg;
	struct wpi_shared	*shared;

	struct wpi_tx_ring	txq[4];
	struct wpi_tx_ring	cmdq;
	struct wpi_tx_ring	svcq;
	struct wpi_rx_ring	rxq;

	bus_space_tag_t		sc_st;
	bus_space_handle_t	sc_sh;
	void 			*sc_ih;
	pci_chipset_tag_t	sc_pct;
	pcitag_t		sc_pcitag;
	bus_size_t		sc_sz;

	struct wpi_config	config;
	uint16_t		calib1[14];
	uint16_t		calib2[14];

	int			authmode;
	int			sc_tx_timer;
	void			*powerhook;

#if NBPFILTER > 0
	caddr_t			sc_drvbpf;

	union {
		struct wpi_rx_radiotap_header th;
		uint8_t	pad[64];
	} sc_rxtapu;
#define sc_rxtap	sc_rxtapu.th
	int			sc_rxtap_len;

	union {
		struct wpi_tx_radiotap_header th;
		uint8_t	pad[64];
	} sc_txtapu;
#define sc_txtap	sc_txtapu.th
	int			sc_txtap_len;
#endif
};
