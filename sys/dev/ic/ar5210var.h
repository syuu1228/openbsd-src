/*	$OpenBSD: src/sys/dev/ic/ar5210var.h,v 1.13 2007/03/05 16:54:33 deraadt Exp $	*/

/*
 * Copyright (c) 2004, 2005 Reyk Floeter <reyk@openbsd.org>
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
 * Specific definitions for the Atheros AR5000 Wireless LAN chipset
 * (AR5210 + AR5110).
 */

#ifndef _AR5K_AR5210_VAR_H
#define _AR5K_AR5210_VAR_H

#include <dev/ic/ar5xxx.h>

/*
 * Define a "magic" code for the AR5210 (the HAL layer wants it)
 */

#define AR5K_AR5210_MAGIC		0x0000145a /* 5210 */
#define AR5K_AR5210_TX_NUM_QUEUES	2

#if BYTE_ORDER == BIG_ENDIAN
#define AR5K_AR5210_INIT_CFG	(					\
	AR5K_AR5210_CFG_SWTD | AR5K_AR5210_CFG_SWRD			\
)
#else
#define AR5K_AR5210_INIT_CFG	0x00000000
#endif

/*
 * Internal RX/TX descriptor structures
 * (rX: reserved fields possibily used by future versions of the ar5k chipset)
 */

struct ar5k_ar5210_rx_desc {
	/*
	 * RX control word 0
	 */
	u_int32_t	rx_control_0;

#define AR5K_AR5210_DESC_RX_CTL0			0x00000000

	/*
	 * RX control word 1
	 */
	u_int32_t	rx_control_1;

#define AR5K_AR5210_DESC_RX_CTL1_BUF_LEN		0x00000fff
#define AR5K_AR5210_DESC_RX_CTL1_INTREQ			0x00002000
} __packed;

struct ar5k_ar5210_rx_status {
	/*
	 * RX status word 0
	 */
	u_int32_t	rx_status_0;

#define AR5K_AR5210_DESC_RX_STATUS0_DATA_LEN		0x00000fff
#define AR5K_AR5210_DESC_RX_STATUS0_MORE		0x00001000
#define AR5K_AR5210_DESC_RX_STATUS0_RECEIVE_ANTENNA	0x00004000
#define AR5K_AR5210_DESC_RX_STATUS0_RECEIVE_RATE	0x00078000
#define AR5K_AR5210_DESC_RX_STATUS0_RECEIVE_RATE_S	15
#define AR5K_AR5210_DESC_RX_STATUS0_RECEIVE_SIGNAL	0x07f80000
#define AR5K_AR5210_DESC_RX_STATUS0_RECEIVE_SIGNAL_S	19

	/*
	 * RX status word 1
	 */
	u_int32_t	rx_status_1;

#define AR5K_AR5210_DESC_RX_STATUS1_DONE		0x00000001
#define AR5K_AR5210_DESC_RX_STATUS1_FRAME_RECEIVE_OK	0x00000002
#define AR5K_AR5210_DESC_RX_STATUS1_CRC_ERROR		0x00000004
#define AR5K_AR5210_DESC_RX_STATUS1_FIFO_OVERRUN	0x00000008
#define AR5K_AR5210_DESC_RX_STATUS1_DECRYPT_CRC_ERROR	0x00000010
#define AR5K_AR5210_DESC_RX_STATUS1_PHY_ERROR		0x000000e0
#define AR5K_AR5210_DESC_RX_STATUS1_PHY_ERROR_S		5
#define AR5K_AR5210_DESC_RX_STATUS1_KEY_INDEX_VALID	0x00000100
#define AR5K_AR5210_DESC_RX_STATUS1_KEY_INDEX		0x00007e00
#define AR5K_AR5210_DESC_RX_STATUS1_KEY_INDEX_S		9
#define AR5K_AR5210_DESC_RX_STATUS1_RECEIVE_TIMESTAMP	0x0fff8000
#define AR5K_AR5210_DESC_RX_STATUS1_RECEIVE_TIMESTAMP_S	15
#define AR5K_AR5210_DESC_RX_STATUS1_KEY_CACHE_MISS	0x10000000
} __packed;

#define AR5K_AR5210_DESC_RX_PHY_ERROR_NONE		0x00
#define AR5K_AR5210_DESC_RX_PHY_ERROR_TIMING		0x20
#define AR5K_AR5210_DESC_RX_PHY_ERROR_PARITY		0x40
#define AR5K_AR5210_DESC_RX_PHY_ERROR_RATE		0x60
#define AR5K_AR5210_DESC_RX_PHY_ERROR_LENGTH		0x80
#define AR5K_AR5210_DESC_RX_PHY_ERROR_64QAM		0xa0
#define AR5K_AR5210_DESC_RX_PHY_ERROR_SERVICE		0xc0
#define AR5K_AR5210_DESC_RX_PHY_ERROR_TRANSMITOVR	0xe0

struct ar5k_ar5210_tx_desc {
	/*
	 * TX control word 0
	 */
	u_int32_t	tx_control_0;

#define AR5K_AR5210_DESC_TX_CTL0_FRAME_LEN		0x00000fff
#define AR5K_AR5210_DESC_TX_CTL0_HEADER_LEN		0x0003f000
#define AR5K_AR5210_DESC_TX_CTL0_HEADER_LEN_S		12
#define AR5K_AR5210_DESC_TX_CTL0_XMIT_RATE		0x003c0000
#define AR5K_AR5210_DESC_TX_CTL0_XMIT_RATE_S		18
#define AR5K_AR5210_DESC_TX_CTL0_RTSENA			0x00400000
#define AR5K_AR5210_DESC_TX_CTL0_LONG_PACKET		0x00800000
#define AR5K_AR5210_DESC_TX_CTL0_CLRDMASK		0x01000000
#define AR5K_AR5210_DESC_TX_CTL0_ANT_MODE_XMIT		0x02000000
#define AR5K_AR5210_DESC_TX_CTL0_FRAME_TYPE		0x1c000000
#define AR5K_AR5210_DESC_TX_CTL0_FRAME_TYPE_S		26
#define AR5K_AR5210_DESC_TX_CTL0_INTREQ			0x20000000
#define AR5K_AR5210_DESC_TX_CTL0_ENCRYPT_KEY_VALID	0x40000000

	/*
	 * TX control word 1
	 */
	u_int32_t	tx_control_1;

#define AR5K_AR5210_DESC_TX_CTL1_BUF_LEN		0x00000fff
#define AR5K_AR5210_DESC_TX_CTL1_MORE			0x00001000
#define AR5K_AR5210_DESC_TX_CTL1_ENCRYPT_KEY_INDEX	0x0007e000
#define AR5K_AR5210_DESC_TX_CTL1_ENCRYPT_KEY_INDEX_S	13
#define AR5K_AR5210_DESC_TX_CTL1_RTS_DURATION		0xfff80000
} __packed;

#define AR5K_AR5210_DESC_TX_FRAME_TYPE_NORMAL   0x00
#define AR5K_AR5210_DESC_TX_FRAME_TYPE_ATIM     0x04
#define AR5K_AR5210_DESC_TX_FRAME_TYPE_PSPOLL   0x08
#define AR5K_AR5210_DESC_TX_FRAME_TYPE_NO_DELAY 0x0c
#define AR5K_AR5210_DESC_TX_FRAME_TYPE_PIFS     0x10

struct ar5k_ar5210_tx_status {
	/*
	 * TX status word 0
	 */
	u_int32_t	tx_status_0;

#define AR5K_AR5210_DESC_TX_STATUS0_FRAME_XMIT_OK	0x00000001
#define AR5K_AR5210_DESC_TX_STATUS0_EXCESSIVE_RETRIES	0x00000002
#define AR5K_AR5210_DESC_TX_STATUS0_FIFO_UNDERRUN	0x00000004
#define AR5K_AR5210_DESC_TX_STATUS0_FILTERED		0x00000008
#define AR5K_AR5210_DESC_TX_STATUS0_SHORT_RETRY_COUNT	0x000000f0
#define AR5K_AR5210_DESC_TX_STATUS0_SHORT_RETRY_COUNT_S	4
#define AR5K_AR5210_DESC_TX_STATUS0_LONG_RETRY_COUNT	0x00000f00
#define AR5K_AR5210_DESC_TX_STATUS0_LONG_RETRY_COUNT_S	8
#define AR5K_AR5210_DESC_TX_STATUS0_SEND_TIMESTAMP	0xffff0000
#define AR5K_AR5210_DESC_TX_STATUS0_SEND_TIMESTAMP_S	16

	/*
	 * TX status word 1
	 */
	u_int32_t	tx_status_1;

#define AR5K_AR5210_DESC_TX_STATUS1_DONE		0x00000001
#define AR5K_AR5210_DESC_TX_STATUS1_SEQ_NUM		0x00001ffe
#define AR5K_AR5210_DESC_TX_STATUS1_SEQ_NUM_S		1
#define AR5K_AR5210_DESC_TX_STATUS1_ACK_SIG_STRENGTH	0x001fe000
#define AR5K_AR5210_DESC_TX_STATUS1_ACK_SIG_STRENGTH_S	13
} __packed;

/*
 * Public function prototypes
 */
extern ar5k_attach_t ar5k_ar5210_attach;

/*
 * Initial mode settings ("Base Mode" or "Turbo Mode")
 */

#define AR5K_AR5210_INI_MODE(_aifs) {					\
	{ AR5K_AR5210_SLOT_TIME,					\
	    AR5K_INIT_SLOT_TIME,					\
	    AR5K_INIT_SLOT_TIME_TURBO },				\
	{ AR5K_AR5210_SLOT_TIME,					\
	    AR5K_INIT_ACK_CTS_TIMEOUT,					\
	    AR5K_INIT_ACK_CTS_TIMEOUT_TURBO },				\
	{ AR5K_AR5210_USEC,						\
	    AR5K_INIT_TRANSMIT_LATENCY,					\
	    AR5K_INIT_TRANSMIT_LATENCY_TURBO},				\
	{ AR5K_AR5210_IFS0,						\
	    ((AR5K_INIT_SIFS + (_aifs) * AR5K_INIT_SLOT_TIME)		\
	    << AR5K_AR5210_IFS0_DIFS_S) | AR5K_INIT_SIFS,		\
	    ((AR5K_INIT_SIFS_TURBO + (_aifs) * AR5K_INIT_SLOT_TIME_TURBO) \
	    << AR5K_AR5210_IFS0_DIFS_S) | AR5K_INIT_SIFS_TURBO },	\
	{ AR5K_AR5210_IFS1,						\
	    AR5K_INIT_PROTO_TIME_CNTRL,					\
	    AR5K_INIT_PROTO_TIME_CNTRL_TURBO },				\
	{ AR5K_AR5210_PHY(17),						\
	    (AR5K_REG_READ(AR5K_AR5210_PHY(17)) & ~0x7F) | 0x1C,	\
	    (AR5K_REG_READ(AR5K_AR5210_PHY(17)) & ~0x7F) | 0x38 },	\
	{ AR5K_AR5210_PHY_FC,						\
	    AR5K_AR5210_PHY_FC_SERVICE_ERR |				\
	    AR5K_AR5210_PHY_FC_TXURN_ERR |				\
	    AR5K_AR5210_PHY_FC_ILLLEN_ERR |				\
	    AR5K_AR5210_PHY_FC_ILLRATE_ERR |				\
	    AR5K_AR5210_PHY_FC_PARITY_ERR |				\
	    AR5K_AR5210_PHY_FC_TIMING_ERR | 0x1020,			\
	    AR5K_AR5210_PHY_FC_SERVICE_ERR |				\
	    AR5K_AR5210_PHY_FC_TXURN_ERR |				\
	    AR5K_AR5210_PHY_FC_ILLLEN_ERR |				\
	    AR5K_AR5210_PHY_FC_ILLRATE_ERR |				\
	    AR5K_AR5210_PHY_FC_PARITY_ERR |				\
	    AR5K_AR5210_PHY_FC_TURBO_MODE |				\
	    AR5K_AR5210_PHY_FC_TURBO_SHORT |				\
	    AR5K_AR5210_PHY_FC_TIMING_ERR | 0x2020 },			\
}

/*
 * Initial register values which have to be loaded into the
 * card at boot time and after each reset.
 */

#define AR5K_AR5210_INI {						\
	/* PCU and MAC registers */					\
	{ AR5K_AR5210_TXDP0, 0 },					\
	{ AR5K_AR5210_TXDP1, 0 },					\
	{ AR5K_AR5210_RXDP, 0 },					\
	{ AR5K_AR5210_CR, 0 },						\
	{ AR5K_AR5210_ISR, 0, AR5K_INI_READ },				\
	{ AR5K_AR5210_IMR, 0 },						\
	{ AR5K_AR5210_IER, AR5K_AR5210_IER_DISABLE },			\
	{ AR5K_AR5210_BSR, 0, AR5K_INI_READ },				\
	{ AR5K_AR5210_TXCFG, AR5K_AR5210_DMASIZE_128B },		\
	{ AR5K_AR5210_RXCFG, AR5K_AR5210_DMASIZE_128B },		\
	{ AR5K_AR5210_CFG, AR5K_AR5210_INIT_CFG },			\
	{ AR5K_AR5210_TOPS, AR5K_INIT_TOPS },				\
	{ AR5K_AR5210_RXNOFRM, AR5K_INIT_RXNOFRM },			\
	{ AR5K_AR5210_RPGTO, AR5K_INIT_RPGTO },				\
	{ AR5K_AR5210_TXNOFRM, AR5K_INIT_TXNOFRM },			\
	{ AR5K_AR5210_SFR, 0 },						\
	{ AR5K_AR5210_MIBC, 0 },					\
	{ AR5K_AR5210_MISC, 0 },					\
	{ AR5K_AR5210_RX_FILTER, 0 },					\
	{ AR5K_AR5210_MCAST_FIL0, 0 },					\
	{ AR5K_AR5210_MCAST_FIL1, 0 },					\
	{ AR5K_AR5210_TX_MASK0, 0 },					\
	{ AR5K_AR5210_TX_MASK1, 0 },					\
	{ AR5K_AR5210_CLR_TMASK, 0 },					\
	{ AR5K_AR5210_TRIG_LVL, AR5K_TUNE_MIN_TX_FIFO_THRES },		\
	{ AR5K_AR5210_DIAG_SW, 0 },					\
	{ AR5K_AR5210_RSSI_THR, AR5K_TUNE_RSSI_THRES },			\
	{ AR5K_AR5210_TSF_L32, 0 },					\
	{ AR5K_AR5210_TIMER0, 0 },					\
	{ AR5K_AR5210_TIMER1, 0xffffffff },				\
	{ AR5K_AR5210_TIMER2, 0xffffffff },				\
	{ AR5K_AR5210_TIMER3, 1 },					\
	{ AR5K_AR5210_CFP_DUR, 0 },					\
	{ AR5K_AR5210_CFP_PERIOD, 0 },					\
	/* PHY registers */						\
	{ AR5K_AR5210_PHY(0), 0x00000047 },				\
	{ AR5K_AR5210_PHY_AGC, 0x00000000 },				\
	{ AR5K_AR5210_PHY(3), 0x09848ea6 },				\
	{ AR5K_AR5210_PHY(4), 0x3d32e000 },				\
	{ AR5K_AR5210_PHY(5), 0x0000076b },				\
	{ AR5K_AR5210_PHY_ACTIVE, AR5K_AR5210_PHY_DISABLE },		\
	{ AR5K_AR5210_PHY(8), 0x02020200 },				\
	{ AR5K_AR5210_PHY(9), 0x00000e0e },				\
	{ AR5K_AR5210_PHY(10), 0x0a020201 },				\
	{ AR5K_AR5210_PHY(11), 0x00036ffc },				\
	{ AR5K_AR5210_PHY(12), 0x00000000 },				\
	{ AR5K_AR5210_PHY(13), 0x00000e0e },				\
	{ AR5K_AR5210_PHY(14), 0x00000007 },				\
	{ AR5K_AR5210_PHY(15), 0x00020100 },				\
	{ AR5K_AR5210_PHY(16), 0x89630000 },				\
	{ AR5K_AR5210_PHY(17), 0x1372169c },				\
	{ AR5K_AR5210_PHY(18), 0x0018b633 },				\
	{ AR5K_AR5210_PHY(19), 0x1284613c },				\
	{ AR5K_AR5210_PHY(20), 0x0de8b8e0 },				\
	{ AR5K_AR5210_PHY(21), 0x00074859 },				\
	{ AR5K_AR5210_PHY(22), 0x7e80beba },				\
	{ AR5K_AR5210_PHY(23), 0x313a665e },				\
	{ AR5K_AR5210_PHY_AGCCTL, 0x00001d08 },				\
	{ AR5K_AR5210_PHY(25), 0x0001ce00 },				\
	{ AR5K_AR5210_PHY(26), 0x409a4190 },				\
	{ AR5K_AR5210_PHY(28), 0x0000000f },				\
	{ AR5K_AR5210_PHY(29), 0x00000080 },				\
	{ AR5K_AR5210_PHY(30), 0x00000004 },				\
	{ AR5K_AR5210_PHY(31), 0x00000018 }, /* 0x987c */		\
	{ AR5K_AR5210_PHY(64), 0x00000000 }, /* 0x9900 */		\
	{ AR5K_AR5210_PHY(65), 0x00000000 },				\
	{ AR5K_AR5210_PHY(66), 0x00000000 },				\
	{ AR5K_AR5210_PHY(67), 0x00800000 },				\
	{ AR5K_AR5210_PHY(68), 0x00000003 },				\
	/* BB gain table (64bytes) */					\
	{ AR5K_AR5210_BB_GAIN(0), 0x00000000 },				\
	{ AR5K_AR5210_BB_GAIN(0x01), 0x00000020 },			\
	{ AR5K_AR5210_BB_GAIN(0x02), 0x00000010 },			\
	{ AR5K_AR5210_BB_GAIN(0x03), 0x00000030 },			\
	{ AR5K_AR5210_BB_GAIN(0x04), 0x00000008 },			\
	{ AR5K_AR5210_BB_GAIN(0x05), 0x00000028 },			\
	{ AR5K_AR5210_BB_GAIN(0x06), 0x00000028 },			\
	{ AR5K_AR5210_BB_GAIN(0x07), 0x00000004 },			\
	{ AR5K_AR5210_BB_GAIN(0x08), 0x00000024 },			\
	{ AR5K_AR5210_BB_GAIN(0x09), 0x00000014 },			\
	{ AR5K_AR5210_BB_GAIN(0x0a), 0x00000034 },			\
	{ AR5K_AR5210_BB_GAIN(0x0b), 0x0000000c },			\
	{ AR5K_AR5210_BB_GAIN(0x0c), 0x0000002c },			\
	{ AR5K_AR5210_BB_GAIN(0x0d), 0x00000002 },			\
	{ AR5K_AR5210_BB_GAIN(0x0e), 0x00000022 },			\
	{ AR5K_AR5210_BB_GAIN(0x0f), 0x00000012 },			\
	{ AR5K_AR5210_BB_GAIN(0x10), 0x00000032 },			\
	{ AR5K_AR5210_BB_GAIN(0x11), 0x0000000a },			\
	{ AR5K_AR5210_BB_GAIN(0x12), 0x0000002a },			\
	{ AR5K_AR5210_BB_GAIN(0x13), 0x00000001 },			\
	{ AR5K_AR5210_BB_GAIN(0x14), 0x00000021 },			\
	{ AR5K_AR5210_BB_GAIN(0x15), 0x00000011 },			\
	{ AR5K_AR5210_BB_GAIN(0x16), 0x00000031 },			\
	{ AR5K_AR5210_BB_GAIN(0x17), 0x00000009 },			\
	{ AR5K_AR5210_BB_GAIN(0x18), 0x00000029 },			\
	{ AR5K_AR5210_BB_GAIN(0x19), 0x00000005 },			\
	{ AR5K_AR5210_BB_GAIN(0x1a), 0x00000025 },			\
	{ AR5K_AR5210_BB_GAIN(0x1b), 0x00000015 },			\
	{ AR5K_AR5210_BB_GAIN(0x1c), 0x00000035 },			\
	{ AR5K_AR5210_BB_GAIN(0x1d), 0x0000000d },			\
	{ AR5K_AR5210_BB_GAIN(0x1e), 0x0000002d },			\
	{ AR5K_AR5210_BB_GAIN(0x1f), 0x00000003 },			\
	{ AR5K_AR5210_BB_GAIN(0x20), 0x00000023 },			\
	{ AR5K_AR5210_BB_GAIN(0x21), 0x00000013 },			\
	{ AR5K_AR5210_BB_GAIN(0x22), 0x00000033 },			\
	{ AR5K_AR5210_BB_GAIN(0x23), 0x0000000b },			\
	{ AR5K_AR5210_BB_GAIN(0x24), 0x0000002b },			\
	{ AR5K_AR5210_BB_GAIN(0x25), 0x00000007 },			\
	{ AR5K_AR5210_BB_GAIN(0x26), 0x00000027 },			\
	{ AR5K_AR5210_BB_GAIN(0x27), 0x00000017 },			\
	{ AR5K_AR5210_BB_GAIN(0x28), 0x00000037 },			\
	{ AR5K_AR5210_BB_GAIN(0x29), 0x0000000f },			\
	{ AR5K_AR5210_BB_GAIN(0x2a), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x2b), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x2c), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x2d), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x2e), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x2f), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x30), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x31), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x32), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x33), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x34), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x35), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x36), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x37), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x38), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x39), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x3a), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x3b), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x3c), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x3d), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x3e), 0x0000002f },			\
	{ AR5K_AR5210_BB_GAIN(0x3f), 0x0000002f },			\
	/* RF gain table (64bytes) */					\
	{ AR5K_AR5210_RF_GAIN(0), 0x0000001d },				\
	{ AR5K_AR5210_RF_GAIN(0x01), 0x0000005d },			\
	{ AR5K_AR5210_RF_GAIN(0x02), 0x0000009d },			\
	{ AR5K_AR5210_RF_GAIN(0x03), 0x000000dd },			\
	{ AR5K_AR5210_RF_GAIN(0x04), 0x0000011d },			\
	{ AR5K_AR5210_RF_GAIN(0x05), 0x00000021 },			\
	{ AR5K_AR5210_RF_GAIN(0x06), 0x00000061 },			\
	{ AR5K_AR5210_RF_GAIN(0x07), 0x000000a1 },			\
	{ AR5K_AR5210_RF_GAIN(0x08), 0x000000e1 },			\
	{ AR5K_AR5210_RF_GAIN(0x09), 0x00000031 },			\
	{ AR5K_AR5210_RF_GAIN(0x0a), 0x00000071 },			\
	{ AR5K_AR5210_RF_GAIN(0x0b), 0x000000b1 },			\
	{ AR5K_AR5210_RF_GAIN(0x0c), 0x0000001c },			\
	{ AR5K_AR5210_RF_GAIN(0x0d), 0x0000005c },			\
	{ AR5K_AR5210_RF_GAIN(0x0e), 0x00000029 },			\
	{ AR5K_AR5210_RF_GAIN(0x0f), 0x00000069 },			\
	{ AR5K_AR5210_RF_GAIN(0x10), 0x000000a9 },			\
	{ AR5K_AR5210_RF_GAIN(0x11), 0x00000020 },			\
	{ AR5K_AR5210_RF_GAIN(0x12), 0x00000019 },			\
	{ AR5K_AR5210_RF_GAIN(0x13), 0x00000059 },			\
	{ AR5K_AR5210_RF_GAIN(0x14), 0x00000099 },			\
	{ AR5K_AR5210_RF_GAIN(0x15), 0x00000030 },			\
	{ AR5K_AR5210_RF_GAIN(0x16), 0x00000005 },			\
	{ AR5K_AR5210_RF_GAIN(0x17), 0x00000025 },			\
	{ AR5K_AR5210_RF_GAIN(0x18), 0x00000065 },			\
	{ AR5K_AR5210_RF_GAIN(0x19), 0x000000a5 },			\
	{ AR5K_AR5210_RF_GAIN(0x1a), 0x00000028 },			\
	{ AR5K_AR5210_RF_GAIN(0x1b), 0x00000068 },			\
	{ AR5K_AR5210_RF_GAIN(0x1c), 0x0000001f },			\
	{ AR5K_AR5210_RF_GAIN(0x1d), 0x0000001e },			\
	{ AR5K_AR5210_RF_GAIN(0x1e), 0x00000018 },			\
	{ AR5K_AR5210_RF_GAIN(0x1f), 0x00000058 },			\
	{ AR5K_AR5210_RF_GAIN(0x20), 0x00000098 },			\
	{ AR5K_AR5210_RF_GAIN(0x21), 0x00000003 },			\
	{ AR5K_AR5210_RF_GAIN(0x22), 0x00000004 },			\
	{ AR5K_AR5210_RF_GAIN(0x23), 0x00000044 },			\
	{ AR5K_AR5210_RF_GAIN(0x24), 0x00000084 },			\
	{ AR5K_AR5210_RF_GAIN(0x25), 0x00000013 },			\
	{ AR5K_AR5210_RF_GAIN(0x26), 0x00000012 },			\
	{ AR5K_AR5210_RF_GAIN(0x27), 0x00000052 },			\
	{ AR5K_AR5210_RF_GAIN(0x28), 0x00000092 },			\
	{ AR5K_AR5210_RF_GAIN(0x29), 0x000000d2 },			\
	{ AR5K_AR5210_RF_GAIN(0x2a), 0x0000002b },			\
	{ AR5K_AR5210_RF_GAIN(0x2b), 0x0000002a },			\
	{ AR5K_AR5210_RF_GAIN(0x2c), 0x0000006a },			\
	{ AR5K_AR5210_RF_GAIN(0x2d), 0x000000aa },			\
	{ AR5K_AR5210_RF_GAIN(0x2e), 0x0000001b },			\
	{ AR5K_AR5210_RF_GAIN(0x2f), 0x0000001a },			\
	{ AR5K_AR5210_RF_GAIN(0x30), 0x0000005a },			\
	{ AR5K_AR5210_RF_GAIN(0x31), 0x0000009a },			\
	{ AR5K_AR5210_RF_GAIN(0x32), 0x000000da },			\
	{ AR5K_AR5210_RF_GAIN(0x33), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x34), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x35), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x36), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x37), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x38), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x39), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x3a), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x3b), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x3c), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x3d), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x3e), 0x00000006 },			\
	{ AR5K_AR5210_RF_GAIN(0x3f), 0x00000006 },			\
	/* PHY activation */						\
	{ AR5K_AR5210_PHY(53), 0x00000020 },				\
	{ AR5K_AR5210_PHY(51), 0x00000004 },				\
	{ AR5K_AR5210_PHY(50), 0x00060106 },				\
	{ AR5K_AR5210_PHY(39), 0x0000006d },				\
	{ AR5K_AR5210_PHY(48), 0x00000000 },				\
	{ AR5K_AR5210_PHY(52), 0x00000014 },				\
	{ AR5K_AR5210_PHY_ACTIVE, AR5K_AR5210_PHY_ENABLE },		\
}

#endif /* _AR5K_AR5210_VAR_H */
