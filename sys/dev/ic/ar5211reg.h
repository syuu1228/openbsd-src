/*	$OpenBSD: src/sys/dev/ic/ar5211reg.h,v 1.11 2007/03/12 01:04:52 reyk Exp $	*/

/*
 * Copyright (c) 2004, 2005, 2006, 2007 Reyk Floeter <reyk@openbsd.org>
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
 * Known registers of the Atheros AR5001 Wireless LAN chipsets
 * (AR5211/AR5311).
 */

#ifndef _AR5K_AR5211_REG_H
#define _AR5K_AR5211_REG_H

/*
 * Command register
 */
#define AR5K_AR5211_CR		0x0008
#define AR5K_AR5211_CR_RXE	0x00000004
#define AR5K_AR5211_CR_RXD	0x00000020
#define AR5K_AR5211_CR_SWI	0x00000040

/*
 * Receive queue descriptor pointer register
 */
#define AR5K_AR5211_RXDP	0x000c

/*
 * Configuration and status register
 */
#define AR5K_AR5211_CFG			0x0014
#define AR5K_AR5211_CFG_SWTD		0x00000001
#define AR5K_AR5211_CFG_SWTB		0x00000002
#define AR5K_AR5211_CFG_SWRD		0x00000004
#define AR5K_AR5211_CFG_SWRB		0x00000008
#define AR5K_AR5211_CFG_SWRG		0x00000010
#define AR5K_AR5211_CFG_ADHOC		0x00000020
#define AR5K_AR5211_CFG_PHY_OK		0x00000100
#define AR5K_AR5211_CFG_EEBS		0x00000200
#define	AR5K_AR5211_CFG_CLKGD		0x00000400
#define	AR5K_AR5211_CFG_PCI_THRES	0x00060000
#define	AR5K_AR5211_CFG_PCI_THRES_S	17

/*
 * Interrupt enable register
 */
#define AR5K_AR5211_IER		0x0024
#define AR5K_AR5211_IER_DISABLE	0x00000000
#define AR5K_AR5211_IER_ENABLE	0x00000001

/*
 * First RTS duration register
 */
#define AR5K_AR5211_RTSD0	0x0028
#define	AR5K_AR5211_RTSD0_6	0x000000ff
#define	AR5K_AR5211_RTSD0_6_S	0
#define	AR5K_AR5211_RTSD0_9	0x0000ff00
#define	AR5K_AR5211_RTSD0_9_S	8
#define	AR5K_AR5211_RTSD0_12	0x00ff0000
#define	AR5K_AR5211_RTSD0_12_S	16
#define	AR5K_AR5211_RTSD0_18	0xff000000
#define	AR5K_AR5211_RTSD0_18_S	24

/*
 * Second RTS duration register
 */
#define AR5K_AR5211_RTSD1	0x002c
#define	AR5K_AR5211_RTSD1_24	0x000000ff
#define	AR5K_AR5211_RTSD1_24_S	0
#define	AR5K_AR5211_RTSD1_36	0x0000ff00
#define	AR5K_AR5211_RTSD1_36_S	8
#define	AR5K_AR5211_RTSD1_48	0x00ff0000
#define	AR5K_AR5211_RTSD1_48_S	16
#define	AR5K_AR5211_RTSD1_54	0xff000000
#define	AR5K_AR5211_RTSD1_54_S	24

/*
 * Transmit configuration register
 */
#define AR5K_AR5211_TXCFG		0x0030
#define AR5K_AR5211_TXCFG_SDMAMR	0x00000007
#define AR5K_AR5211_TXCFG_SDMAMR_S	0
#define AR5K_AR5211_TXCFG_B_MODE	0x00000008
#define AR5K_AR5211_TXCFG_TXFULL	0x000003f0
#define AR5K_AR5211_TXCFG_TXFULL_S	4
#define AR5K_AR5211_TXCFG_TXFULL_0B	0x00000000
#define AR5K_AR5211_TXCFG_TXFULL_64B	0x00000010
#define AR5K_AR5211_TXCFG_TXFULL_128B	0x00000020
#define AR5K_AR5211_TXCFG_TXFULL_192B	0x00000030
#define AR5K_AR5211_TXCFG_TXFULL_256B	0x00000040
#define AR5K_AR5211_TXCFG_TXCONT_ENABLE	0x00000080
#define AR5K_AR5211_TXCFG_DMASIZE	0x00000100
#define AR5K_AR5211_TXCFG_JUMBO_TXE	0x00000400
#define AR5K_AR5211_TXCFG_RTSRND	0x00001000
#define AR5K_AR5211_TXCFG_FRMPAD_DIS	0x00002000
#define AR5K_AR5211_TXCFG_RDY_DIS	0x00004000

/*
 * Receive configuration register
 */
#define AR5K_AR5211_RXCFG			0x0034
#define AR5K_AR5211_RXCFG_SDMAMW		0x00000007
#define AR5K_AR5211_RXCFG_SDMAMW_S		0
#define	AR5K_AR5311_RXCFG_DEFAULT_ANTENNA	0x00000008
#define AR5K_AR5211_RXCFG_ZLFDMA		0x00000010
#define AR5K_AR5211_RXCFG_JUMBO_RXE		0x00000020
#define AR5K_AR5211_RXCFG_JUMBO_WRAP		0x00000040

/*
 * Receive jumbo descriptor last address register
 */
#define AR5K_AR5211_RXJLA		0x0038

/*
 * MIB control register
 */
#define AR5K_AR5211_MIBC		0x0040
#define AR5K_AR5211_MIBC_COW		0x00000001
#define AR5K_AR5211_MIBC_FMC		0x00000002
#define AR5K_AR5211_MIBC_CMC		0x00000004
#define AR5K_AR5211_MIBC_MCS		0x00000008

/*
 * Timeout prescale register
 */
#define AR5K_AR5211_TOPS		0x0044
#define	AR5K_AR5211_TOPS_M		0x0000ffff

/*
 * Receive timeout register (no frame received)
 */
#define AR5K_AR5211_RXNOFRM		0x0048
#define	AR5K_AR5211_RXNOFRM_M		0x000003ff

/*
 * Transmit timeout register (no frame sent)
 */
#define AR5K_AR5211_TXNOFRM		0x004c
#define	AR5K_AR5211_TXNOFRM_M		0x000003ff
#define	AR5K_AR5211_TXNOFRM_QCU		0x000ffc00

/*
 * Receive frame gap timeout register
 */
#define AR5K_AR5211_RPGTO		0x0050
#define AR5K_AR5211_RPGTO_M		0x000003ff

/*
 * Receive frame count limit register
 */
#define AR5K_AR5211_RFCNT		0x0054
#define AR5K_AR5211_RFCNT_M		0x0000001f

/*
 * Misc settings register
 */
#define AR5K_AR5211_MISC		0x0058
#define	AR5K_AR5211_MISC_DMA_OBS_M	0x000001e0
#define	AR5K_AR5211_MISC_DMA_OBS_S	5
#define	AR5K_AR5211_MISC_MISC_OBS_M	0x00000e00
#define	AR5K_AR5211_MISC_MISC_OBS_S	9
#define	AR5K_AR5211_MISC_MAC_OBS_LSB_M	0x00007000
#define	AR5K_AR5211_MISC_MAC_OBS_LSB_S	12
#define	AR5K_AR5211_MISC_MAC_OBS_MSB_M	0x00038000
#define	AR5K_AR5211_MISC_MAC_OBS_MSB_S	15

/*
 * QCU/DCU clock gating register
 */
#define	AR5K_AR5311_QCUDCU_CLKGT
#define	AR5K_AR5311_QCUDCU_CLKGT_QCU	0x0000ffff
#define	AR5K_AR5311_QCUDCU_CLKGT_DCU	0x07ff0000

/*
 * Primary interrupt status register
 */
#define AR5K_AR5211_PISR		0x0080
#define AR5K_AR5211_PISR_RXOK		0x00000001
#define AR5K_AR5211_PISR_RXDESC		0x00000002
#define AR5K_AR5211_PISR_RXERR		0x00000004
#define AR5K_AR5211_PISR_RXNOFRM	0x00000008
#define AR5K_AR5211_PISR_RXEOL		0x00000010
#define AR5K_AR5211_PISR_RXORN		0x00000020
#define AR5K_AR5211_PISR_TXOK		0x00000040
#define AR5K_AR5211_PISR_TXDESC		0x00000080
#define AR5K_AR5211_PISR_TXERR		0x00000100
#define AR5K_AR5211_PISR_TXNOFRM	0x00000200
#define AR5K_AR5211_PISR_TXEOL		0x00000400
#define AR5K_AR5211_PISR_TXURN		0x00000800
#define AR5K_AR5211_PISR_MIB		0x00001000
#define AR5K_AR5211_PISR_SWI		0x00002000
#define AR5K_AR5211_PISR_RXPHY		0x00004000
#define AR5K_AR5211_PISR_RXKCM		0x00008000
#define AR5K_AR5211_PISR_SWBA		0x00010000
#define AR5K_AR5211_PISR_BRSSI		0x00020000
#define AR5K_AR5211_PISR_BMISS		0x00040000
#define AR5K_AR5211_PISR_HIUERR		0x00080000
#define AR5K_AR5211_PISR_BNR		0x00100000
#define AR5K_AR5211_PISR_TIM		0x00800000
#define AR5K_AR5211_PISR_GPIO		0x01000000
#define AR5K_AR5211_PISR_QCBRORN	0x02000000
#define AR5K_AR5211_PISR_QCBRURN	0x04000000
#define AR5K_AR5211_PISR_QTRIG		0x08000000

/*
 * Secondary interrupt status registers (0 - 4)
 */
#define AR5K_AR5211_SISR0		0x0084
#define AR5K_AR5211_SISR0_QCU_TXOK	0x000003ff
#define AR5K_AR5211_SISR0_QCU_TXDESC	0x03ff0000

#define AR5K_AR5211_SISR1		0x0088
#define AR5K_AR5211_SISR1_QCU_TXERR	0x000003ff
#define AR5K_AR5211_SISR1_QCU_TXEOL	0x03ff0000

#define AR5K_AR5211_SISR2		0x008c
#define AR5K_AR5211_SISR2_QCU_TXURN	0x000003ff
#define	AR5K_AR5211_SISR2_MCABT		0x00100000
#define	AR5K_AR5211_SISR2_SSERR		0x00200000
#define	AR5K_AR5211_SISR2_DPERR		0x00400000

#define AR5K_AR5211_SISR3		0x0090
#define AR5K_AR5211_SISR3_QCBRORN	0x000003ff
#define AR5K_AR5211_SISR3_QCBRURN	0x03ff0000

#define AR5K_AR5211_SISR4		0x0094
#define AR5K_AR5211_SISR4_QTRIG		0x000003ff

/*
 * Shadow read-and-clear interrupt status registers
 */
#define AR5K_AR5211_RAC_PISR	0x00c0
#define AR5K_AR5211_RAC_SISR0	0x00c4
#define AR5K_AR5211_RAC_SISR1	0x00c8
#define AR5K_AR5211_RAC_SISR2	0x00cc
#define AR5K_AR5211_RAC_SISR3	0c00d0
#define AR5K_AR5211_RAC_SISR4	0c00d4

/*
 * Primary interrupt mask register
 */
#define AR5K_AR5211_PIMR		0x00a0
#define AR5K_AR5211_PIMR_RXOK		0x00000001
#define AR5K_AR5211_PIMR_RXDESC		0x00000002
#define AR5K_AR5211_PIMR_RXERR		0x00000004
#define AR5K_AR5211_PIMR_RXNOFRM	0x00000008
#define AR5K_AR5211_PIMR_RXEOL		0x00000010
#define AR5K_AR5211_PIMR_RXORN		0x00000020
#define AR5K_AR5211_PIMR_TXOK		0x00000040
#define AR5K_AR5211_PIMR_TXDESC		0x00000080
#define AR5K_AR5211_PIMR_TXERR		0x00000100
#define AR5K_AR5211_PIMR_TXNOFRM	0x00000200
#define AR5K_AR5211_PIMR_TXEOL		0x00000400
#define AR5K_AR5211_PIMR_TXURN		0x00000800
#define AR5K_AR5211_PIMR_MIB		0x00001000
#define AR5K_AR5211_PIMR_SWI		0x00002000
#define AR5K_AR5211_PIMR_RXPHY		0x00004000
#define AR5K_AR5211_PIMR_RXKCM		0x00008000
#define AR5K_AR5211_PIMR_SWBA		0x00010000
#define AR5K_AR5211_PIMR_BRSSI		0x00020000
#define AR5K_AR5211_PIMR_BMISS		0x00040000
#define AR5K_AR5211_PIMR_HIUERR		0x00080000
#define AR5K_AR5211_PIMR_BNR		0x00100000
#define AR5K_AR5211_PIMR_TIM		0x00800000
#define AR5K_AR5211_PIMR_GPIO		0x01000000
#define AR5K_AR5211_PIMR_QCBRORN	0x02000000
#define AR5K_AR5211_PIMR_QCBRURN	0x04000000
#define AR5K_AR5211_PIMR_QTRIG		0x08000000

/*
 * Secondary interrupt mask registers (0 - 4)
 */
#define AR5K_AR5211_SIMR0		0x00a4
#define AR5K_AR5211_SIMR0_QCU_TXOK	0x000003ff
#define AR5K_AR5211_SIMR0_QCU_TXOK_S	0
#define AR5K_AR5211_SIMR0_QCU_TXDESC	0x03ff0000
#define AR5K_AR5211_SIMR0_QCU_TXDESC_S	16

#define AR5K_AR5211_SIMR1		0x00a8
#define AR5K_AR5211_SIMR1_QCU_TXERR	0x000003ff
#define AR5K_AR5211_SIMR1_QCU_TXERR_S	0
#define AR5K_AR5211_SIMR1_QCU_TXEOL	0x03ff0000
#define AR5K_AR5211_SIMR1_QCU_TXEOL_S	16

#define AR5K_AR5211_SIMR2		0x00ac
#define AR5K_AR5211_SIMR2_QCU_TXURN	0x000003ff
#define AR5K_AR5211_SIMR2_QCU_TXURN_S	0
#define	AR5K_AR5211_SIMR2_MCABT		0x00100000
#define	AR5K_AR5211_SIMR2_SSERR		0x00200000
#define	AR5K_AR5211_SIMR2_DPERR		0x00400000

#define AR5K_AR5211_SIMR3		0x00b0
#define AR5K_AR5211_SIMR3_QCBRORN	0x000003ff
#define AR5K_AR5211_SIMR3_QCBRORN_S	0
#define AR5K_AR5211_SIMR3_QCBRURN	0x03ff0000
#define AR5K_AR5211_SIMR3_QCBRURN_S	16

#define AR5K_AR5211_SIMR4		0x00b4
#define AR5K_AR5211_SIMR4_QTRIG		0x000003ff
#define AR5K_AR5211_SIMR4_QTRIG_S	0

/*
 * Queue control unit (QCU) registers (0 - 9)
 */
#define	AR5K_AR5211_QCU(_n, _a)		(((_n) << 2) + _a)

/*
 * QCU Transmit descriptor pointer registers
 */
#define AR5K_AR5211_QCU_TXDP(_n)	AR5K_AR5211_QCU(_n, 0x0800)

/*
 * QCU Transmit enable register
 */
#define AR5K_AR5211_QCU_TXE		0x0840

/*
 * QCU Transmit disable register
 */
#define AR5K_AR5211_QCU_TXD		0x0880

/*
 * QCU CBR configuration registers
 */
#define	AR5K_AR5211_QCU_CBRCFG(_n)		AR5K_AR5211_QCU(_n, 0x08c0)
#define	AR5K_AR5211_QCU_CBRCFG_INTVAL		0x00ffffff
#define AR5K_AR5211_QCU_CBRCFG_INTVAL_S		0
#define	AR5K_AR5211_QCU_CBRCFG_ORN_THRES	0xff000000
#define AR5K_AR5211_QCU_CBRCFG_ORN_THRES_S	24

/*
 * QCU Ready time configuration registers
 */
#define	AR5K_AR5211_QCU_RDYTIMECFG(_n)		AR5K_AR5211_QCU(_n, 0x0900)
#define	AR5K_AR5211_QCU_RDYTIMECFG_INTVAL	0x00ffffff
#define AR5K_AR5211_QCU_RDYTIMECFG_INTVAL_S	0
#define	AR5K_AR5211_QCU_RDYTIMECFG_DURATION	0x00ffffff
#define	AR5K_AR5211_QCU_RDYTIMECFG_ENABLE	0x01000000

/*
 * QCU one shot arm set registers
 */
#define	AR5K_AR5211_QCU_ONESHOTARMS(_n)	AR5K_AR5211_QCU(_n, 0x0940)
#define	AR5K_AR5211_QCU_ONESHOTARMS_M	0x0000ffff

/*
 * QCU one shot arm clear registers
 */
#define	AR5K_AR5211_QCU_ONESHOTARMC(_n)	AR5K_AR5211_QCU(_n, 0x0980)
#define	AR5K_AR5211_QCU_ONESHOTARMC_M	0x0000ffff

/*
 * QCU misc registers
 */
#define AR5K_AR5211_QCU_MISC(_n)		AR5K_AR5211_QCU(_n, 0x09c0)
#define	AR5K_AR5211_QCU_MISC_FRSHED_M		0x0000000f
#define	AR5K_AR5211_QCU_MISC_FRSHED_ASAP	0
#define	AR5K_AR5211_QCU_MISC_FRSHED_CBR		1
#define	AR5K_AR5211_QCU_MISC_FRSHED_DBA_GT	2
#define	AR5K_AR5211_QCU_MISC_FRSHED_TIM_GT	3
#define	AR5K_AR5211_QCU_MISC_FRSHED_BCN_SENT_GT	4
#define	AR5K_AR5211_QCU_MISC_ONESHOT_ENABLE	0x00000010
#define	AR5K_AR5211_QCU_MISC_CBREXP		0x00000020
#define	AR5K_AR5211_QCU_MISC_CBREXP_BCN		0x00000040
#define	AR5K_AR5211_QCU_MISC_BCN_ENABLE		0x00000080
#define	AR5K_AR5211_QCU_MISC_CBR_THRES_ENABLE	0x00000100
#define	AR5K_AR5211_QCU_MISC_TXE		0x00000200
#define	AR5K_AR5211_QCU_MISC_CBR		0x00000400
#define	AR5K_AR5211_QCU_MISC_DCU_EARLY		0x00000800

/*
 * QCU status registers
 */
#define AR5K_AR5211_QCU_STS(_n)		AR5K_AR5211_QCU(_n, 0x0a00)
#define	AR5K_AR5211_QCU_STS_FRMPENDCNT	0x00000003
#define	AR5K_AR5211_QCU_STS_CBREXPCNT	0x0000ff00

/*
 * QCU ready time shutdown register
 */
#define AR5K_AR5211_QCU_RDYTIMESHDN	0x0a40
#define AR5K_AR5211_QCU_RDYTIMESHDN_M	0x000003ff

/*
 * DCF control unit (DCU) registers (0 - 9)
 */
#define	AR5K_AR5211_DCU(_n, _a)		AR5K_AR5211_QCU(_n, _a)

/*
 * DCU QCU mask registers
 */
#define AR5K_AR5211_DCU_QCUMASK(_n)	AR5K_AR5211_DCU(_n, 0x1000)
#define AR5K_AR5211_DCU_QCUMASK_M	0x000003ff

/*
 * DCU local IFS settings register
 */
#define AR5K_AR5211_DCU_LCL_IFS(_n)		AR5K_AR5211_DCU(_n, 0x1040)
#define	AR5K_AR5211_DCU_LCL_IFS_CW_MIN		0x000003ff
#define	AR5K_AR5211_DCU_LCL_IFS_CW_MIN_S	0
#define	AR5K_AR5211_DCU_LCL_IFS_CW_MAX		0x000ffc00
#define	AR5K_AR5211_DCU_LCL_IFS_CW_MAX_S	10
#define	AR5K_AR5211_DCU_LCL_IFS_AIFS		0x0ff00000
#define	AR5K_AR5211_DCU_LCL_IFS_AIFS_S		20

/*
 * DCU retry limit registers
 */
#define AR5K_AR5211_DCU_RETRY_LMT(_n)		AR5K_AR5211_DCU(_n, 0x1080)
#define AR5K_AR5211_DCU_RETRY_LMT_SH_RETRY	0x0000000f
#define AR5K_AR5211_DCU_RETRY_LMT_SH_RETRY_S	0
#define AR5K_AR5211_DCU_RETRY_LMT_LG_RETRY	0x000000f0
#define AR5K_AR5211_DCU_RETRY_LMT_LG_RETRY_S	4
#define AR5K_AR5211_DCU_RETRY_LMT_SSH_RETRY	0x00003f00
#define AR5K_AR5211_DCU_RETRY_LMT_SSH_RETRY_S	8
#define AR5K_AR5211_DCU_RETRY_LMT_SLG_RETRY	0x000fc000
#define AR5K_AR5211_DCU_RETRY_LMT_SLG_RETRY_S	14

/*
 * DCU channel time registers
 */
#define AR5K_AR5211_DCU_CHAN_TIME(_n)		AR5K_AR5211_DCU(_n, 0x10c0)
#define	AR5K_AR5211_DCU_CHAN_TIME_ENABLE	0x00100000
#define	AR5K_AR5211_DCU_CHAN_TIME_DUR		0x000fffff
#define	AR5K_AR5211_DCU_CHAN_TIME_DUR_S		0

/*
 * DCU misc registers
 */
#define AR5K_AR5211_DCU_MISC(_n)		AR5K_AR5211_DCU(_n, 0x1100)
#define	AR5K_AR5211_DCU_MISC_BACKOFF		0x000007ff
#define AR5K_AR5211_DCU_MISC_BACKOFF_FRAG	0x00000200
#define	AR5K_AR5211_DCU_MISC_HCFPOLL_ENABLE	0x00000800
#define	AR5K_AR5211_DCU_MISC_BACKOFF_PERSIST	0x00001000
#define	AR5K_AR5211_DCU_MISC_FRMPRFTCH_ENABLE	0x00002000
#define	AR5K_AR5211_DCU_MISC_VIRTCOL		0x0000c000
#define	AR5K_AR5211_DCU_MISC_VIRTCOL_NORMAL	0
#define	AR5K_AR5211_DCU_MISC_VIRTCOL_MODIFIED	1
#define	AR5K_AR5211_DCU_MISC_VIRTCOL_IGNORE	2
#define	AR5K_AR5211_DCU_MISC_BCN_ENABLE		0x00010000
#define	AR5K_AR5211_DCU_MISC_ARBLOCK_CTL	0x00060000
#define	AR5K_AR5211_DCU_MISC_ARBLOCK_CTL_S	17
#define	AR5K_AR5211_DCU_MISC_ARBLOCK_CTL_NONE	0
#define	AR5K_AR5211_DCU_MISC_ARBLOCK_CTL_INTFRM	1
#define	AR5K_AR5211_DCU_MISC_ARBLOCK_CTL_GLOBAL	2
#define	AR5K_AR5211_DCU_MISC_ARBLOCK_IGNORE	0x00080000
#define	AR5K_AR5211_DCU_MISC_SEQ_NUM_INCR_DIS	0x00100000
#define	AR5K_AR5211_DCU_MISC_POST_FR_BKOFF_DIS	0x00200000
#define	AR5K_AR5211_DCU_MISC_VIRT_COLL_POLICY	0x00400000
#define	AR5K_AR5211_DCU_MISC_BLOWN_IFS_POLICY	0x00800000
#define	AR5K_AR5211_DCU_MISC_SEQNUM_CTL		0x01000000

/*
 * DCU frame sequence number registers
 */
#define AR5K_AR5211_DCU_SEQNUM(_n)	AR5K_AR5211_DCU(_n, 0x1140)
#define	AR5K_AR5211_DCU_SEQNUM_M	0x00000fff
/*
 * DCU global IFS SIFS registers
 */
#define AR5K_AR5211_DCU_GBL_IFS_SIFS	0x1030
#define AR5K_AR5211_DCU_GBL_IFS_SIFS_M	0x0000ffff

/*
 * DCU global IFS slot interval registers
 */
#define AR5K_AR5211_DCU_GBL_IFS_SLOT	0x1070
#define AR5K_AR5211_DCU_GBL_IFS_SLOT_M	0x0000ffff

/*
 * DCU global IFS EIFS registers
 */
#define AR5K_AR5211_DCU_GBL_IFS_EIFS	0x10b0
#define AR5K_AR5211_DCU_GBL_IFS_EIFS_M	0x0000ffff

/*
 * DCU global IFS misc registers
 */
#define AR5K_AR5211_DCU_GBL_IFS_MISC			0x10f0
#define	AR5K_AR5211_DCU_GBL_IFS_MISC_LFSR_SLICE		0x00000007
#define	AR5K_AR5211_DCU_GBL_IFS_MISC_TURBO_MODE		0x00000008
#define	AR5K_AR5211_DCU_GBL_IFS_MISC_SIFS_DUR_USEC	0x000003f0
#define	AR5K_AR5211_DCU_GBL_IFS_MISC_USEC_DUR		0x000ffc00
#define	AR5K_AR5211_DCU_GBL_IFS_MISC_DCU_ARB_DELAY	0x00300000

/*
 * DCU frame prefetch control register
 */
#define AR5K_AR5211_DCU_FP		0x1230

/*
 * DCU transmit pause control/status register
 */
#define AR5K_AR5211_DCU_TXP		0x1270
#define	AR5K_AR5211_DCU_TXP_M		0x000003ff
#define	AR5K_AR5211_DCU_TXP_STATUS	0x00010000

/*
 * DCU transmit filter register
 */
#define AR5K_AR5211_DCU_TX_FILTER	0x1038

/*
 * DCU clear transmit filter register
 */
#define AR5K_AR5211_DCU_TX_FILTER_CLR	0x143c

/*
 * DCU set transmit filter register
 */
#define AR5K_AR5211_DCU_TX_FILTER_SET	0x147c

/*
 * DMA size definitions
 */
typedef enum {
	AR5K_AR5211_DMASIZE_4B = 0,
	AR5K_AR5211_DMASIZE_8B = 1,
	AR5K_AR5211_DMASIZE_16B = 2,
	AR5K_AR5211_DMASIZE_32B = 3,
	AR5K_AR5211_DMASIZE_64B = 4,
	AR5K_AR5211_DMASIZE_128B = 5,
	AR5K_AR5211_DMASIZE_256B = 6,
	AR5K_AR5211_DMASIZE_512B = 7
} ar5k_ar5211_dmasize_t;

/*
 * Reset control register
 */
#define AR5K_AR5211_RC			0x4000
#define AR5K_AR5211_RC_PCU		0x00000001
#define AR5K_AR5211_RC_BB		0x00000002
#define AR5K_AR5211_RC_PCI		0x00000010
#define AR5K_AR5211_RC_CHIP		(				\
	AR5K_AR5211_RC_PCU | AR5K_AR5211_RC_BB | AR5K_AR5211_RC_PCI	\
)

/*
 * Sleep control register
 */
#define AR5K_AR5211_SCR			0x4004
#define AR5K_AR5211_SCR_SLDUR		0x0000ffff
#define AR5K_AR5211_SCR_SLDUR_S		0
#define AR5K_AR5211_SCR_SLE		0x00030000
#define AR5K_AR5211_SCR_SLE_S		16
#define AR5K_AR5211_SCR_SLE_WAKE	0x00000000
#define AR5K_AR5211_SCR_SLE_SLP		0x00010000
#define AR5K_AR5211_SCR_SLE_ALLOW	0x00020000
#define AR5K_AR5211_SCR_SLE_UNITS	0x00000008

/*
 * Interrupt pending register
 */
#define AR5K_AR5211_INTPEND	0x4008
#define AR5K_AR5211_INTPEND_M	0x00000001

/*
 * Sleep force register
 */
#define AR5K_AR5211_SFR		0x400c
#define AR5K_AR5211_SFR_M	0x00000001

/*
 * PCI configuration register
 */
#define AR5K_AR5211_PCICFG		0x4010
#define AR5K_AR5211_PCICFG_CLKRUNEN	0x00000004
#define AR5K_AR5211_PCICFG_EESIZE	0x00000018
#define AR5K_AR5211_PCICFG_EESIZE_S	3
#define AR5K_AR5211_PCICFG_EESIZE_4K	0
#define AR5K_AR5211_PCICFG_EESIZE_8K	1
#define AR5K_AR5211_PCICFG_EESIZE_16K	2
#define AR5K_AR5211_PCICFG_EESIZE_FAIL	3
#define AR5K_AR5211_PCICFG_LED		0x00000060
#define AR5K_AR5211_PCICFG_LED_NONE	0x00000000
#define AR5K_AR5211_PCICFG_LED_PEND	0x00000020
#define AR5K_AR5211_PCICFG_LED_ASSOC	0x00000040
#define	AR5K_AR5211_PCICFG_BUS_SEL	0x00000380
#define	AR5K_AR5211_PCICFG_CBEFIX_DIS	0x00000400
#define AR5K_AR5211_PCICFG_SL_INTEN	0x00000800
#define AR5K_AR5211_PCICFG_SL_INPEN	0x00002800
#define AR5K_AR5211_PCICFG_SPWR_DN	0x00010000
#define AR5K_AR5211_PCICFG_LEDMODE	0x000e0000
#define AR5K_AR5211_PCICFG_LEDMODE_PROP	0x00000000
#define AR5K_AR5211_PCICFG_LEDMODE_PROM	0x00020000
#define AR5K_AR5211_PCICFG_LEDMODE_PWR	0x00040000
#define AR5K_AR5211_PCICFG_LEDMODE_RAND	0x00060000
#define AR5K_AR5211_PCICFG_LEDBLINK	0x00700000
#define AR5K_AR5211_PCICFG_LEDBLINK_S	20
#define AR5K_AR5211_PCICFG_LEDSLOW	0x00800000
#define AR5K_AR5211_PCICFG_LEDSTATE					\
	(AR5K_AR5211_PCICFG_LED | AR5K_AR5211_PCICFG_LEDMODE |		\
	AR5K_AR5211_PCICFG_LEDBLINK | AR5K_AR5211_PCICFG_LEDSLOW)

/*
 * "General Purpose Input/Output" (GPIO) control register
 */
#define AR5K_AR5211_GPIOCR		0x4014
#define AR5K_AR5211_GPIOCR_INT_ENA	0x00008000
#define AR5K_AR5211_GPIOCR_INT_SELL	0x00000000
#define AR5K_AR5211_GPIOCR_INT_SELH	0x00010000
#define AR5K_AR5211_GPIOCR_NONE(n)	(0 << ((n) * 2))
#define AR5K_AR5211_GPIOCR_OUT0(n)	(1 << ((n) * 2))
#define AR5K_AR5211_GPIOCR_OUT1(n)	(2 << ((n) * 2))
#define AR5K_AR5211_GPIOCR_ALL(n)	(3 << ((n) * 2))
#define AR5K_AR5211_GPIOCR_INT_SEL(n)	((n) << 12)

#define AR5K_AR5211_NUM_GPIO	6

/*
 * "General Purpose Input/Output" (GPIO) data output register
 */
#define AR5K_AR5211_GPIODO	0x4018

/*
 * "General Purpose Input/Output" (GPIO) data input register
 */
#define AR5K_AR5211_GPIODI	0x401c
#define AR5K_AR5211_GPIODI_M	0x0000002f

/*
 * Silicon revision register
 */
#define AR5K_AR5211_SREV		0x4020
#define AR5K_AR5211_SREV_REV		0x0000000f
#define AR5K_AR5211_SREV_REV_S		0
#define AR5K_AR5211_SREV_VER		0x000000ff
#define AR5K_AR5211_SREV_VER_S		4

/*
 * EEPROM access registers
 */
#define AR5K_AR5211_EEPROM_BASE		0x6000
#define AR5K_AR5211_EEPROM_DATA		0x6004
#define AR5K_AR5211_EEPROM_CMD		0x6008
#define AR5K_AR5211_EEPROM_CMD_READ	0x00000001
#define AR5K_AR5211_EEPROM_CMD_WRITE	0x00000002
#define AR5K_AR5211_EEPROM_CMD_RESET	0x00000004
#define AR5K_AR5211_EEPROM_STATUS	0x600c
#define AR5K_AR5211_EEPROM_STAT_RDERR	0x00000001
#define AR5K_AR5211_EEPROM_STAT_RDDONE	0x00000002
#define AR5K_AR5211_EEPROM_STAT_WRERR	0x00000004
#define AR5K_AR5211_EEPROM_STAT_WRDONE	0x00000008
#define AR5K_AR5211_EEPROM_CFG		0x6010

/*
 * AR5211 EEPROM data registers
 */
#define	AR5K_AR5211_EEPROM_MAGIC		0x3d
#define	AR5K_AR5211_EEPROM_MAGIC_VALUE		0x5aa5
#define AR5K_AR5211_EEPROM_PROTECT		0x3f
#define	AR5K_AR5211_EEPROM_PROTECT_128_191	0x80
#define AR5K_AR5211_EEPROM_REG_DOMAIN		0xbf
#define AR5K_AR5211_EEPROM_INFO_BASE		0xc0
#define AR5K_AR5211_EEPROM_INFO_VERSION					\
	(AR5K_AR5211_EEPROM_INFO_BASE + 1)
#define	AR5K_AR5211_EEPROM_INFO_MAX					\
	(0x400 - AR5K_AR5211_EEPROM_INFO_BASE)

/*
 * PCU registers
 */

#define AR5K_AR5211_PCU_MIN	0x8000
#define AR5K_AR5211_PCU_MAX	0x8fff

/*
 * First station id register (MAC address in lower 32 bits)
 */
#define AR5K_AR5211_STA_ID0	0x8000

/*
 * Second station id register (MAC address in upper 16 bits)
 */
#define AR5K_AR5211_STA_ID1			0x8004
#define AR5K_AR5211_STA_ID1_AP			0x00010000
#define AR5K_AR5211_STA_ID1_ADHOC		0x00020000
#define AR5K_AR5211_STA_ID1_PWR_SV		0x00040000
#define AR5K_AR5211_STA_ID1_NO_KEYSRCH		0x00080000
#define AR5K_AR5211_STA_ID1_PCF			0x00100000
#define AR5K_AR5211_STA_ID1_DEFAULT_ANTENNA	0x00200000
#define AR5K_AR5211_STA_ID1_DESC_ANTENNA	0x00400000
#define AR5K_AR5211_STA_ID1_RTS_DEFAULT_ANTENNA	0x00800000
#define AR5K_AR5211_STA_ID1_ACKCTS_6MB		0x01000000
#define AR5K_AR5211_STA_ID1_BASE_RATE_11B	0x02000000

/*
 * First BSSID register (MAC address, lower 32bits)
 */
#define AR5K_AR5211_BSS_ID0	0x8008

/*
 * Second BSSID register (MAC address in upper 16 bits)
 *
 * AID: Association ID
 */
#define AR5K_AR5211_BSS_ID1		0x800c
#define AR5K_AR5211_BSS_ID1_AID		0xffff0000
#define AR5K_AR5211_BSS_ID1_AID_S	16

/*
 * Backoff slot time register
 */
#define AR5K_AR5211_SLOT_TIME	0x8010

/*
 * ACK/CTS timeout register
 */
#define AR5K_AR5211_TIME_OUT		0x8014
#define AR5K_AR5211_TIME_OUT_ACK	0x00001fff
#define AR5K_AR5211_TIME_OUT_ACK_S	0
#define AR5K_AR5211_TIME_OUT_CTS	0x1fff0000
#define AR5K_AR5211_TIME_OUT_CTS_S	16

/*
 * RSSI threshold register
 */
#define AR5K_AR5211_RSSI_THR		0x8018
#define AR5K_AR5211_RSSI_THR_M		0x000000ff
#define AR5K_AR5211_RSSI_THR_BMISS	0x0000ff00
#define AR5K_AR5211_RSSI_THR_BMISS_S	8

/*
 * Transmit latency register
 */
#define AR5K_AR5211_USEC		0x801c
#define AR5K_AR5211_USEC_1		0x0000007f
#define AR5K_AR5211_USEC_1_S		0
#define AR5K_AR5211_USEC_32		0x00003f80
#define AR5K_AR5211_USEC_32_S		7
#define AR5K_AR5211_USEC_TX_LATENCY	0x007fc000
#define AR5K_AR5211_USEC_TX_LATENCY_S	14
#define AR5K_AR5211_USEC_RX_LATENCY	0x1f800000
#define AR5K_AR5211_USEC_RX_LATENCY_S	23
#define AR5K_AR5311_USEC_TX_LATENCY	0x000fc000
#define AR5K_AR5311_USEC_TX_LATENCY_S	14
#define AR5K_AR5311_USEC_RX_LATENCY	0x03f00000
#define AR5K_AR5311_USEC_RX_LATENCY_S	20

/*
 * PCU beacon control register
 */
#define AR5K_AR5211_BEACON		0x8020
#define AR5K_AR5211_BEACON_PERIOD	0x0000ffff
#define AR5K_AR5211_BEACON_PERIOD_S	0
#define AR5K_AR5211_BEACON_TIM		0x007f0000
#define AR5K_AR5211_BEACON_TIM_S	16
#define AR5K_AR5211_BEACON_ENABLE	0x00800000
#define AR5K_AR5211_BEACON_RESET_TSF	0x01000000

/*
 * CFP period register
 */
#define AR5K_AR5211_CFP_PERIOD		0x8024

/*
 * Next beacon time register
 */
#define AR5K_AR5211_TIMER0		0x8028

/*
 * Next DMA beacon alert register
 */
#define AR5K_AR5211_TIMER1		0x802c

/*
 * Next software beacon alert register
 */
#define AR5K_AR5211_TIMER2		0x8030

/*
 * Next ATIM window time register
 */
#define AR5K_AR5211_TIMER3		0x8034

/*
 * CFP duration register
 */
#define AR5K_AR5211_CFP_DUR		0x8038

/*
 * Receive filter register
 */
#define AR5K_AR5211_RX_FILTER		0x803c
#define AR5K_AR5211_RX_FILTER_UNICAST	0x00000001
#define AR5K_AR5211_RX_FILTER_MULTICAST	0x00000002
#define AR5K_AR5211_RX_FILTER_BROADCAST	0x00000004
#define AR5K_AR5211_RX_FILTER_CONTROL	0x00000008
#define AR5K_AR5211_RX_FILTER_BEACON	0x00000010
#define AR5K_AR5211_RX_FILTER_PROMISC	0x00000020
#define AR5K_AR5211_RX_FILTER_PHYERR	0x00000040
#define AR5K_AR5211_RX_FILTER_RADARERR	0x00000080

/*
 * Multicast filter register (lower 32 bits)
 */
#define AR5K_AR5211_MCAST_FIL0	0x8040

/*
 * Multicast filter register (higher 16 bits)
 */
#define AR5K_AR5211_MCAST_FIL1	0x8044

/*
 * PCU control register
 */
#define AR5K_AR5211_DIAG_SW			0x8048
#define AR5K_AR5211_DIAG_SW_DIS_WEP_ACK		0x00000001
#define AR5K_AR5211_DIAG_SW_DIS_ACK		0x00000002
#define AR5K_AR5211_DIAG_SW_DIS_CTS		0x00000004
#define AR5K_AR5211_DIAG_SW_DIS_ENC		0x00000008
#define AR5K_AR5211_DIAG_SW_DIS_DEC		0x00000010
#define AR5K_AR5211_DIAG_SW_DIS_RX		0x00000020
#define AR5K_AR5211_DIAG_SW_LOOP_BACK		0x00000040
#define AR5K_AR5211_DIAG_SW_CORR_FCS		0x00000080
#define AR5K_AR5211_DIAG_SW_CHAN_INFO		0x00000100
#define AR5K_AR5211_DIAG_SW_EN_SCRAM_SEED	0x00000200
#define AR5K_AR5211_DIAG_SW_ECO_ENABLE		0x00000400
#define AR5K_AR5211_DIAG_SW_SCRAM_SEED_M	0x0001fc00
#define AR5K_AR5211_DIAG_SW_SCRAM_SEED_S	10
#define AR5K_AR5211_DIAG_SW_FRAME_NV0		0x00020000
#define AR5K_AR5211_DIAG_SW_OBSPT_M		0x000c0000
#define AR5K_AR5211_DIAG_SW_OBSPT_S		18

/*
 * TSF (clock) register (lower 32 bits)
 */
#define AR5K_AR5211_TSF_L32	0x804c

/*
 * TSF (clock) register (higher 32 bits)
 */
#define AR5K_AR5211_TSF_U32	0x8050

/*
 * ADDAC test register
 */
#define AR5K_AR5211_ADDAC_TEST	0x8054

/*
 * Default antenna register
 */
#define AR5K_AR5211_DEFAULT_ANTENNA	0x8058

/*
 * Last beacon timestamp register
 */
#define AR5K_AR5211_LAST_TSTP	0x8080

/*
 * NAV register (current)
 */
#define AR5K_AR5211_NAV		0x8084

/*
 * RTS success register
 */
#define AR5K_AR5211_RTS_OK	0x8088

/*
 * RTS failure register
 */
#define AR5K_AR5211_RTS_FAIL	0x808c

/*
 * ACK failure register
 */
#define AR5K_AR5211_ACK_FAIL	0x8090

/*
 * FCS failure register
 */
#define AR5K_AR5211_FCS_FAIL	0x8094

/*
 * Beacon count register
 */
#define AR5K_AR5211_BEACON_CNT	0x8098

/*
 * Key table (WEP) register
 */
#define AR5K_AR5211_KEYTABLE_0		0x8800
#define AR5K_AR5211_KEYTABLE(n)		(AR5K_AR5211_KEYTABLE_0 + ((n) * 32))
#define AR5K_AR5211_KEYTABLE_OFF(_n, x)	(AR5K_AR5211_KEYTABLE(_n) + (x << 2))
#define AR5K_AR5211_KEYTABLE_TYPE(_n)	AR5K_AR5211_KEYTABLE_OFF(_n, 5)
#define AR5K_AR5211_KEYTABLE_TYPE_40	0x00000000
#define AR5K_AR5211_KEYTABLE_TYPE_104	0x00000001
#define AR5K_AR5211_KEYTABLE_TYPE_128	0x00000003
#define AR5K_AR5211_KEYTABLE_TYPE_AES	0x00000005
#define AR5K_AR5211_KEYTABLE_TYPE_NULL	0x00000007
#define AR5K_AR5211_KEYTABLE_MAC0(_n)	AR5K_AR5211_KEYTABLE_OFF(_n, 6)
#define AR5K_AR5211_KEYTABLE_MAC1(_n)	AR5K_AR5211_KEYTABLE_OFF(_n, 7)
#define AR5K_AR5211_KEYTABLE_VALID	0x00008000

#define AR5K_AR5211_KEYTABLE_SIZE	128
#define AR5K_AR5211_KEYCACHE_SIZE	8

/*
 * PHY register
 */
#define	AR5K_AR5211_PHY(_n)		(0x9800 + ((_n) << 2))
#define AR5K_AR5211_PHY_SHIFT_2GHZ	0x00004007
#define AR5K_AR5211_PHY_SHIFT_5GHZ	0x00000007

/*
 * PHY turbo mode register
 */
#define	AR5K_AR5211_PHY_TURBO		0x9804
#define	AR5K_AR5211_PHY_TURBO_MODE	0x00000001
#define	AR5K_AR5211_PHY_TURBO_SHORT	0x00000002

/*
 * PHY agility command register
 */
#define	AR5K_AR5211_PHY_AGC		0x9808
#define	AR5K_AR5211_PHY_AGC_DISABLE	0x08000000

/*
 * PHY chip revision register
 */
#define	AR5K_AR5211_PHY_CHIP_ID		0x9818

/*
 * PHY activation register
 */
#define	AR5K_AR5211_PHY_ACTIVE		0x981c
#define	AR5K_AR5211_PHY_ENABLE		0x00000001
#define	AR5K_AR5211_PHY_DISABLE		0x00000002

/*
 * PHY agility control register
 */
#define	AR5K_AR5211_PHY_AGCCTL		0x9860
#define	AR5K_AR5211_PHY_AGCCTL_CAL	0x00000001
#define	AR5K_AR5211_PHY_AGCCTL_NF	0x00000002

/*
 * PHY noise floor status register
 */
#define AR5K_AR5211_PHY_NF		0x9864
#define AR5K_AR5211_PHY_NF_M		0x000001ff
#define AR5K_AR5211_PHY_NF_ACTIVE	0x00000100
#define AR5K_AR5211_PHY_NF_RVAL(_n)	(((_n) >> 19) & AR5K_AR5211_PHY_NF_M)
#define AR5K_AR5211_PHY_NF_AVAL(_n)	(-((_n) ^ AR5K_AR5211_PHY_NF_M) + 1)
#define AR5K_AR5211_PHY_NF_SVAL(_n)	(((_n) & AR5K_AR5211_PHY_NF_M) | (1 << 9))

/*
 * PHY PLL control register
 */
#define	AR5K_AR5211_PHY_PLL		0x987c
#define	AR5K_AR5211_PHY_PLL_20MHZ	0x13
#define	AR5K_AR5211_PHY_PLL_40MHZ	0x18
#define	AR5K_AR5211_PHY_PLL_44MHZ	0x19

/*
 * PHY receiver delay register
 */
#define	AR5K_AR5211_PHY_RX_DELAY	0x9914
#define	AR5K_AR5211_PHY_RX_DELAY_M	0x00003fff

/*
 * PHY timing IQ control register
 */
#define	AR5K_AR5211_PHY_IQ			0x9920
#define	AR5K_AR5211_PHY_IQ_CORR_Q_Q_COFF	0x0000001f
#define	AR5K_AR5211_PHY_IQ_CORR_Q_I_COFF	0x000007e0
#define	AR5K_AR5211_PHY_IQ_CORR_Q_I_COFF_S	5
#define	AR5K_AR5211_PHY_IQ_CORR_ENABLE		0x00000800
#define	AR5K_AR5211_PHY_IQ_CAL_NUM_LOG_MAX	0x0000f000
#define	AR5K_AR5211_PHY_IQ_CAL_NUM_LOG_MAX_S	12
#define	AR5K_AR5211_PHY_IQ_RUN			0x00010000

/*
 * PHY PAPD probe register
 */
#define	AR5K_AR5211_PHY_PAPD_PROBE		0x9930
#define	AR5K_AR5211_PHY_PAPD_PROBE_TX_PWR	0x00007e00
#define	AR5K_AR5211_PHY_PAPD_PROBE_TX_PWR_S	9
#define	AR5K_AR5211_PHY_PAPD_PROBE_TX_NEXT	0x00008000
#define	AR5K_AR5211_PHY_PAPD_PROBE_GAINF	0xfe000000
#define	AR5K_AR5211_PHY_PAPD_PROBE_GAINF_S	25

/*
 * PHY frame control register
 */
#define	AR5K_AR5211_PHY_FC		0x9944
#define	AR5K_AR5211_PHY_FC_TX_CLIP	0x00000038
#define	AR5K_AR5211_PHY_FC_TX_CLIP_S	3

/*
 * PHY radar detection enable register
 */
#define	AR5K_AR5211_PHY_RADAR		0x9954
#define	AR5K_AR5211_PHY_RADAR_DISABLE	0x00000000
#define	AR5K_AR5211_PHY_RADAR_ENABLE	0x00000001

/*
 * PHY antenna switch table registers
 */
#define AR5K_AR5211_PHY_ANT_SWITCH_TABLE_0	0x9960
#define AR5K_AR5211_PHY_ANT_SWITCH_TABLE_1	0x9964

/*
 * PHY timing IQ calibration result register
 */
#define	AR5K_AR5211_PHY_IQRES_CAL_PWR_I	0x9c10
#define	AR5K_AR5211_PHY_IQRES_CAL_PWR_Q	0x9c14
#define	AR5K_AR5211_PHY_IQRES_CAL_CORR	0x9c18

/*
 * PHY current RSSI register
 */
#define	AR5K_AR5211_PHY_CURRENT_RSSI	0x9c1c

/*
 * PHY mode register
 */
#define	AR5K_AR5211_PHY_MODE		0xa200
#define	AR5K_AR5211_PHY_MODE_MOD	0x00000001
#define AR5K_AR5211_PHY_MODE_MOD_OFDM	0
#define AR5K_AR5211_PHY_MODE_MOD_CCK	1
#define AR5K_AR5211_PHY_MODE_FREQ	0x00000002
#define	AR5K_AR5211_PHY_MODE_FREQ_5GHZ	0
#define	AR5K_AR5211_PHY_MODE_FREQ_2GHZ	2

/*
 * Misc PHY/radio registers
 */
#define AR5K_AR5211_BB_GAIN(_n)		(0x9b00 + ((_n) << 2))
#define AR5K_AR5211_RF_GAIN(_n)		(0x9a00 + ((_n) << 2))

#endif
