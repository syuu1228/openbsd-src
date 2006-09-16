/*	$OpenBSD: src/sys/dev/usb/if_uath.c,v 1.4 2006/09/16 19:54:13 damien Exp $	*/

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

/*-
 * Driver for Atheros AR5005UG/AR5005UX chipsets.
 * http://www.atheros.com/pt/bulletins/AR5005UGBulletin.pdf
 * http://www.atheros.com/pt/bulletins/AR5005UXBulletin.pdf
 *
 * IMPORTANT NOTICE:
 * This driver was written without any documentation or support from Atheros
 * Communications. It is based on a black-box analysis of the Windows binary
 * driver. It handles both pre and post-firmware devices.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/timeout.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/endian.h>
#include <machine/intr.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/rndvar.h>
#include <crypto/arc4.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/if_uathreg.h>
#include <dev/usb/if_uathvar.h>

#ifdef USB_DEBUG
#define UATH_DEBUG
#endif

#ifdef UATH_DEBUG
#define DPRINTF(x)	do { if (uath_debug) logprintf x; } while (0)
#define DPRINTFN(n, x)	do { if (uath_debug >= (n)) logprintf x; } while (0)
int uath_debug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

/* various supported device vendors/products */
#define UATH_DEV(v, p, f)						\
	{ { USB_VENDOR_##v, USB_PRODUCT_##v##_##p },			\
	    (f) },							\
	{ { USB_VENDOR_##v, USB_PRODUCT_##v##_##p##_NF }, 		\
	    (f) | UATH_FLAG_PRE_FIRMWARE }
static const struct uath_type {
	struct usb_devno	dev;
	unsigned int		flags;
#define UATH_FLAG_PRE_FIRMWARE	(1 << 0)
#define UATH_FLAG_DUAL_BAND_RF	(1 << 1)
} uath_devs[] = {
	/* Atheros Communications */
	UATH_DEV(ATHEROS, AR5523, 0),
	UATH_DEV(ATHEROS2, AR5523_1, 0),
	UATH_DEV(ATHEROS2, AR5523_2, 0),
	UATH_DEV(ATHEROS2, AR5523_3, UATH_FLAG_DUAL_BAND_RF),

	/* Conceptronic */
	UATH_DEV(CONCEPTRONIC, AR5523_1, 0),
	UATH_DEV(CONCEPTRONIC, AR5523_2, 0),

	/* D-Link */
	UATH_DEV(DLINK, DWLAG122, UATH_FLAG_DUAL_BAND_RF),
	UATH_DEV(DLINK, DWLAG132, UATH_FLAG_DUAL_BAND_RF),	
	UATH_DEV(DLINK, DWLG132, 0),

	/* Global Sun Technology */
	UATH_DEV(GLOBALSUN, AR5523_1, 0),
	UATH_DEV(GLOBALSUN, AR5523_2, 0),

	/* Netgear */
	UATH_DEV(NETGEAR, WG111U, UATH_FLAG_DUAL_BAND_RF),
	UATH_DEV(NETGEAR3, WG111T, 0),
	UATH_DEV(NETGEAR3, WPN111, 0),

	/* U-MEDIA Communications */
	UATH_DEV(UMEDIA, AR5523_1, 0),
	UATH_DEV(UMEDIA, AR5523_2, UATH_FLAG_DUAL_BAND_RF),

	/* Wistron NeWeb */
	UATH_DEV(WISTRONNEWEB, AR5523_1, 0),
	UATH_DEV(WISTRONNEWEB, AR5523_2, 0)
};
#define uath_lookup(v, p)	\
	((struct uath_type *)usb_lookup(uath_devs, v, p))

Static void	uath_attachhook(void *);
Static int	uath_open_pipes(struct uath_softc *);
Static void	uath_close_pipes(struct uath_softc *);
Static int	uath_alloc_tx_data_list(struct uath_softc *);
Static void	uath_free_tx_data_list(struct uath_softc *);
Static int	uath_alloc_rx_data_list(struct uath_softc *);
Static void	uath_free_rx_data_list(struct uath_softc *);
Static int	uath_alloc_tx_cmd_list(struct uath_softc *);
Static void	uath_free_tx_cmd_list(struct uath_softc *);
Static int	uath_alloc_rx_cmd_list(struct uath_softc *);
Static void	uath_free_rx_cmd_list(struct uath_softc *);
Static int	uath_media_change(struct ifnet *);
Static void	uath_stat(void *);
Static void	uath_next_scan(void *);
Static void	uath_task(void *);
Static int	uath_newstate(struct ieee80211com *, enum ieee80211_state,
		    int);
#ifdef UATH_DEBUG
Static void	uath_dump_cmd(const uint8_t *, int, char);
#endif
Static int	uath_cmd(struct uath_softc *, uint32_t, const void *, int,
		    void *, int);
Static int	uath_cmd_write(struct uath_softc *, uint32_t, const void *,
		    int, int);
Static int	uath_cmd_read(struct uath_softc *, uint32_t, const void *,
		    int, void *, int);
Static int	uath_write_reg(struct uath_softc *, uint32_t, uint32_t);
Static int	uath_write_multi(struct uath_softc *, uint32_t, const void *,
		    int);
Static int	uath_read_reg(struct uath_softc *, uint32_t, uint32_t *);
Static int	uath_read_eeprom(struct uath_softc *, uint32_t, void *);
Static void	uath_cmd_rxeof(usbd_xfer_handle, usbd_private_handle,
		    usbd_status);
Static void	uath_data_rxeof(usbd_xfer_handle, usbd_private_handle,
		    usbd_status);
Static void	uath_data_txeof(usbd_xfer_handle, usbd_private_handle,
		    usbd_status);
Static int	uath_tx_null(struct uath_softc *);
Static int	uath_tx_data(struct uath_softc *, struct mbuf *,
		    struct ieee80211_node *);
Static void	uath_start(struct ifnet *);
Static void	uath_watchdog(struct ifnet *);
Static int	uath_ioctl(struct ifnet *, u_long, caddr_t);
Static int	uath_query_eeprom(struct uath_softc *);
Static int	uath_reset(struct uath_softc *);
Static int	uath_reset_tx_queues(struct uath_softc *);
Static int	uath_wme_init(struct uath_softc *);
Static int	uath_set_chan(struct uath_softc *, struct ieee80211_channel *);
Static int	uath_set_key(struct uath_softc *,
		    const struct ieee80211_wepkey *, int);
Static int	uath_set_keys(struct uath_softc *);
Static int	uath_set_rates(struct uath_softc *,
		    const struct ieee80211_rateset *);
Static int	uath_set_rxfilter(struct uath_softc *, uint32_t, uint32_t);
Static int	uath_set_led(struct uath_softc *, int, int);
Static int	uath_switch_channel(struct uath_softc *,
		    struct ieee80211_channel *);
Static int	uath_init(struct ifnet *);
Static void	uath_stop(struct ifnet *, int);
Static int	uath_loadfirmware(struct uath_softc *, const u_char *, int);
Static int	uath_activate(device_ptr_t, enum devact);

/*
 * Supported rates for 802.11b/g modes (in 500Kbps unit).
 */
static const struct ieee80211_rateset uath_rateset_11b =
	{ 4, { 2, 4, 11, 22 } };

static const struct ieee80211_rateset uath_rateset_11g =
	{ 12, { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 } };

USB_DECLARE_DRIVER(uath);

USB_MATCH(uath)
{
	USB_MATCH_START(uath, uaa);

	if (uaa->iface != NULL)
		return UMATCH_NONE;

	return (uath_lookup(uaa->vendor, uaa->product) != NULL) ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

Static void
uath_attachhook(void *xsc)
{
	struct uath_softc *sc = xsc;
	u_char *fw;
	size_t size;
	int error;

	if ((error = loadfirmware("uath-ar5523", &fw, &size)) != 0) {
		printf("%s: could not read firmware (error=%d)\n",
		    USBDEVNAME(sc->sc_dev), error);
		return;
	}

	if ((error = uath_loadfirmware(sc, fw, size)) != 0) {
		printf("%s: could not load firmware (error=%s)\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(error));
	}

	free(fw, M_DEVBUF);
}

USB_ATTACH(uath)
{
	USB_ATTACH_START(uath, sc, uaa);
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	usbd_status error;
	char *devinfop;
	int i;

	sc->sc_udev = uaa->device;

	devinfop = usbd_devinfo_alloc(uaa->device, 0);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->sc_dev), devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_flags = uath_lookup(uaa->vendor, uaa->product)->flags;

	if (usbd_set_config_no(sc->sc_udev, UATH_CONFIG_NO, 0) != 0) {
		printf("%s: could not set configuration no\n",
		    USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	/* get the first interface handle */
	error = usbd_device2interface_handle(sc->sc_udev, UATH_IFACE_INDEX,
	    &sc->sc_iface);
	if (error != 0) {
		printf("%s: could not get interface handle\n",
		    USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	/*
	 * We must open the pipes early because they're used to upload the
	 * firmware (pre-firmware devices) or to send firmware commands.
	 */
	if (uath_open_pipes(sc) != 0) {
		printf("%s: could not open pipes\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	if (sc->sc_flags & UATH_FLAG_PRE_FIRMWARE) {
		if (rootvp == NULL)
			mountroothook_establish(uath_attachhook, sc);
		else
			uath_attachhook(sc);
		USB_ATTACH_SUCCESS_RETURN;
	}

	/*
	 * Only post-firmware devices here.
	 */
	usb_init_task(&sc->sc_task, uath_task, sc);
	timeout_set(&sc->scan_to, uath_next_scan, sc);
	timeout_set(&sc->stat_to, uath_stat, sc);

	/*
	 * Allocate xfers for firmware commands.
	 */
	if (uath_alloc_tx_cmd_list(sc) != 0) {
		printf("%s: could not allocate Tx command list\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail1;
	}
	if (uath_alloc_rx_cmd_list(sc) != 0) {
		printf("%s: could not allocate Rx command list\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail2;
	}

	/*
	 * Queue Rx command xfers.
	 */
	for (i = 0; i < UATH_RX_CMD_LIST_COUNT; i++) {
		struct uath_rx_cmd *cmd = &sc->rx_cmd[i];

		usbd_setup_xfer(cmd->xfer, sc->cmd_rx_pipe, cmd, cmd->buf,
		    UATH_MAX_RXCMDSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY,
		    USBD_NO_TIMEOUT, uath_cmd_rxeof);
		error = usbd_transfer(cmd->xfer);
		if (error != USBD_IN_PROGRESS && error != 0) {
			printf("%s: could not queue Rx command xfer\n",
			    USBDEVNAME(sc->sc_dev));
			goto fail3;
		}
	}

	/*
	 * We're now ready to send/receive firmware commands.
	 */
	if (uath_reset(sc) != 0) {
		printf("%s: could not initialize adapter\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail3;
	}
	if (uath_query_eeprom(sc) != 0) {
		printf("%s: could not read EEPROM\n", USBDEVNAME(sc->sc_dev));
		goto fail3;
	}

	printf("%s: MAC/BBP AR5523, RF AR%c112, address %s\n",
	    USBDEVNAME(sc->sc_dev),
	    (sc->sc_flags & UATH_FLAG_DUAL_BAND_RF) ? '5': '2',
	    ether_sprintf(ic->ic_myaddr));

	/*
	 * Allocate xfers for Tx/Rx data pipes.
	 */
	if (uath_alloc_tx_data_list(sc) != 0) {
		printf("%s: could not allocate Tx data list\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail3;
	}
	if (uath_alloc_rx_data_list(sc) != 0) {
		printf("%s: could not allocate Rx data list\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail4;
	}

	ic->ic_phytype = IEEE80211_T_OFDM;	/* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA;	/* default to BSS mode */
	ic->ic_state = IEEE80211_S_INIT;

	/* set device capabilities */
	ic->ic_caps =
	    IEEE80211_C_TXPMGT |	/* tx power management */
	    IEEE80211_C_SHPREAMBLE |	/* short preamble supported */
	    IEEE80211_C_SHSLOT |	/* short slot time supported */
	    IEEE80211_C_WEP;		/* h/w WEP */

	/* set supported .11b and .11g rates */
	ic->ic_sup_rates[IEEE80211_MODE_11B] = uath_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = uath_rateset_11g;

	/* set supported .11b and .11g channels (1 through 14) */
	for (i = 1; i <= 14; i++) {
		ic->ic_channels[i].ic_freq =
		    ieee80211_ieee2mhz(i, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[i].ic_flags =
		    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
		    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
	}

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = uath_init;
	ifp->if_ioctl = uath_ioctl;
	ifp->if_start = uath_start;
	ifp->if_watchdog = uath_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, USBDEVNAME(sc->sc_dev), IFNAMSIZ);

	if_attach(ifp);
	ieee80211_ifattach(ifp);

	/* override state transition machine */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = uath_newstate;
	ieee80211_media_init(ifp, uath_media_change, ieee80211_media_status);

#if NBPFILTER > 0
	bpfattach(&sc->sc_drvbpf, ifp, DLT_IEEE802_11_RADIO,
	    sizeof (struct ieee80211_frame) + IEEE80211_RADIOTAP_HDRLEN);

	sc->sc_rxtap_len = sizeof sc->sc_rxtapu;
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(UATH_RX_RADIOTAP_PRESENT);

	sc->sc_txtap_len = sizeof sc->sc_txtapu;
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(UATH_TX_RADIOTAP_PRESENT);
#endif

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
	    USBDEV(sc->sc_dev));

	USB_ATTACH_SUCCESS_RETURN;

fail4:	uath_free_tx_data_list(sc);
fail3:	uath_free_rx_cmd_list(sc);
fail2:	uath_free_tx_cmd_list(sc);
fail1:	uath_close_pipes(sc);

	USB_ATTACH_ERROR_RETURN;
}

USB_DETACH(uath)
{
	USB_DETACH_START(uath, sc);
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	s = splusb();

	if (sc->sc_flags & UATH_FLAG_PRE_FIRMWARE) {
		uath_close_pipes(sc);
		splx(s);
		return 0;
	}

	/* post-firmware device */

	usb_rem_task(sc->sc_udev, &sc->sc_task);
	timeout_del(&sc->scan_to);
	timeout_del(&sc->stat_to);

	/* abort and free xfers */
	uath_free_tx_data_list(sc);
	uath_free_rx_data_list(sc);
	uath_free_tx_cmd_list(sc);
	uath_free_rx_cmd_list(sc);

	/* close Tx/Rx pipes */
	uath_close_pipes(sc);

	ieee80211_ifdetach(ifp);	/* free all nodes */
	if_detach(ifp);

	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
	    USBDEV(sc->sc_dev));

	return 0;
}

Static int
uath_open_pipes(struct uath_softc *sc)
{
	int error;

	/*
	 * XXX pipes numbers are hardcoded because we don't have any way
	 * to distinguish the data pipes from the firmware command pipes
	 * (both are bulk pipes) using the endpoints descriptors.
	 */
	error = usbd_open_pipe(sc->sc_iface, 0x01, USBD_EXCLUSIVE_USE,
	    &sc->cmd_tx_pipe);
	if (error != 0) {
		printf("%s: could not open Tx command pipe: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(error));
		goto fail;
	}

	error = usbd_open_pipe(sc->sc_iface, 0x02, USBD_EXCLUSIVE_USE,
	    &sc->data_tx_pipe);
	if (error != 0) {
		printf("%s: could not open Tx data pipe: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(error));
		goto fail;
	}

	error = usbd_open_pipe(sc->sc_iface, 0x81, USBD_EXCLUSIVE_USE,
	    &sc->cmd_rx_pipe);
	if (error != 0) {
		printf("%s: could not open Rx command pipe: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(error));
		goto fail;
	}

	error = usbd_open_pipe(sc->sc_iface, 0x82, USBD_EXCLUSIVE_USE,
	    &sc->data_rx_pipe);
	if (error != 0) {
		printf("%s: could not open Rx data pipe: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(error));
		goto fail;
	}

	return 0;

fail:	uath_close_pipes(sc);
	return error;
}

Static void
uath_close_pipes(struct uath_softc *sc)
{
	/* assumes no transfers are pending on the pipes */

	if (sc->data_tx_pipe != NULL)
		usbd_close_pipe(sc->data_tx_pipe);

	if (sc->data_rx_pipe != NULL)
		usbd_close_pipe(sc->data_rx_pipe);

	if (sc->cmd_tx_pipe != NULL)
		usbd_close_pipe(sc->cmd_tx_pipe);

	if (sc->cmd_rx_pipe != NULL)
		usbd_close_pipe(sc->cmd_rx_pipe);
}

Static int
uath_alloc_tx_data_list(struct uath_softc *sc)
{
	int i, error;

	for (i = 0; i < UATH_TX_DATA_LIST_COUNT; i++) {
		struct uath_tx_data *data = &sc->tx_data[i];

		data->sc = sc;	/* backpointer for callbacks */

		data->xfer = usbd_alloc_xfer(sc->sc_udev);
		if (data->xfer == NULL) {
			printf("%s: could not allocate xfer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
		data->buf = usbd_alloc_buffer(data->xfer, UATH_MAX_TXBUFSZ);
		if (data->buf == NULL) {
			printf("%s: could not allocate xfer buffer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
	}
	return 0;

fail:	uath_free_tx_data_list(sc);
	return error;
}

Static void
uath_free_tx_data_list(struct uath_softc *sc)
{
	int i;

	/* make sure no transfers are pending */
	usbd_abort_pipe(sc->data_tx_pipe);

	for (i = 0; i < UATH_TX_DATA_LIST_COUNT; i++)
		if (sc->tx_data[i].xfer != NULL)
			usbd_free_xfer(sc->tx_data[i].xfer);
}

Static int
uath_alloc_rx_data_list(struct uath_softc *sc)
{
	int i, error;

	for (i = 0; i < UATH_RX_DATA_LIST_COUNT; i++) {
		struct uath_rx_data *data = &sc->rx_data[i];

		data->sc = sc;	/* backpointer for callbacks */

		data->xfer = usbd_alloc_xfer(sc->sc_udev);
		if (data->xfer == NULL) {
			printf("%s: could not allocate xfer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
		if (usbd_alloc_buffer(data->xfer, sc->rxbufsz) == NULL) {
			printf("%s: could not allocate xfer buffer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}

		MGETHDR(data->m, M_DONTWAIT, MT_DATA);
		if (data->m == NULL) {
			printf("%s: could not allocate rx mbuf\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
		MCLGET(data->m, M_DONTWAIT);
		if (!(data->m->m_flags & M_EXT)) {
			printf("%s: could not allocate rx mbuf cluster\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}

		data->buf = mtod(data->m, uint8_t *);
	}
	return 0;

fail:	uath_free_rx_data_list(sc);
	return error;
}

Static void
uath_free_rx_data_list(struct uath_softc *sc)
{
	int i;

	/* make sure no transfers are pending */
	usbd_abort_pipe(sc->data_rx_pipe);

	for (i = 0; i < UATH_RX_DATA_LIST_COUNT; i++) {
		struct uath_rx_data *data = &sc->rx_data[i];

		if (data->xfer != NULL)
			usbd_free_xfer(data->xfer);

		if (data->m != NULL)
			m_freem(data->m);
	}
}

Static int
uath_alloc_tx_cmd_list(struct uath_softc *sc)
{
	int i, error;

	for (i = 0; i < UATH_TX_CMD_LIST_COUNT; i++) {
		struct uath_tx_cmd *cmd = &sc->tx_cmd[i];

		cmd->sc = sc;	/* backpointer for callbacks */

		cmd->xfer = usbd_alloc_xfer(sc->sc_udev);
		if (cmd->xfer == NULL) {
			printf("%s: could not allocate xfer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
		cmd->buf = usbd_alloc_buffer(cmd->xfer, UATH_MAX_TXCMDSZ);
		if (cmd->buf == NULL) {
			printf("%s: could not allocate xfer buffer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
	}
	return 0;

fail:	uath_free_tx_cmd_list(sc);
	return error;
}

Static void
uath_free_tx_cmd_list(struct uath_softc *sc)
{
	int i;

	/* make sure no transfers are pending */
	usbd_abort_pipe(sc->cmd_tx_pipe);

	for (i = 0; i < UATH_TX_CMD_LIST_COUNT; i++)
		if (sc->tx_cmd[i].xfer != NULL)
			usbd_free_xfer(sc->tx_cmd[i].xfer);
}

Static int
uath_alloc_rx_cmd_list(struct uath_softc *sc)
{
	int i, error;

	for (i = 0; i < UATH_RX_CMD_LIST_COUNT; i++) {
		struct uath_rx_cmd *cmd = &sc->rx_cmd[i];

		cmd->sc = sc;	/* backpointer for callbacks */

		cmd->xfer = usbd_alloc_xfer(sc->sc_udev);
		if (cmd->xfer == NULL) {
			printf("%s: could not allocate xfer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
		cmd->buf = usbd_alloc_buffer(cmd->xfer, UATH_MAX_RXCMDSZ);
		if (cmd->buf == NULL) {
			printf("%s: could not allocate xfer buffer\n",
			    USBDEVNAME(sc->sc_dev));
			error = ENOMEM;
			goto fail;
		}
	}
	return 0;

fail:	uath_free_rx_cmd_list(sc);
	return error;
}

Static void
uath_free_rx_cmd_list(struct uath_softc *sc)
{
	int i;

	/* make sure no transfers are pending */
	usbd_abort_pipe(sc->cmd_rx_pipe);

	for (i = 0; i < UATH_RX_CMD_LIST_COUNT; i++)
		if (sc->rx_cmd[i].xfer != NULL)
			usbd_free_xfer(sc->rx_cmd[i].xfer);
}

Static int
uath_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return error;

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
		uath_init(ifp);

	return 0;
}

/*
 * This function is called periodically (every second) when associated to
 * query device statistics.
 */
Static void
uath_stat(void *arg)
{
	struct uath_softc *sc = arg;
	int error;

	/*
	 * Send request for statistics asynchronously. The timer will be
	 * restarted when we'll get the stats notification.
	 */
	error = uath_cmd_write(sc, UATH_CMD_STATS, NULL, 0,
	    UATH_CMD_FLAG_ASYNC);
	if (error != 0) {
		printf("%s: could not query statistics (error=%d)\n",
		    USBDEVNAME(sc->sc_dev), error);
	}
}

/*
 * This function is called periodically (every 250ms) during scanning to
 * switch from one channel to another.
 */
Static void
uath_next_scan(void *arg)
{
	struct uath_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;

	if (ic->ic_state == IEEE80211_S_SCAN)
		ieee80211_next_scan(ifp);
}

Static void
uath_task(void *arg)
{
	struct uath_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	enum ieee80211_state ostate;

	ostate = ic->ic_state;

	switch (sc->sc_state) {
	case IEEE80211_S_INIT:
		if (ostate == IEEE80211_S_RUN) {
			/* turn link and activity LEDs off */
			(void)uath_set_led(sc, UATH_LED_LINK, 0);
			(void)uath_set_led(sc, UATH_LED_ACTIVITY, 0);
		}
		break;

	case IEEE80211_S_SCAN:
		if (uath_switch_channel(sc, ic->ic_bss->ni_chan) != 0) {
			printf("%s: could not switch channel\n",
			    USBDEVNAME(sc->sc_dev));
			break;
		}
		timeout_add(&sc->scan_to, hz / 4);
		break;

	case IEEE80211_S_AUTH:
	{
		struct ieee80211_node *ni = ic->ic_bss;
		struct uath_cmd_bssid bssid;
		struct uath_cmd_0b cmd0b;
		struct uath_cmd_0c cmd0c;

		if (uath_switch_channel(sc, ni->ni_chan) != 0) {
			printf("%s: could not switch channel\n",
			    USBDEVNAME(sc->sc_dev));
			break;
		}

		(void)uath_cmd_write(sc, UATH_CMD_24, NULL, 0, 0);

		bzero(&bssid, sizeof bssid);
		bssid.len = htobe32(IEEE80211_ADDR_LEN);
		IEEE80211_ADDR_COPY(bssid.bssid, ni->ni_bssid);
		(void)uath_cmd_write(sc, UATH_CMD_SET_BSSID, &bssid,
		    sizeof bssid, 0);

		bzero(&cmd0b, sizeof cmd0b);
		cmd0b.code = htobe32(2);
		cmd0b.size = htobe32(sizeof (cmd0b.data));
		(void)uath_cmd_write(sc, UATH_CMD_0B, &cmd0b, sizeof cmd0b, 0);

		bzero(&cmd0c, sizeof cmd0c);
		cmd0c.magic1 = htobe32(2);
		cmd0c.magic2 = htobe32(7);
		cmd0c.magic3 = htobe32(1);
		(void)uath_cmd_write(sc, UATH_CMD_0C, &cmd0c, sizeof cmd0c, 0);

		if (uath_set_rates(sc, &ni->ni_rates) != 0) {
			printf("%s: could not set negotiated rate set\n",
			    USBDEVNAME(sc->sc_dev));
			break;
		}
		break;
	}

	case IEEE80211_S_ASSOC:
		break;

	case IEEE80211_S_RUN:
	{
		struct ieee80211_node *ni = ic->ic_bss;
		struct uath_cmd_bssid bssid;
		struct uath_cmd_xled xled;
		uint32_t val;

		if (ic->ic_opmode == IEEE80211_M_MONITOR) {
			/* make both LEDs blink while monitoring */
			bzero(&xled, sizeof xled);
			xled.which = htobe32(0);
			xled.rate = htobe32(1);
			xled.mode = htobe32(2);
			(void)uath_cmd_write(sc, UATH_CMD_SET_XLED, &xled,
			    sizeof xled, 0);
			break;
		}

		/*
		 * Tx rate is controlled by firmware, report the maximum
		 * negotiated rate in ifconfig output.
		 */
		ni->ni_txrate = ni->ni_rates.rs_nrates - 1;

		val = htobe32(1);
		(void)uath_cmd_write(sc, UATH_CMD_2E, &val, sizeof val, 0);

		bzero(&bssid, sizeof bssid);
		bssid.flags1 = htobe32(0xc004);
		bssid.flags2 = htobe32(0x003b);
		bssid.len = htobe32(IEEE80211_ADDR_LEN);
		IEEE80211_ADDR_COPY(bssid.bssid, ni->ni_bssid);
		(void)uath_cmd_write(sc, UATH_CMD_SET_BSSID, &bssid,
		    sizeof bssid, 0);

		/* turn link LED on */
		(void)uath_set_led(sc, UATH_LED_LINK, 1);

		/* make activity LED blink */
		bzero(&xled, sizeof xled);
		xled.which = htobe32(1);
		xled.rate = htobe32(1);
		xled.mode = htobe32(2);
		(void)uath_cmd_write(sc, UATH_CMD_SET_XLED, &xled, sizeof xled,
		    0);

		/* set state to associated */
		val = htobe32(1);
		(void)uath_cmd_write(sc, UATH_CMD_SET_STATE, &val, sizeof val,
		    0);

		/* start statistics timer */
		timeout_add(&sc->stat_to, hz);
		break;
	}
	}
	sc->sc_newstate(ic, sc->sc_state, sc->sc_arg);
}

Static int
uath_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct uath_softc *sc = ic->ic_softc;

	usb_rem_task(sc->sc_udev, &sc->sc_task);
	timeout_del(&sc->scan_to);
	timeout_del(&sc->stat_to);

	/* do it in a process context */
	sc->sc_state = nstate;
	sc->sc_arg = arg;
	usb_add_task(sc->sc_udev, &sc->sc_task);

	return 0;
}

#ifdef UATH_DEBUG
Static void
uath_dump_cmd(const uint8_t *buf, int len, char prefix)
{
	int i;

	for (i = 0; i < len; i++) {
		if ((i % 16) == 0)
			printf("\n%c ", prefix);
		else if ((i % 4) == 0)
			printf(" ");
		printf("%02x", buf[i]);
	}
	printf("\n");
}
#endif

/*
 * Low-level function to send read or write commands to the firmware.
 */
Static int
uath_cmd(struct uath_softc *sc, uint32_t code, const void *idata, int ilen,
    void *odata, int flags)
{
	struct uath_cmd_hdr *hdr;
	struct uath_tx_cmd *cmd;
	uint16_t xferflags;
	int s, xferlen, error;

	/* grab a xfer */
	cmd = &sc->tx_cmd[sc->cmd_idx];

	/* always bulk-out a multiple of 4 bytes */
	xferlen = (sizeof (struct uath_cmd_hdr) + ilen + 3) & ~3;

	hdr = (struct uath_cmd_hdr *)cmd->buf;
	bzero(hdr, sizeof (struct uath_cmd_hdr));
	hdr->len   = htobe32(xferlen);
	hdr->code  = htobe32(code);
	hdr->priv  = sc->cmd_idx;	/* don't care about endianness */
	hdr->magic = htobe32((flags & UATH_CMD_FLAG_MAGIC) ? 1 << 24 : 0);
	bcopy(idata, (uint8_t *)(hdr + 1), ilen);

#ifdef UATH_DEBUG
	if (uath_debug >= 5) {
		printf("sending command code=0x%02x flags=0x%x index=%u",
		    code, flags, sc->cmd_idx);
		uath_dump_cmd(cmd->buf, xferlen, '+');
	}
#endif
	xferflags = USBD_FORCE_SHORT_XFER | USBD_NO_COPY;
	if (!(flags & UATH_CMD_FLAG_READ)) {
		if (!(flags & UATH_CMD_FLAG_ASYNC))
			xferflags |= USBD_SYNCHRONOUS;
	} else
		s = splusb();

	cmd->odata = odata;

	usbd_setup_xfer(cmd->xfer, sc->cmd_tx_pipe, cmd, cmd->buf, xferlen,
	    xferflags, UATH_CMD_TIMEOUT, NULL);
	error = usbd_transfer(cmd->xfer);
	if (error != USBD_IN_PROGRESS && error != 0) {
		if (flags & UATH_CMD_FLAG_READ)
			splx(s);
		printf("%s: could not send command (error=%s)\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(error));
		return error;
	}
	sc->cmd_idx = (sc->cmd_idx + 1) % UATH_TX_CMD_LIST_COUNT;

	if (!(flags & UATH_CMD_FLAG_READ))
		return 0;	/* write: don't wait for reply */

	/* wait at most two seconds for command reply */
	error = tsleep(cmd, PCATCH, "uathcmd", 2 * hz);
	splx(s);
	if (error != 0) {
		printf("%s: timeout waiting for command reply\n",
		    USBDEVNAME(sc->sc_dev));
	}
	return error;
}

Static int
uath_cmd_write(struct uath_softc *sc, uint32_t code, const void *data, int len,
    int flags)
{
	flags &= ~UATH_CMD_FLAG_READ;
	return uath_cmd(sc, code, data, len, NULL, flags);
}

Static int
uath_cmd_read(struct uath_softc *sc, uint32_t code, const void *idata,
    int ilen, void *odata, int flags)
{
	flags |= UATH_CMD_FLAG_READ;
	return uath_cmd(sc, code, idata, ilen, odata, flags);
}

Static int
uath_write_reg(struct uath_softc *sc, uint32_t reg, uint32_t val)
{
	struct uath_write_mac write;
	int error;

	write.reg = htobe32(reg);
	write.len = htobe32(0);	/* 0 = single write */
	*(uint32_t *)write.data = htobe32(val);

	error = uath_cmd_write(sc, UATH_CMD_WRITE_MAC, &write,
	    3 * sizeof (uint32_t), 0);
	if (error != 0) {
		printf("%s: could not write register 0x%02x\n",
		    USBDEVNAME(sc->sc_dev), reg);
	}
	return error;
}

Static int
uath_write_multi(struct uath_softc *sc, uint32_t reg, const void *data,
    int len)
{
	struct uath_write_mac write;
	int error;

	write.reg = htobe32(reg);
	write.len = htobe32(len);
	bcopy(data, write.data, len);

	/* properly handle the case where len is zero (reset) */
	error = uath_cmd_write(sc, UATH_CMD_WRITE_MAC, &write,
	    (len == 0) ? sizeof (uint32_t) : 2 * sizeof (uint32_t) + len, 0);
	if (error != 0) {
		printf("%s: could not write %d bytes to register 0x%02x\n",
		    USBDEVNAME(sc->sc_dev), len, reg);
	}
	return error;
}

Static int
uath_read_reg(struct uath_softc *sc, uint32_t reg, uint32_t *val)
{
	struct uath_read_mac read;
	int error;

	reg = htobe32(reg);
	error = uath_cmd_read(sc, UATH_CMD_READ_MAC, &reg, sizeof reg, &read,
	    0);
	if (error != 0) {
		printf("%s: could not read register 0x%02x\n",
		    USBDEVNAME(sc->sc_dev), betoh32(reg));
		return error;
	}
	*val = betoh32(*(uint32_t *)read.data);
	return error;
}

Static int
uath_read_eeprom(struct uath_softc *sc, uint32_t reg, void *odata)
{
	struct uath_read_mac read;
	int len, error;

	reg = htobe32(reg);
	error = uath_cmd_read(sc, UATH_CMD_READ_EEPROM, &reg, sizeof reg,
	    &read, 0);
	if (error != 0) {
		printf("%s: could not read EEPROM offset 0x%02x\n",
		    USBDEVNAME(sc->sc_dev), betoh32(reg));
		return error;
	}
	len = betoh32(read.len);
	bcopy(read.data, odata, (len == 0) ? sizeof (uint32_t) : len);
	return error;
}

Static void
uath_cmd_rxeof(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status)
{
	struct uath_rx_cmd *cmd = priv;
	struct uath_softc *sc = cmd->sc;
	struct uath_cmd_hdr *hdr;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->cmd_rx_pipe);
		return;
	}

	hdr = (struct uath_cmd_hdr *)cmd->buf;

#ifdef UATH_DEBUG
	if (uath_debug >= 5) {
		printf("received command code=0x%x index=%u len=%u",
		    betoh32(hdr->code), hdr->priv, betoh32(hdr->len));
		uath_dump_cmd(cmd->buf, betoh32(hdr->len), '-');
	}
#endif

	switch (betoh32(hdr->code) & 0xff) {
	/* reply to a read command */
	default:
	{
		struct uath_tx_cmd *txcmd = &sc->tx_cmd[hdr->priv];

		if (txcmd->odata != NULL) {
			/* copy answer into caller's supplied buffer */
			bcopy((uint8_t *)(hdr + 1), txcmd->odata,
			    betoh32(hdr->len) - sizeof (struct uath_cmd_hdr));
		}
		wakeup(txcmd);	/* wake up caller */
		break;
	}
	/* spontaneous firmware notifications */
	case UATH_NOTIF_READY:
		DPRINTF(("received device ready notification\n"));
		wakeup(UATH_COND_INIT(sc));
		break;

	case UATH_NOTIF_TX:
		/* this notification is sent when UATH_TX_NOTIFY is set */
		DPRINTF(("received Tx notification\n"));
		break;

	case UATH_NOTIF_STATS:
		DPRINTFN(2, ("received device statistics\n"));
		timeout_add(&sc->stat_to, hz);
		break;
	}

	/* setup a new transfer */
	usbd_setup_xfer(xfer, sc->cmd_rx_pipe, cmd, cmd->buf, UATH_MAX_RXCMDSZ,
	    USBD_SHORT_XFER_OK | USBD_NO_COPY, USBD_NO_TIMEOUT,
	    uath_cmd_rxeof);
	(void)usbd_transfer(xfer);
}

Static void
uath_data_rxeof(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status)
{
	struct uath_rx_data *data = priv;
	struct uath_softc *sc = data->sc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct uath_rx_desc *desc;
	struct mbuf *mnew, *m;
	uint32_t hdr;
	int s, len;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->data_rx_pipe);

		ifp->if_ierrors++;
		return;
	}
	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	if (len < UATH_MIN_RXBUFSZ || len > sc->rxbufsz) {
		DPRINTF(("wrong xfer size: !(%d <= %d <= %d)\n",
		    UATH_MIN_RXBUFSZ, len, sc->rxbufsz));
		ifp->if_ierrors++;
		goto skip;
	}

	hdr = betoh32(*(uint32_t *)data->buf);

	/* Rx descriptor is located at the end, 32-bit aligned */
	desc = (struct uath_rx_desc *)
	    (data->buf + len - sizeof (struct uath_rx_desc));

	/* there's probably a "bad CRC" flag somewhere in the descriptor.. */

	MGETHDR(mnew, M_DONTWAIT, MT_DATA);
	if (mnew == NULL) {
		printf("%s: could not allocate rx mbuf\n",
		    USBDEVNAME(sc->sc_dev));
		ifp->if_ierrors++;
		goto skip;
	}
	MCLGET(mnew, M_DONTWAIT);
	if (!(mnew->m_flags & M_EXT)) {
		printf("%s: could not allocate rx mbuf cluster\n",
		    USBDEVNAME(sc->sc_dev));
		m_freem(mnew);
		ifp->if_ierrors++;
		goto skip;
	}

	m = data->m;
	data->m = mnew;

	/* finalize mbuf */
	m->m_pkthdr.rcvif = ifp;
	m->m_data = data->buf + sizeof (uint32_t);
	m->m_pkthdr.len = m->m_len =
	    betoh32(desc->len) - sizeof (struct uath_rx_desc);

	data->buf = mtod(data->m, uint8_t *);

	wh = mtod(m, struct ieee80211_frame *);
	if ((wh->i_fc[1] & IEEE80211_FC1_WEP) &&
	    ic->ic_opmode != IEEE80211_M_MONITOR) {
		/*
		 * Hardware decrypts the frame itself but leaves the WEP bit
		 * set in the 802.11 header and doesn't remove the IV and CRC
		 * fields.
		 */
		wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
		ovbcopy(wh, (caddr_t)wh + IEEE80211_WEP_IVLEN +
		    IEEE80211_WEP_KIDLEN, sizeof (struct ieee80211_frame));
		m_adj(m, IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN);
		m_adj(m, -IEEE80211_WEP_CRCLEN);
		wh = mtod(m, struct ieee80211_frame *);
	}

#if NBPFILTER > 0
	/* there are a lot more fields in the Rx descriptor */
	if (sc->sc_drvbpf != NULL) {
		struct mbuf mb;
		struct uath_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_flags = 0;
		tap->wr_chan_freq = htole16(betoh32(desc->freq));
		tap->wr_chan_flags = htole16(ic->ic_bss->ni_chan->ic_flags);
		tap->wr_dbm_antsignal = (int8_t)betoh32(desc->rssi);

		M_DUP_PKTHDR(&mb, m);
		mb.m_data = (caddr_t)tap;
		mb.m_len = sc->sc_rxtap_len;
		mb.m_next = m;
		mb.m_pkthdr.len += mb.m_len;
		bpf_mtap(sc->sc_drvbpf, &mb, BPF_DIRECTION_IN);
	}
#endif

	s = splnet();
	ni = ieee80211_find_rxnode(ic, wh);
	ieee80211_input(ifp, m, ni, (int)betoh32(desc->rssi), 0);

	/* node is no longer needed */
	ieee80211_release_node(ic, ni);
	splx(s);

skip:	/* setup a new transfer */
	usbd_setup_xfer(xfer, sc->data_rx_pipe, data, data->buf, sc->rxbufsz,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, uath_data_rxeof);
	(void)usbd_transfer(xfer);
}

Static int
uath_tx_null(struct uath_softc *sc)
{
	struct uath_tx_data *data;
	struct uath_tx_desc *desc;

	data = &sc->tx_data[sc->data_idx];

	data->ni = NULL;

	*(uint32_t *)data->buf = UATH_MAKECTL(1, sizeof (struct uath_tx_desc));
	desc = (struct uath_tx_desc *)(data->buf + sizeof (uint32_t));

	bzero(desc, sizeof (struct uath_tx_desc));
	desc->len  = htobe32(sizeof (struct uath_tx_desc));
	desc->type = htobe32(UATH_TX_NULL);

	usbd_setup_xfer(data->xfer, sc->data_tx_pipe, data, data->buf,
	    sizeof (uint32_t) + sizeof (struct uath_tx_desc), USBD_NO_COPY |
	    USBD_FORCE_SHORT_XFER, UATH_DATA_TIMEOUT, NULL);
	if (usbd_sync_transfer(data->xfer) != 0)
		return EIO;

	sc->data_idx = (sc->data_idx + 1) % UATH_TX_DATA_LIST_COUNT;

	return uath_cmd_write(sc, UATH_CMD_0F, NULL, 0, UATH_CMD_FLAG_ASYNC);
}

Static void
uath_data_txeof(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status)
{
	struct uath_tx_data *data = priv;
	struct uath_softc *sc = data->sc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int s;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		printf("%s: could not transmit buffer: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(status));

		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->data_tx_pipe);

		ifp->if_oerrors++;
		return;
	}

	s = splnet();

	ieee80211_release_node(ic, data->ni);
	data->ni = NULL;

	sc->tx_queued--;
	ifp->if_opackets++;

	sc->sc_tx_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;
	uath_start(ifp);

	splx(s);
}

Static int
uath_tx_data(struct uath_softc *sc, struct mbuf *m0, struct ieee80211_node *ni)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct uath_tx_data *data;
	struct uath_tx_desc *desc;
	const struct ieee80211_frame *wh;
	int paylen, totlen, xferlen, error;

	data = &sc->tx_data[sc->data_idx];
	desc = (struct uath_tx_desc *)(data->buf + sizeof (uint32_t));

	data->ni = ni;

#if NBPFILTER > 0
	if (sc->sc_drvbpf != NULL) {
		struct mbuf mb;
		struct uath_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		tap->wt_chan_freq = htole16(ic->ic_bss->ni_chan->ic_freq);
		tap->wt_chan_flags = htole16(ic->ic_bss->ni_chan->ic_flags);

		M_DUP_PKTHDR(&mb, m0);
		mb.m_data = (caddr_t)tap;
		mb.m_len = sc->sc_txtap_len;
		mb.m_next = m0;
		mb.m_pkthdr.len += mb.m_len;
		bpf_mtap(sc->sc_drvbpf, &mb, BPF_DIRECTION_OUT);
	}
#endif

	paylen = m0->m_pkthdr.len;
	xferlen = sizeof (uint32_t) + sizeof (struct uath_tx_desc) + paylen;

	wh = mtod(m0, struct ieee80211_frame *);
	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		uint8_t *frm = (uint8_t *)(desc + 1);
		uint32_t iv;

		/* h/w WEP: it's up to the host to fill the IV field */
		bcopy(wh, frm, sizeof (struct ieee80211_frame));
		frm += sizeof (struct ieee80211_frame);

		/* insert IV: code copied from net80211 */
		iv = (ic->ic_iv != 0) ? ic->ic_iv : arc4random();
		if (iv >= 0x03ff00 && (iv & 0xf8ff00) == 0x00ff00)
			iv += 0x000100;
		ic->ic_iv = iv + 1;

		*frm++ = iv & 0xff;
		*frm++ = (iv >>  8) & 0xff;
		*frm++ = (iv >> 16) & 0xff;
		*frm++ = ic->ic_wep_txkey << 6;

		m_copydata(m0, sizeof (struct ieee80211_frame),
		    m0->m_pkthdr.len - sizeof (struct ieee80211_frame), frm);

		paylen  += IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
		xferlen += IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
		totlen = xferlen + IEEE80211_WEP_CRCLEN;
	} else {
		m_copydata(m0, 0, m0->m_pkthdr.len, (uint8_t *)(desc + 1));
		totlen = xferlen;
	}

	/* fill Tx descriptor */
	*(uint32_t *)data->buf = UATH_MAKECTL(1, xferlen - sizeof (uint32_t));

	desc->len    = htobe32(totlen);
	desc->priv   = sc->data_idx;	/* don't care about endianness */
	desc->paylen = htobe32(paylen);
	desc->type   = htobe32(UATH_TX_DATA);
	desc->flags  = htobe32(0);
	if (IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		desc->dest  = htobe32(UATH_ID_BROADCAST);
		desc->magic = htobe32(3);
	} else {
		desc->dest  = htobe32(UATH_ID_BSS);
		desc->magic = htobe32(1);
	}

	m_freem(m0);	/* mbuf is no longer needed */

#ifdef UATH_DEBUG
	if (uath_debug >= 6) {
		printf("sending frame index=%u len=%d xferlen=%d",
		    sc->data_idx, paylen, xferlen);
		uath_dump_cmd(data->buf, xferlen, '+');
	}
#endif
	usbd_setup_xfer(data->xfer, sc->data_tx_pipe, data, data->buf, xferlen,
	    USBD_FORCE_SHORT_XFER | USBD_NO_COPY, UATH_DATA_TIMEOUT,
	    uath_data_txeof);
	error = usbd_transfer(data->xfer);
	if (error != USBD_IN_PROGRESS && error != 0) {
		ic->ic_if.if_oerrors++;
		return error;
	}
	sc->data_idx = (sc->data_idx + 1) % UATH_TX_DATA_LIST_COUNT;
	sc->tx_queued++;

	return 0;
}

Static void
uath_start(struct ifnet *ifp)
{
	struct uath_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct mbuf *m0;

	/*
	 * net80211 may still try to send management frames even if the
	 * IFF_RUNNING flag is not set...
	 */
	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IF_POLL(&ic->ic_mgtq, m0);
		if (m0 != NULL) {
			if (sc->tx_queued >= UATH_TX_DATA_LIST_COUNT) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IF_DEQUEUE(&ic->ic_mgtq, m0);

			ni = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
			m0->m_pkthdr.rcvif = NULL;
#if NBPFILTER > 0
			if (ic->ic_rawbpf != NULL)
				bpf_mtap(ic->ic_rawbpf, m0, BPF_DIRECTION_OUT);
#endif
			if (uath_tx_data(sc, m0, ni) != 0)
				break;
		} else {
			if (ic->ic_state != IEEE80211_S_RUN)
				break;
			IFQ_DEQUEUE(&ifp->if_snd, m0);
			if (m0 == NULL)
				break;
			if (sc->tx_queued >= UATH_TX_DATA_LIST_COUNT) {
				IF_PREPEND(&ifp->if_snd, m0);
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
#if NBPFILTER > 0
			if (ifp->if_bpf != NULL)
				bpf_mtap(ifp->if_bpf, m0, BPF_DIRECTION_OUT);
#endif
			m0 = ieee80211_encap(ifp, m0, &ni);
			if (m0 == NULL)
				continue;
#if NBPFILTER > 0
			if (ic->ic_rawbpf != NULL)
				bpf_mtap(ic->ic_rawbpf, m0, BPF_DIRECTION_OUT);
#endif
			if (uath_tx_data(sc, m0, ni) != 0) {
				if (ni != NULL)
					ieee80211_release_node(ic, ni);
				ifp->if_oerrors++;
				break;
			}
		}

		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

Static void
uath_watchdog(struct ifnet *ifp)
{
	struct uath_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n", USBDEVNAME(sc->sc_dev));
			/*uath_init(ifp); XXX needs a process context! */
			ifp->if_oerrors++;
			return;
		}
		ifp->if_timer = 1;
	}

	ieee80211_watchdog(ifp);
}

Static int
uath_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct uath_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifaddr *ifa;
	struct ifreq *ifr;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&ic->ic_ac, ifa);
#endif
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (!(ifp->if_flags & IFF_RUNNING))
				uath_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				uath_stop(ifp, 1);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &ic->ic_ac) :
		    ether_delmulti(ifr, &ic->ic_ac);
		if (error == ENETRESET)
			error = 0;
		break;

	default:
		error = ieee80211_ioctl(ifp, cmd, data);
	}

	if (error == ENETRESET) {
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING))
			uath_init(ifp);
		error = 0;
	}

	splx(s);

	return error;
}

Static int
uath_query_eeprom(struct uath_softc *sc)
{
	uint32_t tmp;
	int error;

	/* retrieve MAC address */
	error = uath_read_eeprom(sc, UATH_EEPROM_MACADDR, sc->sc_ic.ic_myaddr);
	if (error != 0) {
		printf("%s: could not read MAC address\n",
		    USBDEVNAME(sc->sc_dev));
		return error;
	}

	/* retrieve the maximum frame size that the hardware can receive */
	error = uath_read_eeprom(sc, UATH_EEPROM_RXBUFSZ, &tmp);
	if (error != 0) {
		printf("%s: could not read maximum Rx buffer size\n",
		    USBDEVNAME(sc->sc_dev));
		return error;
	}
	sc->rxbufsz = betoh32(tmp) & 0xfff;
	DPRINTF(("maximum Rx buffer size %d\n", sc->rxbufsz));
	return 0;
}

Static int
uath_reset(struct uath_softc *sc)
{
	struct uath_cmd_setup setup;
	uint32_t reg, val;
	int s, error;

	/* init device with some voodoo incantations.. */
	setup.magic1 = htobe32(1);
	setup.magic2 = htobe32(5);
	setup.magic3 = htobe32(200);
	setup.magic4 = htobe32(27);
	s = splusb();
	error = uath_cmd_write(sc, UATH_CMD_SETUP, &setup, sizeof setup,
	    UATH_CMD_FLAG_ASYNC);
	/* ..and wait until firmware notifies us that it is ready */
	if (error == 0)
		error = tsleep(UATH_COND_INIT(sc), PCATCH, "uathinit", 5 * hz);
	splx(s);

	/* read PHY registers */
	for (reg = 0x09; reg <= 0x24; reg++) {
		if (reg == 0x0b || reg == 0x0c)
			continue;
		if ((error = uath_read_reg(sc, reg, &val)) != 0)
			return error;
		DPRINTFN(2, ("reg 0x%02x=0x%08x\n", reg, val));
	}
	return error;
}

Static int
uath_reset_tx_queues(struct uath_softc *sc)
{
	int ac, error;

	for (ac = 0; ac < 4; ac++) {
		const uint32_t qid = htobe32(UATH_AC_TO_QID(ac));

		DPRINTF(("resetting Tx queue %d\n", UATH_AC_TO_QID(ac)));
		error = uath_cmd_write(sc, UATH_CMD_RESET_QUEUE, &qid,
		    sizeof qid, 0);
		if (error != 0)
			break;
	}
	return error;
}

Static int
uath_wme_init(struct uath_softc *sc)
{
	struct uath_qinfo qinfo;
	int ac, error;
	static const struct uath_wme_settings uath_wme_11g[4] = {
		{ 7, 4, 10,  0, 0 },	/* Background */
		{ 3, 4, 10,  0, 0 },	/* Best-Effort */
		{ 3, 3,  4, 26, 0 },	/* Video */
		{ 2, 2,  3, 47, 0 }	/* Voice */
	};

	bzero(&qinfo, sizeof qinfo);
	qinfo.size   = htobe32(32);
	qinfo.magic1 = htobe32(1);	/* XXX ack policy? */
	qinfo.magic2 = htobe32(1);
	for (ac = 0; ac < 4; ac++) {
		qinfo.qid      = htobe32(UATH_AC_TO_QID(ac));
		qinfo.ac       = htobe32(ac);
		qinfo.aifsn    = htobe32(uath_wme_11g[ac].aifsn);
		qinfo.logcwmin = htobe32(uath_wme_11g[ac].logcwmin);
		qinfo.logcwmax = htobe32(uath_wme_11g[ac].logcwmax);
		qinfo.txop     = htobe32(UATH_TXOP_TO_US(
				     uath_wme_11g[ac].txop));
		qinfo.acm      = htobe32(uath_wme_11g[ac].acm);

		DPRINTF(("setting up Tx queue %d\n", UATH_AC_TO_QID(ac)));
		error = uath_cmd_write(sc, UATH_CMD_SET_QUEUE, &qinfo,
		    sizeof qinfo, 0);
		if (error != 0)
			break;
	}
	return error;
}

Static int
uath_set_chan(struct uath_softc *sc, struct ieee80211_channel *c)
{
#ifdef UATH_DEBUG
	struct ieee80211com *ic = &sc->sc_ic;
#endif
	struct uath_set_chan chan;

	bzero(&chan, sizeof chan);
	chan.flags  = htobe32(0x1400);
	chan.freq   = htobe32(c->ic_freq);
	chan.magic1 = htobe32(20);
	chan.magic2 = htobe32(50);
	chan.magic3 = htobe32(1);

	DPRINTF(("switching to channel %d\n", ieee80211_chan2ieee(ic, c)));
	return uath_cmd_write(sc, UATH_CMD_SET_CHAN, &chan, sizeof chan, 0);
}

Static int
uath_set_key(struct uath_softc *sc, const struct ieee80211_wepkey *wk,
    int index)
{
	struct uath_cmd_crypto crypto;
	int i;

	bzero(&crypto, sizeof crypto);
	crypto.keyidx = htobe32(index);
	crypto.magic1 = htobe32(1);
	crypto.size   = htobe32(368);
	crypto.mask   = htobe32(0xffff);
	crypto.flags  = htobe32(0x80000068);
	if (index != UATH_DEFAULT_KEY)
		crypto.flags |= htobe32(index << 16);
	memset(crypto.magic2, 0xff, sizeof crypto.magic2);

	/*
	 * Each byte of the key must be XOR'ed with 10101010 before being
	 * transmitted to the firmware.
	 */
	for (i = 0; i < wk->wk_len; i++)
		crypto.key[i] = wk->wk_key[i] ^ 0xaa;

	DPRINTF(("setting crypto key index=%d len=%d\n", index, wk->wk_len));
	return uath_cmd_write(sc, UATH_CMD_CRYPTO, &crypto, sizeof crypto, 0);
}

Static int
uath_set_keys(struct uath_softc *sc)
{
	const struct ieee80211com *ic = &sc->sc_ic;
	int i, error;

	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		const struct ieee80211_wepkey *wk = &ic->ic_nw_keys[i];

		if (wk->wk_len > 0 &&
		    (error = uath_set_key(sc, wk, i)) != 0)
			return error;
	}
	return uath_set_key(sc, &ic->ic_nw_keys[ic->ic_wep_txkey],
	    UATH_DEFAULT_KEY);
}

Static int
uath_set_rates(struct uath_softc *sc, const struct ieee80211_rateset *rs)
{
	struct uath_cmd_rates rates;

	bzero(&rates, sizeof rates);
	rates.magic1 = htobe32(0x02);
	rates.magic2 = htobe32(0x21);
	rates.nrates = rs->rs_nrates;
	bcopy(rs->rs_rates, rates.rates, rs->rs_nrates);

	DPRINTF(("setting supported rates nrates=%d\n", rs->rs_nrates));
	return uath_cmd_write(sc, UATH_CMD_SET_RATES, &rates,
	    3 * 4 + 1 + rs->rs_nrates, 0);
}

Static int
uath_set_rxfilter(struct uath_softc *sc, uint32_t filter, uint32_t flags)
{
	struct uath_cmd_filter rxfilter;

	rxfilter.filter = htobe32(filter);
	rxfilter.flags  = htobe32(flags);

	DPRINTF(("setting Rx filter=0x%x flags=0x%x\n", filter, flags));
	return uath_cmd_write(sc, UATH_CMD_SET_FILTER, &rxfilter,
	    sizeof rxfilter, 0);
}

Static int
uath_set_led(struct uath_softc *sc, int which, int on)
{
	struct uath_cmd_led led;

	led.which = htobe32(which);
	led.state = htobe32(on ? UATH_LED_ON : UATH_LED_OFF);

	DPRINTFN(2, ("switching %s led %s\n",
	    (which == UATH_LED_LINK) ? "link" : "activity",
	    on ? "on" : "off"));
	return uath_cmd_write(sc, UATH_CMD_SET_LED, &led, sizeof led, 0);
}

Static int
uath_switch_channel(struct uath_softc *sc, struct ieee80211_channel *c)
{
	uint32_t val;
	int error;

	/* set radio frequency */
	if ((error = uath_set_chan(sc, c)) != 0) {
		printf("%s: could not set channel\n", USBDEVNAME(sc->sc_dev));
		return error;
	}

	/* reset Tx rings */
	if ((error = uath_reset_tx_queues(sc)) != 0) {
		printf("%s: could not reset Tx queues\n",
		    USBDEVNAME(sc->sc_dev));
		return error;
	}

	/* set Tx rings WME properties */
	if ((error = uath_wme_init(sc)) != 0) {
		printf("%s: could not init Tx queues\n",
		    USBDEVNAME(sc->sc_dev));
		return error;
	}

	val = htobe32(0);
	error = uath_cmd_write(sc, UATH_CMD_SET_STATE, &val, sizeof val, 0);
	if (error != 0) {
		printf("%s: could not set state\n", USBDEVNAME(sc->sc_dev));
		return error;
	}

	return uath_tx_null(sc);
}

Static int
uath_init(struct ifnet *ifp)
{
	struct uath_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct uath_cmd_31 cmd31;
	uint32_t val;
	int i, error;

	/* reset data and command rings */
	sc->tx_queued = sc->data_idx = sc->cmd_idx = 0;

	val = htobe32(0);
	(void)uath_cmd_write(sc, UATH_CMD_02, &val, sizeof val, 0);

	/* set MAC address */
	IEEE80211_ADDR_COPY(ic->ic_myaddr, LLADDR(ifp->if_sadl));
	(void)uath_write_multi(sc, 0x13, ic->ic_myaddr, IEEE80211_ADDR_LEN);

	(void)uath_write_reg(sc, 0x02, 0x00000001);
	(void)uath_write_reg(sc, 0x0e, 0x0000003f);
	(void)uath_write_reg(sc, 0x10, 0x00000001);
	(void)uath_write_reg(sc, 0x06, 0x0000001e);

	/*
	 * Queue Rx data xfers.
	 */
	for (i = 0; i < UATH_RX_DATA_LIST_COUNT; i++) {
		struct uath_rx_data *data = &sc->rx_data[i];

		usbd_setup_xfer(data->xfer, sc->data_rx_pipe, data, data->buf,
		    sc->rxbufsz, USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT,
		    uath_data_rxeof);
		error = usbd_transfer(data->xfer);
		if (error != USBD_IN_PROGRESS && error != 0) {
			printf("%s: could not queue Rx transfer\n",
			    USBDEVNAME(sc->sc_dev));
			goto fail;
		}
	}

	error = uath_cmd_read(sc, UATH_CMD_07, 0, NULL, &val,
	    UATH_CMD_FLAG_MAGIC);
	if (error != 0) {
		printf("%s: could not send read command 07h\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail;
	}
	DPRINTF(("command 07h return code: %x\n", betoh32(val)));

	/* set default channel */
	ic->ic_bss->ni_chan = ic->ic_ibss_chan;
	if ((error = uath_set_chan(sc, ic->ic_bss->ni_chan)) != 0) {
		printf("%s: could not set channel\n", USBDEVNAME(sc->sc_dev));
		goto fail;
	}

	if ((error = uath_wme_init(sc)) != 0) {
		printf("%s: could not setup WME parameters\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail;
	}

	/* init MAC registers */
	(void)uath_write_reg(sc, 0x19, 0x00000000);
	(void)uath_write_reg(sc, 0x1a, 0x0000003c);
	(void)uath_write_reg(sc, 0x1b, 0x0000003c);
	(void)uath_write_reg(sc, 0x1c, 0x00000000);
	(void)uath_write_reg(sc, 0x1e, 0x00000000);
	(void)uath_write_reg(sc, 0x1f, 0x00000003);
	(void)uath_write_reg(sc, 0x0c, 0x00000000);
	(void)uath_write_reg(sc, 0x0f, 0x00000002);
	(void)uath_write_reg(sc, 0x0a, 0x00000007);	/* XXX retry? */
	(void)uath_write_reg(sc, 0x09, ic->ic_rtsthreshold);

	val = htobe32(4);
	(void)uath_cmd_write(sc, UATH_CMD_27, &val, sizeof val, 0);
	(void)uath_cmd_write(sc, UATH_CMD_27, &val, sizeof val, 0);
	(void)uath_cmd_write(sc, UATH_CMD_1B, NULL, 0, 0);

	if ((error = uath_set_keys(sc)) != 0) {
		printf("%s: could not set crypto keys\n",
		    USBDEVNAME(sc->sc_dev));
		goto fail;
	}

	/* enable Rx */
	(void)uath_set_rxfilter(sc, 0x0000, 4);
	(void)uath_set_rxfilter(sc, 0x0817, 1);

	cmd31.magic1 = htobe32(0xffffffff);
	cmd31.magic2 = htobe32(0xffffffff);
	(void)uath_cmd_write(sc, UATH_CMD_31, &cmd31, sizeof cmd31, 0);

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
	else
		ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);

	return 0;

fail:	uath_stop(ifp, 1);
	return error;
}

Static void
uath_stop(struct ifnet *ifp, int disable)
{
	struct uath_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t val;
	int s;

	s = splusb();

	sc->sc_tx_timer = 0;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);	/* free all nodes */

	val = htobe32(0);
	(void)uath_cmd_write(sc, UATH_CMD_SET_STATE, &val, sizeof val, 0);
	(void)uath_cmd_write(sc, UATH_CMD_RESET, NULL, 0, 0);

	val = htobe32(0);
	(void)uath_cmd_write(sc, UATH_CMD_15, &val, sizeof val, 0);

#if 0
	(void)uath_cmd_read(sc, UATH_CMD_SHUTDOWN, NULL, 0, NULL,
	    UATH_CMD_FLAG_MAGIC);
#endif

	/* abort any pending transfers */
	usbd_abort_pipe(sc->data_tx_pipe);
	usbd_abort_pipe(sc->data_rx_pipe);
	usbd_abort_pipe(sc->cmd_tx_pipe);
	usbd_abort_pipe(sc->cmd_rx_pipe);

	splx(s);
}

/*
 * Load the MIPS R4000 microcode into the device.  Once the image is loaded,
 * the device will detach itself from the bus and reattach later with a new
 * product Id (a la ezusb).  XXX this could also be implemented in userland
 * through /dev/ugen.
 */
Static int
uath_loadfirmware(struct uath_softc *sc, const u_char *fw, int len)
{
	usbd_xfer_handle ctlxfer, txxfer, rxxfer;
	struct uath_fwblock *txblock, *rxblock;
	uint8_t *txdata;
	int error = 0;

	if ((ctlxfer = usbd_alloc_xfer(sc->sc_udev)) == NULL) {
		printf("%s: could not allocate Tx control xfer\n",
		    USBDEVNAME(sc->sc_dev));
		error = USBD_NOMEM;
		goto fail1;
	}
	txblock = usbd_alloc_buffer(ctlxfer, sizeof (struct uath_fwblock));
	if (txblock == NULL) {
		printf("%s: could not allocate Tx control block\n",
		    USBDEVNAME(sc->sc_dev));
		error = USBD_NOMEM;
		goto fail2;
	}

	if ((txxfer = usbd_alloc_xfer(sc->sc_udev)) == NULL) {
		printf("%s: could not allocate Tx xfer\n",
		    USBDEVNAME(sc->sc_dev));
		error = USBD_NOMEM;
		goto fail2;
	}
	txdata = usbd_alloc_buffer(txxfer, UATH_MAX_FWBLOCK_SIZE);
	if (txdata == NULL) {
		printf("%s: could not allocate Tx buffer\n",
		    USBDEVNAME(sc->sc_dev));
		error = USBD_NOMEM;
		goto fail3;
	}

	if ((rxxfer = usbd_alloc_xfer(sc->sc_udev)) == NULL) {
		printf("%s: could not allocate Rx control xfer\n",
		    USBDEVNAME(sc->sc_dev));
		error = USBD_NOMEM;
		goto fail3;
	}
	rxblock = usbd_alloc_buffer(rxxfer, sizeof (struct uath_fwblock));
	if (rxblock == NULL) {
		printf("%s: could not allocate Rx control block\n",
		    USBDEVNAME(sc->sc_dev));
		error = USBD_NOMEM;
		goto fail4;
	}

	bzero(txblock, sizeof (struct uath_fwblock));
	txblock->flags = htobe32(UATH_WRITE_BLOCK);
	txblock->total = htobe32(len);

	while (len > 0) {
		int mlen = min(len, UATH_MAX_FWBLOCK_SIZE);

		txblock->remain = htobe32(len - mlen);
		txblock->len = htobe32(mlen);

		DPRINTF(("sending firmware block: %d bytes remaining\n",
		    len - mlen));

		/* send firmware block meta-data */
		usbd_setup_xfer(ctlxfer, sc->cmd_tx_pipe, sc, txblock,
		    sizeof (struct uath_fwblock), USBD_NO_COPY,
		    UATH_CMD_TIMEOUT, NULL);
		if ((error = usbd_sync_transfer(ctlxfer)) != 0) {
			printf("%s: could not send firmware block info\n",
			    USBDEVNAME(sc->sc_dev));
			break;
		}

		/* send firmware block data */
		bcopy(fw, txdata, mlen);
		usbd_setup_xfer(txxfer, sc->data_tx_pipe, sc, txdata, mlen,
		    USBD_NO_COPY, UATH_DATA_TIMEOUT, NULL);
		if ((error = usbd_sync_transfer(txxfer)) != 0) {
			printf("%s: could not send firmware block data\n",
			    USBDEVNAME(sc->sc_dev));
			break;
		}

		/* wait for ack from firmware */
		usbd_setup_xfer(rxxfer, sc->cmd_rx_pipe, sc, rxblock,
		    sizeof (struct uath_fwblock), USBD_SHORT_XFER_OK |
		    USBD_NO_COPY, UATH_CMD_TIMEOUT, NULL);
		if ((error = usbd_sync_transfer(rxxfer)) != 0) {
			printf("%s: could not read firmware answer\n",
			    USBDEVNAME(sc->sc_dev));
			break;
		}

		DPRINTFN(2, ("rxblock flags=0x%x total=%d\n",
		    betoh32(rxblock->flags), betoh32(rxblock->rxtotal)));
		fw += mlen;
		len -= mlen;
	}

fail4:	usbd_free_xfer(rxxfer);
fail3:	usbd_free_xfer(txxfer);
fail2:	usbd_free_xfer(ctlxfer);
fail1:	return error;
}

Static int
uath_activate(device_ptr_t self, enum devact act)
{
	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		/*if_deactivate(&sc->sc_ic.ic_if);*/
		break;
	}
	return 0;
}
