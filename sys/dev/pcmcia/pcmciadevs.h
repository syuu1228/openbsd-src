/*	$OpenBSD: src/sys/dev/pcmcia/pcmciadevs.h,v 1.65 2001/05/22 08:31:11 fgsch Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *		OpenBSD: pcmciadevs,v 1.60 2001/05/22 08:30:27 fgsch Exp 
 */
/* $NetBSD: pcmciadevs,v 1.13 1998/08/17 23:10:12 thorpej Exp $ */

/*
 * Copyright (c) 1998, Christos Zoulas
 * All rights reserved.
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
 *      This product includes software developed by Christos Zoulas
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * List of known PCMCIA vendors
 */

#define	PCMCIA_VENDOR_FUJITSU	0x0004	/* Fujitsu */
#define	PCMCIA_VENDOR_PANASONIC	0x0032	/* Matsushita Electric Industrial Co. */
#define	PCMCIA_VENDOR_SANDISK	0x0045	/* Sandisk */
#define	PCMCIA_VENDOR_NEWMEDIA	0x0057	/* NewMedia */
#define	PCMCIA_VENDOR_INTEL	0x0089	/* Intel */
#define	PCMCIA_VENDOR_IBM	0x00a4	/* IBM */
#define	PCMCIA_VENDOR_3COM	0x0101	/* 3Com */
#define	PCMCIA_VENDOR_MEGAHERTZ	0x0102	/* Megahertz */
#define	PCMCIA_VENDOR_SOCKET	0x0104	/* Socket Communications */
#define	PCMCIA_VENDOR_TDK	0x0105	/* TDK */
#define	PCMCIA_VENDOR_XIRCOM	0x0105	/* Xircom */
#define	PCMCIA_VENDOR_SMC	0x0108	/* SMC */
#define	PCMCIA_VENDOR_MOTOROLA	0x0109	/* Motorola */
#define	PCMCIA_VENDOR_USROBOTICS	0x0115	/* US Robotics */
#define	PCMCIA_VENDOR_PROXIM	0x0126	/* Proxim */
#define	PCMCIA_VENDOR_MEGAHERTZ2	0x0128	/* Megahertz */
#define	PCMCIA_VENDOR_ADAPTEC	0x012f	/* Adaptec */
#define	PCMCIA_VENDOR_QUATECH	0x0137	/* Quatech */
#define	PCMCIA_VENDOR_COMPAQ	0x0138	/* Compaq */
#define	PCMCIA_VENDOR_GREYCELL	0x0143	/* Grey Cell */
#define	PCMCIA_VENDOR_LINKSYS	0x0149	/* Linksys */
#define	PCMCIA_VENDOR_SIMPLETECH	0x014d	/* Simple Technology */
#define	PCMCIA_VENDOR_SYMBOL	0x014d	/* Symbol */
#define	PCMCIA_VENDOR_LUCENT	0x0156	/* Lucent */
#define	PCMCIA_VENDOR_AIRONET	0x015f	/* Aironet Wireless Communications */
#define	PCMCIA_VENDOR_PSION	0x016c	/* Psion */
#define	PCMCIA_VENDOR_COMPAQ2	0x0183	/* Compaq */
#define	PCMCIA_VENDOR_KINGSTON	0x0186	/* Kingston */
#define	PCMCIA_VENDOR_DAYNA	0x0194	/* Dayna */
#define	PCMCIA_VENDOR_RAYTHEON	0x01a6	/* Raytheon */
#define	PCMCIA_VENDOR_BAY	0x01eb	/* Bay Networks */
#define	PCMCIA_VENDOR_IODATA	0x01bf	/* I-O DATA */
#define	PCMCIA_VENDOR_FARALLON	0x0200	/* Farallon Communications */
#define	PCMCIA_VENDOR_NOKIA	0x023d	/* Nokia Communications */
#define	PCMCIA_VENDOR_SAMSUNG	0x0250	/* Samsung */
#define	PCMCIA_VENDOR_IODATA2	0x028a	/* I-O DATA */
#define	PCMCIA_VENDOR_BREEZECOM	0x0a02	/* BreezeCOM */
#define	PCMCIA_VENDOR_NEWMEDIA2	0x10cd	/* NewMedia */
#define	PCMCIA_VENDOR_LASAT	0x3401	/* Lasat */
#define	PCMCIA_VENDOR_LEXARMEDIA	0x4e01	/* Lexar Media */
#define	PCMCIA_VENDOR_DUAL	0x890f	/* Dual */
#define	PCMCIA_VENDOR_COMPEX	0x8a01	/* Compex */
#define	PCMCIA_VENDOR_MELCO	0x8a01	/* Melco Corporation */
#define	PCMCIA_VENDOR_CONTEC	0xc001	/* Contec */
#define	PCMCIA_VENDOR_COREGA	0xc00f	/* Corega K.K. */
#define	PCMCIA_VENDOR_ALLIEDTELESIS	0xc00f	/* Allied Telesis K.K. */
#define	PCMCIA_VENDOR_HAGIWARASYSCOM	0xc012	/* Hagiwara SYS-COM */
#define	PCMCIA_VENDOR_RATOC	0xc015	/* RATOC System Inc. */
#define	PCMCIA_VENDOR_ELSA	0xd601	/* Elsa */

/*
 * List of known products.  Grouped by vendor.
 */
/* 3COM */
#define	PCMCIA_CIS_3COM_3CRWE737A	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3CRWE737A	0x0001
#define	PCMCIA_CIS_3COM_3CXM056BNW	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3CXM056BNW	0x002f
#define	PCMCIA_CIS_3COM_3CXEM556B	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3CXEM556B	0x003d
#define	PCMCIA_CIS_3COM_3CXEM556	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3CXEM556	0x0035
#define	PCMCIA_CIS_3COM_3CCFEM556BI	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3CCFEM556BI	0x0556
#define	PCMCIA_CIS_3COM_3C562	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3C562	0x0562
#define	PCMCIA_CIS_3COM_3C589	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3C589	0x0589
#define	PCMCIA_CIS_3COM_3C574	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3C574	0x0574
#define	PCMCIA_CIS_3COM_3C1	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_3COM_3C1	0x0cf1

/* Adaptec */
#define	PCMCIA_CIS_ADAPTEC_APA1460_1	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_ADAPTEC_APA1460_1	0x0001
#define	PCMCIA_CIS_ADAPTEC_APA1460_2	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_ADAPTEC_APA1460_2	0x0002

/* Aironet */
#define	PCMCIA_CIS_AIRONET_PC4500	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_AIRONET_PC4500	0x0005
#define	PCMCIA_CIS_AIRONET_PC4800	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_AIRONET_PC4800	0x0007
#define	PCMCIA_CIS_AIRONET_350	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_AIRONET_350	0x000a

/* Allied Telesis K.K. */
#define	PCMCIA_CIS_ALLIEDTELESIS_LA_PCM	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_ALLIEDTELESIS_LA_PCM	0x0002

/* Bay Networks */
#define	PCMCIA_CIS_BAY_STACK_650	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_BAY_STACK_650	0x0804
#define	PCMCIA_CIS_BAY_SURFER_PRO	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_BAY_SURFER_PRO	0x0806
#define	PCMCIA_CIS_BAY_STACK_660	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_BAY_STACK_660	0x0807

/* BreezeCOM */
#define	PCMCIA_CIS_BREEZECOM_BREEZENET	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_BREEZECOM_BREEZENET	0x0102

/* Contec C-NET(PC) */
#define	PCMCIA_CIS_CONTEC_CNETPC	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_CONTEC_CNETPC	0x0000
#define	PCMCIA_CIS_CONTEC_FX_DS110_PCC	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_CONTEC_FX_DS110_PCC	0x0008

/* Compaq Products */
#define	PCMCIA_CIS_COMPAQ_NC5004	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_COMPAQ_NC5004	0x0002

/* Compex */
#define	PCMCIA_CIS_COMPEX_AMP_WIRELESS	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_COMPEX_AMP_WIRELESS	0x0066
#define	PCMCIA_CIS_COMPEX_LINKPORT_ENET_B	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_COMPEX_LINKPORT_ENET_B	0x0100

/* Dayna */
#define	PCMCIA_CIS_DAYNA_COMMUNICARD_E_1	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_DAYNA_COMMUNICARD_E_1	0x002d
#define	PCMCIA_CIS_DAYNA_COMMUNICARD_E_2	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_DAYNA_COMMUNICARD_E_2	0x002f

/* Digital */
#define	PCMCIA_CIS_DIGITAL_MOBILE_MEDIA_CDROM	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_DIGITAL_MOBILE_MEDIA_CDROM	0x0d00

/* Dual */
#define	PCMCIA_CIS_DUAL_NE2000	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_DUAL_NE2000	0x0100

/* ELSA Products */
#define	PCMCIA_CIS_ELSA_MC2_IEEE	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_ELSA_MC2_IEEE	0x0001
#define	PCMCIA_CIS_ELSA_XI300_IEEE	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_ELSA_XI300_IEEE	0x0002

/* Farallon Communications */
#define	PCMCIA_CIS_FARALLON_SKYLINE	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_FARALLON_SKYLINE	0x0a01

/* Fujitsu */
#define	PCMCIA_CIS_FUJITSU_SCSI600	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_FUJITSU_SCSI600	0x0401
#define	PCMCIA_CIS_FUJITSU_LA10S	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_FUJITSU_LA10S	0x1003
#define	PCMCIA_CIS_FUJITSU_LA501	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_FUJITSU_LA501	0x2000
 
/* Greycell */
#define	PCMCIA_CIS_GREYCELL_GCS2000	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_GREYCELL_GCS2000	0x0201

/* IBM */
#define	PCMCIA_CIS_IBM_3270	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_3270	0x0001
#define	PCMCIA_CIS_IBM_INFOMOVER	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_INFOMOVER	0x0002
#define	PCMCIA_CIS_IBM_5250	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_5250	0x000b
#define	PCMCIA_CIS_IBM_TROPIC	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_TROPIC	0x001e
#define	PCMCIA_CIS_IBM_PORTABLE_CDROM	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_PORTABLE_CDROM	0x002d
#define	PCMCIA_CIS_IBM_HOME_AND_AWAY	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_HOME_AND_AWAY	0x002e
#define	PCMCIA_CIS_IBM_WIRELESS_LAN_ENTRY	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_WIRELESS_LAN_ENTRY	0x0032
#define	PCMCIA_CIS_IBM_ETHERJET	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IBM_ETHERJET	0x003f

/* I-O DATA */
#define	PCMCIA_CIS_IODATA_PCLATE	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IODATA_PCLATE	0x2216
#define	PCMCIA_CIS_IODATA2_WNB11PCM	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_IODATA2_WNB11PCM	0x0002

/* Intel */
#define	PCMCIA_CIS_INTEL_EEPRO100	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_INTEL_EEPRO100	0x010a

/* Kingston */
#define	PCMCIA_CIS_KINGSTON_KNE_PC2	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_KINGSTON_KNE_PC2	0x0100

/* Lasat */
#define	PCMCIA_CIS_LASAT_CREDIT_288	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LASAT_CREDIT_288	0x2811

/* Lexar Media */
#define	PCMCIA_CIS_LEXARMEDIA_COMPATFLASH	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LEXARMEDIA_COMPATFLASH	0x0100

/* Linksys corporation */
#define	PCMCIA_CIS_LINKSYS_TRUST_COMBO_ECARD	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LINKSYS_TRUST_COMBO_ECARD	0x021b
#define	PCMCIA_CIS_LINKSYS_ETHERFAST	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LINKSYS_ETHERFAST	0x0230
#define	PCMCIA_CIS_LINKSYS_ECARD_1	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LINKSYS_ECARD_1	0x0265
#define	PCMCIA_CIS_LINKSYS_COMBO_ECARD	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LINKSYS_COMBO_ECARD	0xc1ab

/* Lucent WaveLAN/IEEE */
#define	PCMCIA_CIS_LUCENT_WAVELAN_IEEE	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE	0x0002

/* Megahertz */
#define	PCMCIA_CIS_MEGAHERTZ_XJEM3336	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ_XJEM3336	0x0006
#define	PCMCIA_CIS_MEGAHERTZ_XJ4288	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ_XJ4288	0x0023
#define	PCMCIA_CIS_MEGAHERTZ_XJ4336	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ_XJ4336	0x0027
#define	PCMCIA_CIS_MEGAHERTZ_XJ5560	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ_XJ5560	0x0034
#define	PCMCIA_CIS_MEGAHERTZ2_XJEM1144	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ2_XJEM1144	0x0101
#define	PCMCIA_CIS_MEGAHERTZ2_XJACK	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ2_XJACK	0x0103

/* Melco */
#define	PCMCIA_CIS_MELCO_LPC3_TX	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MELCO_LPC3_TX	0xc1ab

/* Motorola */
#define	PCMCIA_CIS_MOTOROLA_POWER144	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MOTOROLA_POWER144	0x0105
#define	PCMCIA_CIS_MOTOROLA_PM100C	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MOTOROLA_PM100C	0x0302
#define	PCMCIA_CIS_MOTOROLA_MONTANA_336	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_MOTOROLA_MONTANA_336	0x0505

/* NewMedia */
#define	PCMCIA_CIS_NEWMEDIA_BASICS	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA_BASICS	0x0019
#define	PCMCIA_CIS_NEWMEDIA_LANSURFER	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA_LANSURFER	0x0021
#define	PCMCIA_CIS_NEWMEDIA_LIVEWIRE	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA_LIVEWIRE	0x1004
#define	PCMCIA_CIS_NEWMEDIA_MULTIMEDIA	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA_MULTIMEDIA	0x100b
#define	PCMCIA_CIS_NEWMEDIA_BUSTOASTER	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA_BUSTOASTER	0xc102
#define	PCMCIA_CIS_NEWMEDIA_WAVJAMMER	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA_WAVJAMMER	0xe005
#define	PCMCIA_CIS_NEWMEDIA2_BUSTOASTER	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NEWMEDIA2_BUSTOASTER	0x0001

/* Nokia Products */
#define	PCMCIA_CIS_NOKIA_C020_WLAN	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_NOKIA_C020_WLAN	0x20c0

/* Panasonic */
#define	PCMCIA_CIS_PANASONIC_KXLC002	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PANASONIC_KXLC002	0x0304
#define	PCMCIA_CIS_PANASONIC_KXLC003	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PANASONIC_KXLC003	0x0504
#define	PCMCIA_CIS_PANASONIC_KME	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PANASONIC_KME	0x2604

/* Proxim */
#define	PCMCIA_CIS_PROXIM_ROAMABOUT_2400FH	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PROXIM_ROAMABOUT_2400FH	0x1058
#define	PCMCIA_CIS_PROXIM_RANGELAN2_7401	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PROXIM_RANGELAN2_7401	0x1158

/* Psion */
#define	PCMCIA_CIS_PSION_GOLDCARD	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PSION_GOLDCARD	0x0020

/* Quatech */
#define	PCMCIA_CIS_QUATECH_DSP_225	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_QUATECH_DSP_225	0x0008

/* Raylink/WebGear */
#define	PCMCIA_CIS_RAYTHEON_WLAN	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_RAYTHEON_WLAN	0x0000

/* RATOC System Inc. */
#define	PCMCIA_CIS_RATOC_REX_R280	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_RATOC_REX_R280	0x0001

/* Samsung */
#define	PCMCIA_CIS_SAMSUNG_SWL_2000N	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SAMSUNG_SWL_2000N	0x0002

/* Sandisk */
#define	PCMCIA_CIS_SANDISK_SDCFB	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SANDISK_SDCFB	0x0401

/* Simple Technology */
#define	PCMCIA_CIS_SIMPLETECH_COMMUNICATOR288	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SIMPLETECH_COMMUNICATOR288	0x0100

/* Socket Communications */
#define	PCMCIA_CIS_SOCKET_PAGECARD	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SOCKET_PAGECARD	0x0003
#define	PCMCIA_CIS_SOCKET_DUAL_RS232	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SOCKET_DUAL_RS232	0x0006
#define	PCMCIA_CIS_SOCKET_LP_ETHER_CF	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SOCKET_LP_ETHER_CF	0x0075

/* Standard Microsystems Corporation */
#define	PCMCIA_CIS_SMC_8020	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SMC_8020	0x0001
#define	PCMCIA_CIS_SMC_8016	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SMC_8016	0x0105
#define	PCMCIA_CIS_SMC_EZCARD	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SMC_EZCARD	0x8022

/* Symbol */
#define	PCMCIA_CIS_SYMBOL_SPECTRUM24	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SYMBOL_SPECTRUM24	0x0801

/* TDK */
#define	PCMCIA_CIS_TDK_LAK_CD021BX	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_TDK_LAK_CD021BX	0x0200
#define	PCMCIA_CIS_TDK_DFL9610	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_TDK_DFL9610	0x0d0a
#define	PCMCIA_CIS_TDK_LAK_CF010	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_TDK_LAK_CF010	0x0900
#define	PCMCIA_CIS_TDK_LAK_CD011WL	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_TDK_LAK_CD011WL	0x0000

/* US Robotics */
#define	PCMCIA_CIS_USROBOTICS_WORLDPORT144	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_USROBOTICS_WORLDPORT144	0x3330

/* Xircom */
#define	PCMCIA_CIS_XIRCOM_XIR_CE_10	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_CE_10	0x0108
#define	PCMCIA_CIS_XIRCOM_XIR_CE3_10_100	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_CE3_10_100	0x010a
#define	PCMCIA_CIS_XIRCOM_XIR_PS_CE2_10	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_PS_CE2_10	0x010b
#define	PCMCIA_CIS_XIRCOM_XIR_CNW_801	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_CNW_801	0x0801
#define	PCMCIA_CIS_XIRCOM_XIR_CNW_802	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_CNW_802	0x0802
#define	PCMCIA_CIS_XIRCOM_XIR_CEM_10	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_CEM_10	0x110a
#define	PCMCIA_CIS_XIRCOM_XIR_CEM_28	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_XIRCOM_XIR_CEM_28	0x110b

/* Cards we know only by their cis */
#define	PCMCIA_VENDOR_ACCTON	-1	/* ACCTON */
#define	PCMCIA_VENDOR_ADDTRON	-1	/* Addtron */
#define	PCMCIA_VENDOR_AMBICOM	-1	/* AmbiCom */
#define	PCMCIA_VENDOR_AMD	-1	/* AMD */
#define	PCMCIA_VENDOR_BILLIONTON	-1	/* Billionton Systems Inc. */
#define	PCMCIA_VENDOR_CNET	-1	/* CNet */
#define	PCMCIA_VENDOR_DIGITAL	-1	/* Digital */
#define	PCMCIA_VENDOR_DLINK	-1	/* D-Link */
#define	PCMCIA_VENDOR_EDIMAX	-1	/* Edimax */
#define	PCMCIA_VENDOR_EPSON	-1	/* Seiko Epson Corporation */
#define	PCMCIA_VENDOR_EXP	-1	/* EXP Computer Inc */
#define	PCMCIA_VENDOR_ICOM	-1	/* ICOM Inc */
#define	PCMCIA_VENDOR_INTERSIL	-1	/* Intersil */
#define	PCMCIA_VENDOR_PLANET	-1	/* Planet */
#define	PCMCIA_VENDOR_PLANEX	-1	/* Planex */
#define	PCMCIA_VENDOR_PREMAX	-1	/* Premax */
#define	PCMCIA_VENDOR_RPTI	-1	/* RPTI */
#define	PCMCIA_VENDOR_SARAMNCOM	-1	/* SaramNcom Co.,Ltd. */
#define	PCMCIA_VENDOR_SHUTTLE	-1	/* Shuttle */
#define	PCMCIA_VENDOR_SVEC	-1	/* SVEC/Hawking */
#define	PCMCIA_VENDOR_SYNERGY21	-1	/* Synergy 21 */
#define	PCMCIA_VENDOR_TEAC	-1	/* TEAC */
#define	PCMCIA_VENDOR_YEDATA	-1	/* Y-E DATA */
#define	PCMCIA_VENDOR_ZOOM	-1	/* Zoom */

#define	PCMCIA_CIS_MEGAHERTZ_XJ2288	{ "MEGAHERTZ", "MODEM XJ2288", NULL, NULL }
#define	PCMCIA_PRODUCT_MEGAHERTZ_XJ2288	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_PREMAX_PE200	{ "PMX   ", "PE-200", NULL, NULL }
#define	PCMCIA_PRODUCT_PREMAX_PE200	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_PLANET_SMARTCOM2000	{ "PCMCIA", "UE2212", NULL, NULL }
#define	PCMCIA_PRODUCT_PLANET_SMARTCOM2000	PCMCIA_PRODUCT_INVALID
/*
 * vendor ID of both FNW-3600-T and FNW-3700-T is LINKSYS (0x0149) and
 * product ID is 0xc1ab, but it conflicts with LINKSYS Combo EthernetCard.
 */
#define	PCMCIA_CIS_PLANEX_FNW3600T	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PLANEX_FNW3600T	-1
#define	PCMCIA_CIS_PLANEX_FNW3700T	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_PLANEX_FNW3700T	-1
#define	PCMCIA_CIS_DLINK_DE650	{ "D-Link", "DE-650", NULL, NULL }
#define	PCMCIA_PRODUCT_DLINK_DE650	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_DLINK_DE660	{ "D-Link", "DE-660", NULL, NULL }
#define	PCMCIA_PRODUCT_DLINK_DE660	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_RPTI_EP400	{ "RPTI LTD.", "EP400", "CISV100", NULL }
#define	PCMCIA_PRODUCT_RPTI_EP400	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_RPTI_EP401	{ "RPTI", "EP401 Ethernet NE2000 Compatible", NULL, NULL }
#define	PCMCIA_PRODUCT_RPTI_EP401	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_ACCTON_EN2212	{ "ACCTON", "EN2212", NULL, NULL }
#define	PCMCIA_PRODUCT_ACCTON_EN2212	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_YEDATA_EXTERNAL_FDD	{ "Y-E DATA", "External FDD", NULL, NULL }
#define	PCMCIA_PRODUCT_YEDATA_EXTERNAL_FDD	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_DIGITAL_DEPCMXX	{ "DIGITAL", "DEPCM-XX", NULL, NULL }
#define	PCMCIA_PRODUCT_DIGITAL_DEPCMXX	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_TEAC_IDECARDII	{ NULL, "NinjaATA-", NULL, NULL }
#define	PCMCIA_PRODUCT_TEAC_IDECARDII	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_LINKSYS_ECARD_2	{ "LINKSYS", "E-CARD", NULL, NULL }
#define	PCMCIA_PRODUCT_LINKSYS_ECARD_2	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_COREGA_ETHER_PCC_T	{ "corega K.K.", "corega Ether PCC-T", NULL, NULL }
#define	PCMCIA_PRODUCT_COREGA_ETHER_PCC_T	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_COREGA_ETHER_II_PCC_T	{ "corega K.K.", "corega EtherII PCC-T", NULL, NULL }
#define	PCMCIA_PRODUCT_COREGA_ETHER_II_PCC_T	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_COREGA_FAST_ETHER_PCC_TX	{ "corega K.K.", "corega FastEther PCC-TX", NULL, NULL }
#define	PCMCIA_PRODUCT_COREGA_FAST_ETHER_PCC_TX	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_COREGA_FAST_ETHER_PCC_TXF	{ "corega", "FEther PCC-TXF", NULL, NULL }
#define	PCMCIA_PRODUCT_COREGA_FAST_ETHER_PCC_TXF	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_COREGA_WIRELESS_LAN_PCC_11	{ "corega K.K.", "Wireless LAN PCC-11", NULL, NULL }
#define	PCMCIA_PRODUCT_COREGA_WIRELESS_LAN_PCC_11	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_SVEC_COMBOCARD	{ "Ethernet", "Adapter", NULL, NULL }
#define	PCMCIA_PRODUCT_SVEC_COMBOCARD	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_SVEC_LANCARD	{ "SVEC", "FD605 PCMCIA EtherNet Card", "V1-1", NULL }
#define	PCMCIA_PRODUCT_SVEC_LANCARD	PCMCIA_PRODUCT_INVALID
/*
 * vendor ID of PN650TX is LINKSYS (0x0149) and product ID is 0xc1ab, but
 * it conflicts with LINKSYS Combo EthernetCard.
 */
#define	PCMCIA_CIS_SVEC_PN650TX	{ NULL, NULL, NULL, NULL }
#define	PCMCIA_PRODUCT_SVEC_PN650TX	-1
#define	PCMCIA_CIS_AMBICOM_AMB8002T	{ "AmbiCom Inc", "AMB8002T", NULL, NULL }
#define	PCMCIA_PRODUCT_AMBICOM_AMB8002T	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_IODATA_PCLAT	{ "I-O DATA", "PCLA", "ETHERNET", NULL }
#define	PCMCIA_PRODUCT_IODATA_PCLAT	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_EPSON_EEN10B	{ "Seiko Epson Corp.", "Ethernet", "P/N: EEN10B Rev. 00", NULL }
#define	PCMCIA_PRODUCT_EPSON_EEN10B	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_EXP_EXPMULTIMEDIA	{ "EXP   ", "PnPIDE", "F1", NULL }
#define	PCMCIA_PRODUCT_EXP_EXPMULTIMEDIA	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_AMD_AM79C930	{ "AMD", "Am79C930", NULL, NULL }
#define	PCMCIA_PRODUCT_AMD_AM79C930	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_ICOM_SL200	{ "Icom", "SL-200", NULL, NULL }
#define	PCMCIA_PRODUCT_ICOM_SL200	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_XIRCOM_CFE_10	{ "Xircom", "CompactCard Ethernet", "CFE-10", "1.00" }
#define	PCMCIA_PRODUCT_XIRCOM_CFE_10	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_BILLIONTON_LNT10TN	{ "PCMCIA", "LNT-10TN", NULL, NULL }
#define	PCMCIA_PRODUCT_BILLIONTON_LNT10TN	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_NDC_ND5100_E	{ "NDC", "Ethernet", "A", NULL }
#define	PCMCIA_PRODUCT_NDC_ND5100_E	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_PROXIM_RL2_7200	{ "PROXIM", "LAN CARD", "RANGELAN2", NULL }
#define	PCMCIA_PRODUCT_PROXIM_RL2_7200	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_PROXIM_RL2_7400	{ "PROXIM", "LAN PC CARD", "RANGELAN2", NULL }
#define	PCMCIA_PRODUCT_PROXIM_RL2_7400	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_PROXIM_SYMPHONY	{ "PROXIM", "LAN PC CARD", "SYMPHONY", NULL }
#define	PCMCIA_PRODUCT_PROXIM_SYMPHONY	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_EDIMAX_NE2000	{ "PCMCIA", "Ethernet", NULL, NULL }
#define	PCMCIA_PRODUCT_EDIMAX_NE2000	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_CNET_NE2000	{ "CNet", "CN40BC Ethernet", "D", "NE2000" }
#define	PCMCIA_PRODUCT_CNET_NE2000	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_SHUTTLE_IDE_ATAPI	{ "SHUTTLE TECHNOLOGY LTD.", "PCCARD-IDE/ATAPI Adapter", NULL, NULL }
#define	PCMCIA_PRODUCT_SHUTTLE_IDE_ATAPI	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_ZOOM_AIR4000	{ "Zoom", "Air-4000", NULL, NULL }
#define	PCMCIA_PRODUCT_ZOOM_AIR4000	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_TAMARACK_NE2000	{ "TAMARACK", "Ethernet", NULL, NULL }
#define	PCMCIA_PRODUCT_TAMARACK_NE2000	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_SARAMNCOM_NS_1100M	{ "NANOSPEED", "HFA384x/IEEE", NULL, NULL }
#define	PCMCIA_PRODUCT_SARAMNCOM_NS_1100M	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_NETGEAR_FA410TX	{ "NETGEAR", "FA410TX", "Fast Ethernet", NULL} 
#define	PCMCIA_PRODUCT_NETGEAR_FA410TX	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_COREGA_WIRELESS_LAN_PCCA_11	{ "corega K.K.", "Wireless LAN PCCA-11", NULL, NULL }
#define	PCMCIA_PRODUCT_COREGA_WIRELESS_LAN_PCCA_11	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_INTERSIL_PRISM2	{ "INTERSIL", "HFA384x/IEEE", "Version 01.02", NULL }
#define	PCMCIA_PRODUCT_INTERSIL_PRISM2	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_ADDTRON_AWP100	{ "Addtron", "AWP-100 Wireless PCMCIA", "Version 01.02", NULL }
#define	PCMCIA_PRODUCT_ADDTRON_AWP100	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_LINKSYS_WPC11	{ "OEM", "PRISM2 IEEE 802.11 PC-Card", "Version 01.02", NULL }
#define	PCMCIA_PRODUCT_LINKSYS_WPC11	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_NEC_CMZ_RT_WP	{ "NEC", "Wireless Card CMZ-RT-WP", "Version 01.01", NULL }
#define	PCMCIA_PRODUCT_NEC_CMZ_RT_WP	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_NTT_ME_WLAN	{ "NTT-ME", "11Mbps Wireless LAN PC Card", NULL, NULL }
#define	PCMCIA_PRODUCT_NTT_ME_WLAN	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_SMC_2632W	{ "SMC", "SMC2632W", "Version 01.02", NULL }
#define	PCMCIA_PRODUCT_SMC_2632W	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_NANOSPEED_PRISM2	{ "NANOSPEED", "HFA384x/IEEE", "Version 01.02", NULL }
#define	PCMCIA_PRODUCT_NANOSPEED_PRISM2	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_CABLETRON_ROAMABOUT	{ "Cabletron", "RoamAbout 802.11 DS", "Version 01.01", NULL }
#define	PCMCIA_PRODUCT_CABLETRON_ROAMABOUT	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_GVC_NIC2000P	{ "GVC", "NIC-2000p", "ETHERNET", "R01" }
#define	PCMCIA_PRODUCT_GVC_NIC2000P	PCMCIA_PRODUCT_INVALID
#define	PCMCIA_CIS_SYNERGY21_S21810	{ "PCMCIA", "Ethernet", "A", "004743118001" }
#define	PCMCIA_PRODUCT_SYNERGY21_S21810	PCMCIA_PRODUCT_INVALID
