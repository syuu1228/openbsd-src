/*	$OpenBSD: src/sys/dev/usb/uftdi.c,v 1.55 2009/12/16 21:41:29 deraadt Exp $ 	*/
/*	$NetBSD: uftdi.c,v 1.14 2003/02/23 04:20:07 simonb Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * FTDI FT8U100AX serial adapter driver
 */

/*
 * XXX This driver will not support multiple serial ports.
 * XXX The ucom layer needs to be extended first.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/ucomvar.h>

#include <dev/usb/uftdireg.h>

#ifdef UFTDI_DEBUG
#define DPRINTF(x)	do { if (uftdidebug) printf x; } while (0)
#define DPRINTFN(n,x)	do { if (uftdidebug>(n)) printf x; } while (0)
int uftdidebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UFTDI_CONFIG_INDEX	0
#define UFTDI_IFACE_INDEX	0


/*
 * These are the maximum number of bytes transferred per frame.
 * The output buffer size cannot be increased due to the size encoding.
 */
#define UFTDIIBUFSIZE 64
#define UFTDIOBUFSIZE 64

struct uftdi_softc {
	struct device		 sc_dev;		/* base device */
	usbd_device_handle	 sc_udev;	/* device */
	usbd_interface_handle	 sc_iface;	/* interface */

	enum uftdi_type		 sc_type;
	u_int			 sc_hdrlen;

	u_char			 sc_msr;
	u_char			 sc_lsr;

	struct device		*sc_subdev;

	u_char			 sc_dying;

	u_int			 last_lcr;
};

void	uftdi_get_status(void *, int portno, u_char *lsr, u_char *msr);
void	uftdi_set(void *, int, int, int);
int	uftdi_param(void *, int, struct termios *);
int	uftdi_open(void *sc, int portno);
void	uftdi_read(void *sc, int portno, u_char **ptr,
			   u_int32_t *count);
void	uftdi_write(void *sc, int portno, u_char *to, u_char *from,
			    u_int32_t *count);
void	uftdi_break(void *sc, int portno, int onoff);
int	uftdi_8u232am_getrate(speed_t speed, int *rate);

struct ucom_methods uftdi_methods = {
	uftdi_get_status,
	uftdi_set,
	uftdi_param,
	NULL,
	uftdi_open,
	NULL,
	uftdi_read,
	uftdi_write,
};

int uftdi_match(struct device *, void *, void *); 
void uftdi_attach(struct device *, struct device *, void *); 
int uftdi_detach(struct device *, int); 
int uftdi_activate(struct device *, int); 

struct cfdriver uftdi_cd = { 
	NULL, "uftdi", DV_DULL 
}; 

const struct cfattach uftdi_ca = { 
	sizeof(struct uftdi_softc), 
	uftdi_match, 
	uftdi_attach, 
	uftdi_detach, 
	uftdi_activate, 
};

static const struct usb_devno uftdi_devs[] = {
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_232USB9M },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_485USB9F2W },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_485USB9F4W },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USO9ML2 },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USO9ML2DR },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USO9ML2DR2 },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USOPTL4 },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USOPTL4DR },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USOPTL4DR2 },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USOTL4 },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USPTL4 },
	{ USB_VENDOR_BBELECTR, USB_PRODUCT_BBELECTR_USTL4 },
	{ USB_VENDOR_EVOLUTION, USB_PRODUCT_EVOLUTION_ER1 },
	{ USB_VENDOR_EVOLUTION, USB_PRODUCT_EVOLUTION_RCM4_1 },
	{ USB_VENDOR_EVOLUTION, USB_PRODUCT_EVOLUTION_RCM4_2 },
	{ USB_VENDOR_FALCOM, USB_PRODUCT_FALCOM_SAMBA },
	{ USB_VENDOR_FALCOM, USB_PRODUCT_FALCOM_TWIST },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ACCESSO },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ACG_HFDUAL },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ACTROBOTS },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ACTZWAVE },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_AMC232 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ARTEMIS },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_3 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_4 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_5 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_6 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_7 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ASK_RDR4X7_8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ATK16 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ATK16C },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ATK16HR },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ATK16HRC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CANUSB },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CCS_ICDU20 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CCS_ICDU40 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CCS_MACHX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CHAMELEON },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_COASTAL_TNCX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_DMX4ALL },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ECLO_1WIRE },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ECO_PRO },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_EISCOU },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_ALC8500 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_CLI7000 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_CSI8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_EM1000DL },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_EM1010PC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_FHZ1000PC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_FHZ1300PC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_FS20SIG },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_PCD200 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_PCK100 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_PPS7330 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_RFP500 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_T1100 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_TFM100 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UAD7 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UAD8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UDF77 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UIO88 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_ULA200 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UM100 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UO100 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_UR100 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_USI2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_WS300PC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_ELV_WS500 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_FT232_1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_FT232_3 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_FT232_4 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_FT232_5 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_FT232_6 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GAMMASCOUT },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_3 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_4 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_5 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_6 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_7 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_9 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_A },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_GUDE_B },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_APP70 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_PCMCIA },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_PEDO },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_PICPRO },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_PK1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_RS232MON },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IBS_US485 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_IPLUS },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_547 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_631 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_632 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_633 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_634 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_635 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_640 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_642 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_LK202_24 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_LK204_24 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_MX200 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LINX_1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LINX_2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LINX_3 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LINX_MASTER2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LOCOBUFFER },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MATRIX_2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MATRIX_3 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_DB9 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_IC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_KW },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_RS232 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_Y6 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_Y8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_Y9 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_YS },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MJS_SIRIUS_PC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_OPENPORT_13M },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_OPENPORT_13S },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_OPENPORT_13U },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_OPENRD },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_PCDJ_DAC2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_PYRAMID },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SEMC_DSS20 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_2232C },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_2232L },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_232BM },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_8U100AX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_8U232AM },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_8U232AM4 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_TERATRONIK_D2XX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_TERATRONIK_VCP },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_THORLABS },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_TIRA1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_TNCX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_UOPTBR },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_USBSERIAL },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_USBUIRT },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_VNHC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_WESTREX_777 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_WESTREX_8900F },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_1 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_2 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_3 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_4 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_5 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_6 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_7 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_XSENS_8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_YEI_SC31 },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_ID1 },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2000VR },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2000VT },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2C1 },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2C2 },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2D },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2VR },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP2VT },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP4000VR },
	{ USB_VENDOR_ICOM, USB_PRODUCT_ICOM_RP4000VT },
	{ USB_VENDOR_IDTECH, USB_PRODUCT_IDTECH_SERIAL },
	{ USB_VENDOR_INTERBIO, USB_PRODUCT_INTERBIO_IOBOARD },
	{ USB_VENDOR_INTERBIO, USB_PRODUCT_INTERBIO_MINIIOBOARD },
	{ USB_VENDOR_INTREPIDCS, USB_PRODUCT_INTREPIDCS_VALUECAN },
	{ USB_VENDOR_INTREPIDCS, USB_PRODUCT_INTREPIDCS_NEOVI },
	{ USB_VENDOR_IODATA, USB_PRODUCT_IODATA_FT232R },
	{ USB_VENDOR_KOBIL, USB_PRODUCT_KOBIL_B1 },
	{ USB_VENDOR_KOBIL, USB_PRODUCT_KOBIL_KAAN },
	{ USB_VENDOR_MELCO, USB_PRODUCT_MELCO_PCOPRS1 },
	{ USB_VENDOR_MECANIQUE, USB_PRODUCT_MECANIQUE_TELLSTICK },
	{ USB_VENDOR_MOBILITY, USB_PRODUCT_MOBILITY_ED200H },
	{ USB_VENDOR_OCT, USB_PRODUCT_OCT_US2308 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_AD4USB },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_AP485_1 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_AP485_2 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_DRAK5 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_DRAK6 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_GOLIATH_MSR },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_GOLIATH_MUX },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_IRAMP },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_LEC },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_MUC },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO101 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO216 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO22 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO303 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO332 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO44 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO603 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_QUIDO88 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB232 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB422_1 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB422_2 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB485C },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB485S },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB485_1 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SB485_2 },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SERIAL },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_SIMUKEY },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_STAVOVY },
	{ USB_VENDOR_PAPOUCH, USB_PRODUCT_PAPOUCH_TMU },
	{ USB_VENDOR_POSIFLEX, USB_PRODUCT_POSIFLEX_PP7000_1 },
	{ USB_VENDOR_POSIFLEX, USB_PRODUCT_POSIFLEX_PP7000_2 },
	{ USB_VENDOR_RATOC, USB_PRODUCT_RATOC_REXUSB60F },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2101 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2102 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2103 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2104 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2106 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2201_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2201_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2202_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2202_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2203_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2203_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2401_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2401_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2401_3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2401_4 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2402_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2402_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2402_3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2402_4 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2403_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2403_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2403_3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2403_4 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_4 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_5 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_6 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_7 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2801_8 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_4 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_5 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_6 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_7 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2802_8 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_4 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_5 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_6 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_7 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_2803_8 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_174 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_175 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_330 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_435 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_556 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_580 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_845 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_SERIAL_1 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_SERIAL_2 },
	{ USB_VENDOR_TESTO, USB_PRODUCT_TESTO_SERVICE },
	{ USB_VENDOR_THURLBY, USB_PRODUCT_THURLBY_QL355P }
};

int
uftdi_match(struct device *parent, void *match, void *aux)
{
	struct usb_attach_arg *uaa = aux;
	usbd_status err;
	u_int8_t nifaces;

	if (usb_lookup(uftdi_devs, uaa->vendor, uaa->product) == NULL)
		return (UMATCH_NONE);

	/* Get the number of interfaces. */
	if (uaa->iface != NULL) {
		nifaces = uaa->nifaces;
	} else {
		err = usbd_set_config_index(uaa->device, UFTDI_CONFIG_INDEX, 1);
		if (err)
			return (UMATCH_NONE);
		err = usbd_interface_count(uaa->device, &nifaces);
		if (err)
			return (UMATCH_NONE);
		usbd_set_config_index(uaa->device, USB_UNCONFIG_INDEX, 1);
	}

	/* JTAG on USB interface 0 */
	if (uaa->vendor == USB_VENDOR_FTDI &&
	    uaa->product == USB_PRODUCT_FTDI_OPENRD &&
	    uaa->ifaceno == 0)
		return (UMATCH_NONE);

	if (nifaces <= 1)
		return (UMATCH_VENDOR_PRODUCT);

	/* Dual UART chip */
	if (uaa->iface != NULL)
		return (UMATCH_VENDOR_IFACESUBCLASS);
	else
		return (UMATCH_NONE);
}

void
uftdi_attach(struct device *parent, struct device *self, void *aux)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;
	struct usb_attach_arg *uaa = aux;
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devname = sc->sc_dev.dv_xname;
	int i;
	usbd_status err;
	struct ucom_attach_args uca;

	DPRINTFN(10,("\nuftdi_attach: sc=%p\n", sc));

	if (uaa->iface == NULL) {
		/* Move the device into the configured state. */
		err = usbd_set_config_index(dev, UFTDI_CONFIG_INDEX, 1);
		if (err) {
			printf("%s: failed to set configuration, err=%s\n",
			    sc->sc_dev.dv_xname, usbd_errstr(err));
			goto bad;
		}

		err = usbd_device2interface_handle(dev, UFTDI_IFACE_INDEX, &iface);
		if (err) {
			printf("%s: failed to get interface, err=%s\n",
			    sc->sc_dev.dv_xname, usbd_errstr(err));
			goto bad;
		}
	} else
		iface = uaa->iface;

	id = usbd_get_interface_descriptor(iface);

	sc->sc_udev = dev;
	sc->sc_iface = iface;

	if (uaa->release < 0x0200) {
		sc->sc_type = UFTDI_TYPE_SIO;
		sc->sc_hdrlen = 1;
	} else {
		sc->sc_type = UFTDI_TYPE_8U232AM;
		sc->sc_hdrlen = 0;
	}

	uca.bulkin = uca.bulkout = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		int addr, dir, attr;
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: could not read endpoint descriptor\n",
			    devname);
			goto bad;
		}

		addr = ed->bEndpointAddress;
		dir = UE_GET_DIR(ed->bEndpointAddress);
		attr = ed->bmAttributes & UE_XFERTYPE;
		if (dir == UE_DIR_IN && attr == UE_BULK)
			uca.bulkin = addr;
		else if (dir == UE_DIR_OUT && attr == UE_BULK)
			uca.bulkout = addr;
		else {
			printf("%s: unexpected endpoint\n", devname);
			goto bad;
		}
	}
	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
		       sc->sc_dev.dv_xname);
		goto bad;
	}
	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
		       sc->sc_dev.dv_xname);
		goto bad;
	}

	if (uaa->iface == NULL)
		uca.portno = FTDI_PIT_SIOA;
	else
		uca.portno = FTDI_PIT_SIOA + id->bInterfaceNumber;
	/* bulkin, bulkout set above */
	uca.ibufsize = UFTDIIBUFSIZE;
	uca.obufsize = UFTDIOBUFSIZE - sc->sc_hdrlen;
	uca.ibufsizepad = UFTDIIBUFSIZE;
	uca.opkthdrlen = sc->sc_hdrlen;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &uftdi_methods;
	uca.arg = sc;
	uca.info = NULL;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   &sc->sc_dev);

	DPRINTF(("uftdi: in=0x%x out=0x%x\n", uca.bulkin, uca.bulkout));
	sc->sc_subdev = config_found_sm(self, &uca, ucomprint, ucomsubmatch);

	return;

bad:
	DPRINTF(("uftdi_attach: ATTACH ERROR\n"));
	sc->sc_dying = 1;
}

int
uftdi_activate(struct device *self, int act)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_subdev != NULL)
			rv = config_deactivate(sc->sc_subdev);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}

int
uftdi_detach(struct device *self, int flags)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;

	DPRINTF(("uftdi_detach: sc=%p flags=%d\n", sc, flags));
	sc->sc_dying = 1;
	if (sc->sc_subdev != NULL) {
		config_detach(sc->sc_subdev, flags);
		sc->sc_subdev = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   &sc->sc_dev);

	return (0);
}

int
uftdi_open(void *vsc, int portno)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	struct termios t;

	DPRINTF(("uftdi_open: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	/* Perform a full reset on the device */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_RESET;
	USETW(req.wValue, FTDI_SIO_RESET_SIO);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	/* Set 9600 baud, 2 stop bits, no parity, 8 bits */
	t.c_ospeed = 9600;
	t.c_cflag = CSTOPB | CS8;
	(void)uftdi_param(sc, portno, &t);

	/* Turn on RTS/CTS flow control */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW(req.wValue, 0);
	USETW2(req.wIndex, FTDI_SIO_RTS_CTS_HS, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	return (0);
}

void
uftdi_read(void *vsc, int portno, u_char **ptr, u_int32_t *count)
{
	struct uftdi_softc *sc = vsc;
	u_char msr, lsr;

	DPRINTFN(15,("uftdi_read: sc=%p, port=%d count=%d\n", sc, portno,
		     *count));

	msr = FTDI_GET_MSR(*ptr);
	lsr = FTDI_GET_LSR(*ptr);

#ifdef UFTDI_DEBUG
	if (*count != 2)
		DPRINTFN(10,("uftdi_read: sc=%p, port=%d count=%d data[0]="
			    "0x%02x\n", sc, portno, *count, (*ptr)[2]));
#endif

	if (sc->sc_msr != msr ||
	    (sc->sc_lsr & FTDI_LSR_MASK) != (lsr & FTDI_LSR_MASK)) {
		DPRINTF(("uftdi_read: status change msr=0x%02x(0x%02x) "
			 "lsr=0x%02x(0x%02x)\n", msr, sc->sc_msr,
			 lsr, sc->sc_lsr));
		sc->sc_msr = msr;
		sc->sc_lsr = lsr;
		ucom_status_change((struct ucom_softc *)sc->sc_subdev);
	}

	/* Pick up status and adjust data part. */
	*ptr += 2;
	*count -= 2;
}

void
uftdi_write(void *vsc, int portno, u_char *to, u_char *from, u_int32_t *count)
{
	struct uftdi_softc *sc = vsc;

	DPRINTFN(10,("uftdi_write: sc=%p, port=%d count=%u data[0]=0x%02x\n",
		     vsc, portno, *count, from[0]));

	/* Make length tag and copy data */
	if (sc->sc_hdrlen > 0)
		*to = FTDI_OUT_TAG(*count, portno);

	memcpy(to + sc->sc_hdrlen, from, *count);
	*count += sc->sc_hdrlen;
}

void
uftdi_set(void *vsc, int portno, int reg, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int ctl;

	DPRINTF(("uftdi_set: sc=%p, port=%d reg=%d onoff=%d\n", vsc, portno,
		 reg, onoff));

	switch (reg) {
	case UCOM_SET_DTR:
		ctl = onoff ? FTDI_SIO_SET_DTR_HIGH : FTDI_SIO_SET_DTR_LOW;
		break;
	case UCOM_SET_RTS:
		ctl = onoff ? FTDI_SIO_SET_RTS_HIGH : FTDI_SIO_SET_RTS_LOW;
		break;
	case UCOM_SET_BREAK:
		uftdi_break(sc, portno, onoff);
		return;
	default:
		return;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_MODEM_CTRL;
	USETW(req.wValue, ctl);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_set: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}

int
uftdi_param(void *vsc, int portno, struct termios *t)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	int rate, data, flow;

	DPRINTF(("uftdi_param: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	switch (sc->sc_type) {
	case UFTDI_TYPE_SIO:
		switch (t->c_ospeed) {
		case 300: rate = ftdi_sio_b300; break;
		case 600: rate = ftdi_sio_b600; break;
		case 1200: rate = ftdi_sio_b1200; break;
		case 2400: rate = ftdi_sio_b2400; break;
		case 4800: rate = ftdi_sio_b4800; break;
		case 9600: rate = ftdi_sio_b9600; break;
		case 19200: rate = ftdi_sio_b19200; break;
		case 38400: rate = ftdi_sio_b38400; break;
		case 57600: rate = ftdi_sio_b57600; break;
		case 115200: rate = ftdi_sio_b115200; break;
		default:
			return (EINVAL);
		}
		break;

	case UFTDI_TYPE_8U232AM:
		if (uftdi_8u232am_getrate(t->c_ospeed, &rate) == -1)
			return (EINVAL);
		break;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_BAUD_RATE;
	USETW(req.wValue, rate);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	if (ISSET(t->c_cflag, CSTOPB))
		data = FTDI_SIO_SET_DATA_STOP_BITS_2;
	else
		data = FTDI_SIO_SET_DATA_STOP_BITS_1;
	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			data |= FTDI_SIO_SET_DATA_PARITY_ODD;
		else
			data |= FTDI_SIO_SET_DATA_PARITY_EVEN;
	} else
		data |= FTDI_SIO_SET_DATA_PARITY_NONE;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		data |= FTDI_SIO_SET_DATA_BITS(5);
		break;
	case CS6:
		data |= FTDI_SIO_SET_DATA_BITS(6);
		break;
	case CS7:
		data |= FTDI_SIO_SET_DATA_BITS(7);
		break;
	case CS8:
		data |= FTDI_SIO_SET_DATA_BITS(8);
		break;
	}
	sc->last_lcr = data;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	if (ISSET(t->c_cflag, CRTSCTS)) {
		flow = FTDI_SIO_RTS_CTS_HS;
		USETW(req.wValue, 0);
	} else if (ISSET(t->c_iflag, IXON|IXOFF)) {
		flow = FTDI_SIO_XON_XOFF_HS;
		USETW2(req.wValue, t->c_cc[VSTOP], t->c_cc[VSTART]);
	} else {
		flow = FTDI_SIO_DISABLE_FLOW_CTRL;
		USETW(req.wValue, 0);
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW2(req.wIndex, flow, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	return (0);
}

void
uftdi_get_status(void *vsc, int portno, u_char *lsr, u_char *msr)
{
	struct uftdi_softc *sc = vsc;

	DPRINTF(("uftdi_status: msr=0x%02x lsr=0x%02x\n",
		 sc->sc_msr, sc->sc_lsr));

	if (msr != NULL)
		*msr = sc->sc_msr;
	if (lsr != NULL)
		*lsr = sc->sc_lsr;
}

void
uftdi_break(void *vsc, int portno, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int data;

	DPRINTF(("uftdi_break: sc=%p, port=%d onoff=%d\n", vsc, portno,
		  onoff));

	if (onoff) {
		data = sc->last_lcr | FTDI_SIO_SET_BREAK;
	} else {
		data = sc->last_lcr;
	}

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}

int
uftdi_8u232am_getrate(speed_t speed, int *rate)
{
	/* Table of the nearest even powers-of-2 for values 0..15. */
	static const unsigned char roundoff[16] = {
		0, 2, 2, 4,  4,  4,  8,  8,
		8, 8, 8, 8, 16, 16, 16, 16,
	};

	unsigned int d, freq;
	int result;

	if (speed <= 0)
		return (-1);

	/* Special cases for 2M and 3M. */
	if (speed >= 3000000 * 100 / 103 &&
	    speed <= 3000000 * 100 / 97) {
		result = 0;
		goto done;
	}
	if (speed >= 2000000 * 100 / 103 &&
	    speed <= 2000000 * 100 / 97) {
		result = 1;
		goto done;
	}

	d = (FTDI_8U232AM_FREQ << 4) / speed;
	d = (d & ~15) + roundoff[d & 15];

	if (d < FTDI_8U232AM_MIN_DIV)
		d = FTDI_8U232AM_MIN_DIV;
	else if (d > FTDI_8U232AM_MAX_DIV)
		d = FTDI_8U232AM_MAX_DIV;

	/* 
	 * Calculate the frequency needed for d to exactly divide down
	 * to our target speed, and check that the actual frequency is
	 * within 3% of this.
	 */
	freq = speed * d;
	if (freq < (quad_t)(FTDI_8U232AM_FREQ << 4) * 100 / 103 ||
	    freq > (quad_t)(FTDI_8U232AM_FREQ << 4) * 100 / 97)
		return (-1);

	/* 
	 * Pack the divisor into the resultant value.  The lower
	 * 14-bits hold the integral part, while the upper 2 bits
	 * encode the fractional component: either 0, 0.5, 0.25, or
	 * 0.125.
	 */
	result = d >> 4;
	if (d & 8)
		result |= 0x4000;
	else if (d & 4)
		result |= 0x8000;
	else if (d & 2)
		result |= 0xc000;

done:
	*rate = result;
	return (0);
}
