/*	$OpenBSD: src/sys/dev/pci/Attic/isesreg.h,v 1.4 2001/06/24 17:06:18 ho Exp $ $	*/

/*
 * Copyright (c) 2000 H�kan Olsson (ho@crt.se)
 * Copyright (c) 2000 Theo de Raadt
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Register definitions for Pijnenburg PCC-ISES crypto chip.  
 * Definitions from revision 1.6 of the product datasheet.
 */

/* 
 * PCC-ISES Evaluation board DMA offsets 
 */
#define ISES_DMA_READ_COUNT	0x0100		/* bit 31-16 */
#define ISES_DMA_RCOUNT(x)	((x) << 16)
#define ISES_DMA_WRITE_COUNT	0x0100		/* bit 15-0  */
#define ISES_DMA_WCOUNT(x)	((x) & 0x00FF)

#define ISES_DMA_WRITE_START	0x0104
#define ISES_DMA_READ_START	0x0108
#define ISES_DMA_CTRL		0x010C
#define ISES_DMA_STATUS		ISES_DMA_CTRL
#define ISES_DMA_RESET		0x0110

#define ISES_DMA_CTRL_ILT	0x40000000	/* Ignore Latency Timer */
#define ISES_DMA_CTRL_RMULT	0x0D000000	/* Enable PCI Read Multiple */
#define ISES_DMA_CTRL_RLINE	0x09000000	/* Enable PCI Read Line */
#define ISES_DMA_CTRL_READ	0x01000000	/* Enable PCI Read */
#define ISES_DMA_CTRL_WRITE	0x00000100	/* Enable PCI Write */

#define ISES_DMA_STATUS_R_RUN	0x01000000	/* PCI Read running */
#define ISES_DMA_STATUS_R_ERR	0x02000000	/* PCI Read error */
#define ISES_DMA_STATUS_W_RUN	0x00000100	/* PCI Write running */
#define ISES_DMA_STATUS_W_ERR	0x00000200	/* PCI Write error */

/* 
 * PCC-ISES A-interface 
 */

#define ISES_A_OFFSET		0x0200
#define ISES_A(x)		(ISES_A_OFFSET + (x))

#define ISES_A_STAT		ISES_A(0x00)	/* status register */
#define ISES_A_INTS		ISES_A(0x04)	/* interupt status register */
#define ISES_A_INTE		ISES_A(0x08)	/* interupt enable register */
#define ISES_A_SREQ		ISES_A(0x0C)	/* service request (read) */
#define ISES_A_CTRL		ISES_A_SREQ	/* control register (write) */
#define ISES_A_OQD		ISES_A(0x10)	/* Output Queue Data (read) */
#define ISES_A_IQD		ISES_A_OQD	/* Input Queue Data (write) */
#define ISES_A_OQS		ISES_A(0x14)	/* Output Queue Semaphore */
#define ISES_A_IQS		ISES_A(0x18)	/* Input Queue Semaphore */
#define ISES_A_OQF		ISES_A(0x1C)	/* Output Queue Filled (ro) */
#define ISES_A_IQF		ISES_A(0x20)	/* Input Queue Free (ro) */

/*
 * PCC-ISES B-interface 
 */

#define ISES_B_OFFSET		0x0300
#define ISES_B(x)		(ISES_B_OFFSET + (x))

#define ISES_B_BDATAIN		ISES_B(0x0)
#define ISES_B_BDATAOUT		ISES_B(0x4)
#define ISES_B_STAT		ISES_B(0x8)

/*
 * PCC-ISES I-interface (not used)
 */

#define ISES_I_OFFSET		0x0400

/* 
 * PCC-ISES board registers
 */

#define ISES_BO_OFFSET		0x0500
#define ISES_BO(x)		(ISES_BO_OFFSET + (x))

#define ISES_BO_STAT		ISES_BO(0x0)
#define ISES_BO_LOOPCOUNTER	ISES_BO(0x4)
#define ISES_BO_TESTREG		ISES_BO(0x8)

#define ISES_BO_STAT_LOOP	0x00000001	/* B-interface LoopMode */
#define ISES_BO_STAT_TAMPER	0x00000002	/* Set tamper */
#define ISES_BO_STAT_POWERDOWN	0x00000004	/* Set power down */
#define ISES_BO_STAT_ACONF	0x00000008	/* Set A-intf access to 16b */
#define ISES_BO_STAT_HWRESET	0x00000010	/* Reset PCC-ISES (hw) */
#define ISES_BO_STAT_AIRQ	0x00000020	/* A-interface interrupt (ro)*/

/* 
 * PCC-ISES A-interface STAT register bits 
 */

#define ISES_STAT_LNAU_MASKED	0x00000001	/* LNAU flags masked, this bit
						   must be zero for the other
						   LNAU flags to be read
						   correctly. */
#define ISES_STAT_LNAU_BUSY_1	0x00000002	/* LNAU unit 1 is busy */
#define ISES_STAT_LNAU_ERR_1	0x00000004	/* LNAU unit 1 error */
#define ISES_STAT_LNAU_BUSY_2	0x00000008	/* LNAU unit 2 is busy */
#define ISES_STAT_LNAU_ERR_2	0x00000010	/* LNAU unit 2 error */
#define ISES_STAT_BCHU_MASKED	0x00000020	/* BCHU flags masked */
#define ISES_STAT_BCHU_BUSY	0x00000040	/* BCHU is busy */
#define ISES_STAT_BCHU_ERR	0x00000080	/* BCHU error flag */
#define ISES_STAT_BCHU_SCIF	0x00000100	/* symm. crypto inoperative */
#define ISES_STAT_BCHU_HIF	0x00000200	/* hash unit inoperative */
#define ISES_STAT_BCHU_DDB	0x00000400	/* discard data blocks */
#define ISES_STAT_BCHU_IRF	0x00000800	/* input request flag */
#define ISES_STAT_BCHU_OAF	0x00001000	/* output available flag */
#define ISES_STAT_BCHU_DIE	0x00002000	/* data input enabled */
#define ISES_STAT_BCHU_UE	0x00004000	/* unit enable bit */
#define ISES_STAT_BCHU_IFE	0x00008000	/* input FIFO empty */
#define ISES_STAT_BCHU_IFHE	0x00010000	/* input FIFO half emtpy */
#define ISES_STAT_BCHU_IFF	0x00020000	/* input FIFO full */
#define ISES_STAT_BCHU_OFE	0x00040000	/* output FIFO emtpy */
#define ISES_STAT_BCHU_OFHF	0x00080000	/* output FIFO half full */
#define ISES_STAT_BCHU_OFF	0x00100000	/* output FIFO full */
#define ISES_STAT_HW_DA		0x00200000	/* downloaded appl flag */
#define ISES_STAT_HW_ACONF	0x00400000	/* A-intf configuration flag */
#define ISES_STAT_SW_WFOQ	0x00800000	/* SW: Waiting for out queue */
#define ISES_STAT_SW_OQSINC	0x08000000	/* SW 2.x: OQS increased */

#define ISES_STAT_IDP_MASK	0x0f000000	/* IDP state mask (HW_DA=0) */
#define ISES_STAT_IDP_STATE(x)  (((x) & ISES_STAT_IDP_MASK) >> 24)

static const char *ises_idp_state[] = 
{
	"reset state",				/* 0x0 */
	"testing NSRAM",			/* 0x1 */
	"checking for firmware",		/* 0x2 */
	"clearing NSRAM",			/* 0x3 */
	"waiting for program length",		/* 0x4 */
	"waiting for program data",		/* 0x5 */
	"waiting for program CRC",		/* 0x6 */
	"functional test program",		/* 0x7 */
	0, 0, 0, 0, 0, 0, 0,			/* 0x8-0xe */
	"Error: NSRAM or firmware failed"	/* 0xf */
};

#define ISES_STAT_SW_MASK	0x03000000	/* SW mode (HW_DA=1) */
#define ISES_STAT_SW_MODE(x)	(((x) & ISES_STAT_SW_MASK) >> 24)

#define ISES_A_CTRL_RESET	0x0000		/* SW reset (go to ST mode) */
#define ISES_A_CTRL_CONTINUE	0x0001		/* Return to CMD from WFC */

#ifdef ISESDEBUG
static const char *ises_sw_mode[] =
{
	"ST (SelfTest)",			/* 0x0 */
	"CMD",					/* 0x1 (normal) */
	"WFC (Wait for continue)",		/* 0x2 */
	"CMD (Wait for reset)"			/* 0x3 */
};
#endif

/* BERR (BCHU Error Register) */
#define ISES_BERR_DPAR		0x00000001	/* DES parity error */
#define ISES_BERR_IDESBCP	0x00000002	/* illegal DES mode value */
#define ISES_BERR_ISFRBCP	0x00000004	/* illegal SAFER rounds spec */
#define ISES_BERR_INCMBCP	0x00000008	/* illegal non-crypto mode */
#define ISES_BERR_IBCF		0x00000010	/* illegal value in BCFR */
#define ISES_BERR_reserved	0x00000020	/* reserved */
#define ISES_BERR_SRB		0x00000040	/* write SCU while busy */
#define ISES_BERR_HRB		0x00000080	/* write HU while busy */
#define ISES_BERR_IHFR		0x00000100	/* illegal value in HFR */
#define ISES_BERR_PADERR	0x00000200	/* padding error */
#define ISES_BERR_BIDM		0x00000400	/* B-interface input data
						   misalignment */
/* BCHCR (BCHU Control Register) */
#define ISES_BCHCR_BCHU_DIE	0x00000001	/* data input enabled */
#define ISES_BCHCR_BCHU_UE	0x00000002	/* unit enable */
#define ISES_BCHCR_BCHU_RST	0x00000004	/* BCHU reset */

/* 
 * OMR (Operation Method Register) 
 */
/* -- SELR (Selector Register) */
#define ISES_SELR_BCHU_EH	0x80000000	/* stop/continue on error */
#define ISES_SELR_BCHU_HISOF	0x01000000	/* HU input is SCU output */
#define ISES_SELR_BCHU_DIS	0x02000000	/* data interface select */

/* -- HOMR (HU Operation Mode Register) */
#define ISES_HOMR_HMTR		0x00800000	/* hash message type reg bit */
#define ISES_HOMR_ER		0x00300000	/* BE/LE, 2bit mask */

#define ISES_HOMR_HFR		0x00070000	/* Hash function mask, 3bits */
#define ISES_HOMR_HFR_NOP	0x00000000	/* NOP */
#define ISES_HOMR_HFR_MD5	0x00010000	/* MD5 */
#define ISES_HOMR_HFR_RMD160	0x00020000	/* RIPEMD-160 */
#define ISES_HOMR_HFR_RMD128	0x00030000	/* RIPEMD-128 */
#define ISES_HOMR_HFR_SHA1	0x00040000	/* SHA-1 */

/* -- SOMR (Symmetric crypto Operation Method Register) */
#define ISES_SOMR_BCFR		0x0000f000      /* block cipher function reg */
#define ISES_SOMR_BCPR		0x00000ff0	/* block cipher parameters */
#define ISES_SOMR_BOMR		(ISES_SOMR_BCFR | ISES_SOMR_BCPR)
#define ISES_SOMR_BOMR_NOP	0x00000000	/* NOP */
#define ISES_SOMR_BOMR_TRANSPARENT 0x00000010	/* Transparent */
#define ISES_SOMR_BOMR_DES	0x00001000	/* DES */
#define ISES_SOMR_BOMR_3DES2	0x00001010	/* 3DES-2 */
#define ISES_SOMR_BOMR_3DES	0x00001020	/* 3DES-3 */
#define ISES_SOMR_BOMR_SAFER	0x00002000	/* SAFER (actually more) */
#define ISES_SOMR_EDR		0x00000008	/* Encrypt/Decrypt register */
#define ISES_SOMR_FMR		0x00000003	/* feedback mode mask */
#define ISES_SOMR_FMR_ECB	0x00000000	/* EBC */
#define ISES_SOMR_FMR_CBC	0x00000001	/* CBC */
#define ISES_SOMR_FMR_CFB64	0x00000002	/* CFB64 */
#define ISES_SOMR_FMR_OFB64	0x00000003	/* OFB64 */

/* 
 * HRNG (Hardware Random Number Generator)
 */
#define ISES_OFFSET_HRNG_CTRL	0x00		/* Control register */
#define ISES_OFFSET_HRNG_LFSR	0x04		/* Linear feedback shift reg */
#define ISES_HRNG_CTRL_HE	0x00000001	/* HRNG enable */

/*
 * A-interface commands 
 */
#define ISES_MKCMD(cmd,len)	(cmd | cmd << 16 | len << 8 | len << 24)
#define ISES_CMD_NONE		-1

/*	Command name		Code	   Len	RLen    Desc                 */
#define ISES_CMD_CHIP_ID	0x00	/* 0	3	Read chipID */
/* LNAU commands - LNAU 1 */			
#define ISES_CMD_LRESET_1	0x01	/* 0	0	LNAU reset */
#define ISES_CMD_LRSFLG_1	0x02	/* 0	0	LNAU flags reset */
#define ISES_CMD_LUPLOAD_1	0x03	/* 0	64	Upload result */
#define ISES_CMD_LW_A_1		0x04	/* ?64	0	Load A register */
#define ISES_CMD_LW_B_1		0x05	/* ?64	0	Load B register */
#define ISES_CMD_LW_N_1		0x06	/* ?64	0	Load N register */
#define ISES_CMD_LW_Bq_1	0x07	/* ?32	0	Load Bq register */
#define ISES_CMD_LW_Nq_1	0x08	/* ?32	0	Load Nq register */
#define ISES_CMD_LW_Bp_1	0x09	/* ?34	0	Load Bp register */
#define ISES_CMD_LW_Np_1	0x0a	/* ?34	0	Load Np register */
#define ISES_CMD_LW_U_1		0x0b	/* ?34	0	Load U register */
#define ISES_CMD_LMOD_1		0x0c	/* 0	0	Start A % N */
#define ISES_CMD_LMULMOD_1	0x0d	/* 0	0	Start (A*B) % N */
#define ISES_CMD_LEXPMOD_1	0x0e	/* 0	0	Start (A^B) % N */
#define ISES_CMD_LEXPCRTMOD_1	0x0f	/* 0	0	Start (A^B)%N w/ CRT */
/* LNAU commands - LNAU 2 */
#define ISES_CMD_LRESET_2	0x10	/* 0	0	Reset */
#define ISES_CMD_LRSFLG_2	0x11	/* 0	0	Flags reset */
#define ISES_CMD_LUPLOAD_2	0x12	/* 0	64	Upload result */
#define ISES_CMD_LW_A_2		0x13	/* ?64	0	Load A register */
#define ISES_CMD_LW_B_2		0x14	/* ?64	0	Load B register */
#define ISES_CMD_LW_N_2		0x15	/* ?64	0	Load N register */
#define ISES_CMD_LW_Bq_2	0x16	/* ?32	0	Load Bq register */
#define ISES_CMD_LW_Nq_2	0x17	/* ?32	0	Load Nq register */
#define ISES_CMD_LW_Bp_2	0x18	/* ?34	0	Load Bp register */
#define ISES_CMD_LW_Np_2	0x19	/* ?34	0	Load Np register */
#define ISES_CMD_LW_U_2		0x1a	/* ?34	0	Load U register */
#define ISES_CMD_LMOD_2		0x1b	/* 0	0	Start A % N */
#define ISES_CMD_LMULMOD_2	0x1c	/* 0	0	Start (A*B) % N */
#define ISES_CMD_LEXPMOD_2	0x1d	/* 0	0	Start (A^B) % N */
#define ISES_CMD_LEXPCRTMOD_2	0x1e	/* 0	0	Start (A^B)%N w/ CRT */
/* BCHU commands */
#define ISES_CMD_RST_BERR	0x1f	/* 0	0	Reset BERR */
#define ISES_CMD_BR_BERR	0x20	/* 0	0	Read BERR */
#define ISES_CMD_BW_DATA	0x21	/* 2	0	Write DATA */
#define ISES_CMD_BR_DATA	0x22	/* 0	2	Read DATA */
#define ISES_CMD_BW_BCHCR	0x23	/* 1	0	Write BCHCR */
#define ISES_CMD_BR_BCHCR	0x24	/* 0	0	Read BCHCR */
#define ISES_CMD_BW_OMR		0x25	/* 1	0	Write OMR */
#define ISES_CMD_BR_OMR		0x26	/* 0	1	Read OMR */
#define ISES_CMD_BW_KR0		0x27	/* 2	0	Write key 0 */
#define ISES_CMD_BR_KR0		0x28	/* 0	2	Read key 0 */
#define ISES_CMD_BW_KR1		0x29	/* 2	0	Write key 1 */
#define ISES_CMD_BR_KR1		0x2a	/* 0	2	Read key 1 */
#define ISES_CMD_BW_KR2		0x2b	/* 2	0	Write key 2 */
#define ISES_CMD_BR_KR2		0x2c	/* 0	2	Read key 2 */
#define ISES_CMD_BW_SCCR	0x2d	/* 2	0	Write SCCR */
#define ISES_CMD_BR_SCCR	0x2e	/* 0	2	Read SCCR */
#define ISES_CMD_BW_DBCR	0x2f	/* 2	0	Write DBCR */
#define ISES_CMD_BR_DBCR	0x30	/* 0	2	Read DBCR */
#define ISES_CMD_BW_HMLR	0x31	/* 2	0	Write HMLR */
#define ISES_CMD_BR_HMLR	0x32	/* 0	2	Read HMLR */
#define ISES_CMD_BW_CVR		0x33	/* 5	0	Write CVR */
#define ISES_CMD_BR_CVR		0x34	/* 0	5	Read CVR */
#define ISES_CMD_BPROC		0x35	/* ?255	?255	Process data blocks */
#define ISES_CMD_BTERM		0x36	/* 0	0	Terminate session */
#define ISES_CMD_BSWITCH	0x37	/* 18	18	Switch BCHU session */
/* HRNG commands */
#define ISES_CMD_HSTART		0x38	/* 0	0	Start RBG unit */
#define ISES_CMD_HSTOP		0x39	/* 0	0	Stop RGB unit */
#define ISES_CMD_HSEED		0x3a	/* 1	0	Seed LFSR */
#define ISES_CMD_HBITS		0x3b	/* 1	?255	Return n*32 rnd bits */

/* Command return codes (RC) */
#define ISES_RC_MASK		0x0000ffff	
#define ISES_RC_SUCCESS		0x0000		/* success */
#define ISES_RC_CMDERR		0x0001		/* cmd interpretation error */
#define ISES_RC_QERR		0x0002		/* queue handling error */
#define ISES_RC_LNAU_ERR	0x0003		/* LNAU cmd proc error */
#define ISES_RC_BCHU_ERR	0x0004		/* BCHU cmd proc error */
#define ISES_RC_BCHU_BIFCSEL	0x0005		/* OMR says B-if, must be A */
#define ISES_RC_BCHU_ODD	0x0006		/* odd #words in param list */
#define ISES_RC_HRNG_ILLEN	0x0007		/* too large bitstream */

/* Interrupt bits, IRQE, IRQES, IRQEC, IRQSS, IRQ registers */
#define ISES_IRQ_TIMER_1	0x00000001	/* Timer 1 reached zero */
#define ISES_IRQ_TIMER_2	0x00000002	/* Timer 2 reached zero */
#define ISES_IRQ_I_IIN0		0x00000004	/* I-int 'Iin0' */
#define ISES_IRQ_I_IIN1		0x00000008	/* I-int 'Iin1' */
#define ISES_IRQ_I_IIN2		0x00000010	/* I-int 'Iin2' */
#define ISES_IRQ_I_IIN3		0x00000020	/* I-int 'Iin3' */
#define ISES_IRQ_LNAU_1_ERROR	0x00000040	/* LNAU 1 op error/abort */
#define ISES_IRQ_LNAU_1_DONE	0x00000080	/* LNAU 1 op done */
#define ISES_IRQ_LNAU_2_ERROR	0x00000100	/* LNAU 2 op error/abort */
#define ISES_IRQ_LNAU_2_DONE	0x00000200	/* LNAU 1 op done */
#define ISES_IRQ_BCHU_DONE	0x00000400	/* BCHU operation done */
#define ISES_IRQ_BCHU_ERROR	0x00000800	/* BCHU operation error/abrt */
#define ISES_IRQ_BCHU_IRF	0x00001000	/* BCHU input request flag >1*/
#define ISES_IRQ_BCHU_OAF	0x00002000	/* BCHU output avail flag >1 */
#define ISES_IRQ_BCHU_IEF	0x00004000	/* BCHU input empty flag >1  */
#define ISES_IRQ_A_WCTRL	0x00008000	/* A-int CTRL reg was written*/
#define ISES_IRQ_A_RSREQ	0x00010000	/* A-int SREQ reg was read   */
#define ISES_IRQ_A_DIQ		0x00020000	/* in queue emtpy, IQD write */
#define ISES_IRQ_A_CIQ		0x00040000	/* in queue has complete cmd */
#define ISES_IRQ_A_OQF		0x00080000	/* output queue full */

#define ISES_SESSION(sid)	( (sid) & 0x0fffffff)
#define	ISES_CARD(sid)		(((sid) & 0xf0000000) >> 28)
#define	ISES_SID(crd,ses)	(((crd) << 28) | ((ses) & 0x0fffffff))

/* Size and layout of ises_bchu_session is firmware dependent. */
/* This structure should be usable for the SWITCH_SESSION command. */
struct ises_bchu_session {
	u_int32_t	kr[6];		/* Key register KR2,KR1,KR0 */
	u_int32_t	omr;		/* Operation method register */

	/* The following values (on-chip) are cleared after an OMR write */
	u_int32_t	sccr[2];	/* Symm. crypto chaining reg. (IV) */
	u_int32_t	cvr[5];		/* Chaining variables reg. */
	u_int32_t	dbcr[2];	/* Data block count register */
	u_int32_t	hmlr[2];	/* Hash message length reg. */
} __attribute__((packed));

struct ises_pktbuf {
	volatile u_int32_t	pb_addr;	/* address of buffer start */
	volatile u_int32_t	pb_len;		/* packet length */
};

#define ISES_B_DATASIZE			4096
struct ises_databuf {
	  u_int8_t data[ISES_B_DATASIZE];
} __attribute__((packed));

/* 
 * ------------------------------------------
 * PCC-ISES Basic Functionality firmware v2.0 
 * ------------------------------------------
 */

/*
�* Copyright (C) 1999, 2000 Pijnenburg Custom Chips B.V.
�* All rights reserved.
�*
�* Redistribution and use in source and binary forms are permitted provided
�* that the following conditions are met:
�* 1. Redistribution of source code must retain the above copyright
�*��� notice, this list of conditions and the following disclaimer.
�* 2. Redistribution in binary form must reproduce the above copyright
�*��� notice, this list of conditions and the following disclaimer in the
�*��� documentation and/or other materials provided with the distribution.
�* 3. The name of the author may not be used to endorse or promote products
�*��� derived from this software without specific prior written permission
�*
�* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
�* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
�* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
�* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
�* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
�* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
�* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
�* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
�* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
�* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
�*/

#define ISES_BF_IDPLEN		0x00000403	/* Total length, 32bit words */
#define ISES_BF_IDPCRC		0x59DE7DEF	/* Firmware CRC */

static const u_int32_t ises_bf_fw[] = {
	0xEA000006, 0xE1A00000, 0xE1A00000, 0xE1A00000,
	0xE1A00000, 0xE1A00000, 0xEA000013, 0xEA000026,
	0xE59FD0C4, 0xE3A000D2, 0xE121F000, 0xE59FD0BC,
	0xE3A000D1, 0xE121F000, 0xE59FD0B0, 0xE3A000D3,
	0xE121F000, 0xEB00029B, 0xEAFFFFFE, 0xE10F1000,
	0xE1811000, 0xE121F001, 0xE12FFF1E, 0xE10F1000,
	0xE1C11000, 0xE121F001, 0xE12FFF1E, 0xE92D0003,
	0xE59F107C, 0xE5910004, 0xE5810000, 0xE3100902,
	0x0A00000B, 0xE59F106C, 0xE5910000, 0xE3500000,
	0x1A000003, 0xE59F1060, 0xE3A00001, 0xE5810000,
	0xEA000003, 0xE3500001, 0x1A000001, 0xE59F1038,
	0xE5810000, 0xE8BD0003, 0xE25EF004, 0xE25EF004,
	0xE3A000D3, 0xE121F000, 0xE59FE030, 0xE3A00000,
	0xE3A01032, 0xE48E0004, 0xE2511001, 0x1AFFFFFC,
	0xE91E3FFE, 0xE3A0E000, 0xE3A0F000, 0x000017B4,
	0x000017DC, 0xFFFFEA00, 0xFFFFE60C, 0xFFFFF000,
	0x00001714, 0xE3A01001, 0xE3A00AFE, 0xE2400601,
	0xE5A01624, 0xE1A0F00E, 0xE3A00AFE, 0xE2400601,
	0xE5901620, 0xE3510000, 0x0AFFFFFC, 0xE5900610,
	0xE1A0F00E, 0xE3A02000, 0xE3510000, 0x91A0F00E,
	0xE3A03AFE, 0xE2433601, 0xE593C620, 0xE35C0000,
	0x0AFFFFFC, 0xE593C610, 0xE2822001, 0xE1520001,
	0xE480C004, 0x3AFFFFF7, 0xE1A0F00E, 0xE3A01000,
	0xE3A00AFE, 0xE2400601, 0xE5A01618, 0xE1A0F00E,
	0xE3A01002, 0xE3A00AFE, 0xE2400601, 0xE5A01624,
	0xE1A0F00E, 0xE3A01AFE, 0xE2411601, 0xE591261C,
	0xE3520000, 0x1A000006, 0xE3A02506, 0xE5812600,
	0xE591261C, 0xE3520000, 0x0AFFFFFC, 0xE3A02401,
	0xE5812600, 0xE5A10610, 0xE1A0F00E, 0xE3A03000,
	0xE3510000, 0x91A0F00E, 0xE92D4010, 0xE3A0E506,
	0xE3A0C401, 0xE3A02AFE, 0xE2422601, 0xE592461C,
	0xE3540000, 0x1A000004, 0xE582E600, 0xE592461C,
	0xE3540000, 0x0AFFFFFC, 0xE582C600, 0xE4904004,
	0xE5824610, 0xE2833001, 0xE1530001, 0x3AFFFFF2,
	0xE8BD8010, 0xE3A01000, 0xE3A00AFE, 0xE2400601,
	0xE5A01614, 0xE3A00CE6, 0xE2400801, 0xE5901000,
	0xE3811302, 0xE5801000, 0xE5901000, 0xE3C11302,
	0xE5801000, 0xE1A0F00E, 0xE59F2038, 0xE59F0038,
	0xE5901000, 0xE0821181, 0xE5D11006, 0xE20120F0,
	0xE3A03BC2, 0xE2433701, 0xE0832402, 0xE5802008,
	0xE2822004, 0xE580200C, 0xE2432B02, 0xE0821401,
	0xE5A01010, 0xE1A0F00E, 0x00000E00, 0x00001000,
	0xE92D4000, 0xEBFFFFB0, 0xE3A01002, 0xE59F0028,
	0xE5801000, 0xE3A01AFE, 0xE2411601, 0xE5912204,
	0xE5802004, 0xE5911208, 0xE5801008, 0xE3A01003,
	0xEB000264, 0xE3A00000, 0xE8BD8000, 0x00001014,
	0xE92D4000, 0xEBFFFFDB, 0xEBFFFF9F, 0xE51F005C,
	0xE51F205C, 0xE5921000, 0xE0800181, 0xE5D01007,
	0xE3510011, 0x1A000004, 0xE51F0074, 0xE5900008,
	0xE5903000, 0xE3130008, 0x0AFFFFFC, 0xE5B2000C,
	0xE5801000, 0xE3A00000, 0xEB000247, 0xE3A00000,
	0xE8BD8000, 0xE92D4000, 0xEBFFFFC6, 0xEBFFFF8A,
	0xE51F10AC, 0xE5910008, 0xE5902000, 0xE3120008,
	0x0AFFFFFC, 0xE5900000, 0xE3100002, 0x0A000002,
	0xE3A00003, 0xEB000238, 0xEA000002, 0xE5B10010,
	0xE3A01040, 0xEB00023B, 0xE3A00000, 0xE8BD8000,
	0xE92D4010, 0xEBFFFFB3, 0xE51F40F4, 0xE5940008,
	0xE5901000, 0xE3110008, 0x0AFFFFFC, 0xE5900000,
	0xE3100002, 0x0A000005, 0xE5B41004, 0xE51F00D8,
	0xEBFFFF5F, 0xEBFFFF6C, 0xE3A00003, 0xEA00000B,
	0xE5940010, 0xE5941004, 0xEBFFFF59, 0xEBFFFF66,
	0xE51F0140, 0xE5941000, 0xE0800181, 0xE5D02007,
	0xE5B41004, 0xE594000C, 0xEB00024E, 0xE3A00000,
	0xEB000215, 0xE3A00000, 0xE8BD8010, 0xE92D4010,
	0xEBFFFF94, 0xEBFFFF58, 0xE51F0174, 0xE5901008,
	0xE5912000, 0xE3120008, 0x0AFFFFFC, 0xE5911000,
	0xE3110002, 0xE3A04000, 0x13A00003, 0x1A00000F,
	0xE51F11A0, 0xE5902000, 0xE0812182, 0xE5D22007,
	0xE3520034, 0x1A000003, 0xE5902010, 0xE5224088,
	0xE5902010, 0xE5224084, 0xE5902000, 0xE0811182,
	0xE5D11007, 0xE590000C, 0xE5801000, 0xE3A00000,
	0xEB0001F5, 0xE1A00004, 0xE8BD8010, 0xE92D4010,
	0xEBFFFF39, 0xE3A04000, 0xE3A00AFE, 0xE2400601,
	0xE5A04400, 0xE3A00000, 0xEB0001EB, 0xE1A00004,
	0xE8BD8010, 0xE92D4010, 0xE51F01D4, 0xE1A04000,
	0xE3A01002, 0xEBFFFF1E, 0xEBFFFF2B, 0xE3A00AFE,
	0xE2400601, 0xE5901474, 0xE3110002, 0x1A00000F,
	0xE5901424, 0xE3110402, 0x13A00005, 0x1A00000E,
	0xE3A01003, 0xE5801470, 0xE5901474, 0xE3110020,
	0x0AFFFFFC, 0xE5941000, 0xE5801478, 0xE5B41004,
	0xE5801478, 0xE5900474, 0xE3100002, 0x0A000001,
	0xE3A00004, 0xEA000000, 0xE3A00000, 0xEB0001CA,
	0xE3A00000, 0xE8BD8010, 0xE92D4000, 0xEBFFFF0E,
	0xE3A00AFE, 0xE2400601, 0xE5901474, 0xE3110002,
	0xE51F126C, 0x1A00000D, 0xE5902424, 0xE3120402,
	0x13A00005, 0x1A00000C, 0xE5902474, 0xE3120040,
	0x0AFFFFFC, 0xE5902478, 0xE5812000, 0xE5902478,
	0xE5812004, 0xE5900474, 0xE3100002, 0x0A000001,
	0xE3A00004, 0xEA000000, 0xE3A00000, 0xE3500000,
	0x1A000003, 0xE1A00001, 0xE3A01002, 0xEB0001B1,
	0xEA000000, 0xEB0001A8, 0xE3A00000, 0xE8BD8000,
	0xE92D40F0, 0xE51F6320, 0xE5961004, 0xE51F02E8,
	0xE1A04000, 0xEBFFFEDA, 0xEBFFFEE7, 0xE3A05AFE,
	0xE2455601, 0xE3A07000, 0xE5960000, 0xE3500023,
	0x1A000004, 0xE5941000, 0xE3110004, 0x13A00004,
	0x15A50470, 0x1A000018, 0xE3500023, 0x13500031,
	0x0A000002, 0xE5951474, 0xE3110001, 0x1AFFFFFC,
	0xE3500025, 0x1A000003, 0xE5940000, 0xE3C0033F,
	0xE5840000, 0xE5857470, 0xE5960004, 0xE1A02100,
	0xE51F03A0, 0xE5961000, 0xE0800181, 0xE5D00006,
	0xE2400B07, 0xE1A01004, 0xEB0001D0, 0xE5B50474,
	0xE3100002, 0x13A00004, 0x1A000000, 0xE3A00000,
	0xEB000179, 0xE1A00007, 0xE8BD80F0, 0xE92D40F0,
	0xEBFFFEBD, 0xE3A06AFE, 0xE2466601, 0xE5960474,
	0xE3100002, 0xE51F73B0, 0xE51F43F4, 0x15940000,
	0x13500020, 0x1A000014, 0xE5940000, 0xE350002E,
	0x13500030, 0x13500034, 0x1A000002, 0xE5961474,
	0xE3110001, 0x1AFFFFFC, 0xE51F1428, 0xE0810180,
	0xE5D05007, 0xE1A02105, 0xE5D00006, 0xE2401B07,
	0xE1A00007, 0xEB0001AD, 0xE5B60474, 0xE3100002,
	0x15940000, 0x13500020, 0x0A000001, 0xE3A00004,
	0xEA000000, 0xE3A00000, 0xE3500000, 0x1A000003,
	0xE1A01005, 0xE1A00007, 0xEB000156, 0xEA000000,
	0xEB00014D, 0xE3A00000, 0xE8BD80F0, 0xE92D41F0,
	0xE3A04000, 0xE3A00003, 0xE3A08AFE, 0xE2488601,
	0xE5880470, 0xE5980474, 0xE3100002, 0xE51F54A8,
	0x13A04004, 0x1A000006, 0xE5950004, 0xE3100001,
	0x13A04006, 0x1A000002, 0xE5980424, 0xE3100402,
	0x13A04005, 0xE51F7490, 0xE3540000, 0x0A000004,
	0xE1A00007, 0xE5951004, 0xEBFFFE6D, 0xEBFFFE7A,
	0xEA00001D, 0xE3A06000, 0xE5950004, 0xE3500000,
	0xDA000015, 0xE5980474, 0xE3100020, 0x0AFFFFFC,
	0xEBFFFE5C, 0xE5880478, 0xEBFFFE5A, 0xE5880478,
	0xE5980474, 0xE3100004, 0x1A000007, 0xE5980474,
	0xE3100040, 0x0AFFFFFC, 0xE5980478, 0xE7870106,
	0xE5980478, 0xE0871106, 0xE5A10004, 0xE2866002,
	0xE5950004, 0xE1560000, 0xBAFFFFE9, 0xEBFFFE5E,
	0xE5980474, 0xE3100002, 0x13A04004, 0xE3540000,
	0x05B80474, 0x03100004, 0x1A000003, 0xE1A00007,
	0xE5B51004, 0xEB000113, 0xEA000001, 0xE1A00004,
	0xEB000109, 0xE3A00000, 0xE8BD81F0, 0xE92D4010,
	0xEBFFFE4D, 0xE3A00AFE, 0xE2400601, 0xE5901474,
	0xE3110002, 0xE3A04000, 0x1A000013, 0xE5901424,
	0xE3110402, 0x0A000003, 0xE5901474, 0xE3110C02,
	0x0AFFFFFC, 0xEA000002, 0xE5901474, 0xE3110020,
	0x0AFFFFFC, 0xE5901470, 0xE3C11001, 0xE5801470,
	0xE5901474, 0xE3110001, 0x1AFFFFFC, 0xE5804470,
	0xE5900474, 0xE3100002, 0x0A000001, 0xE3A00004,
	0xEA000000, 0xE3A00000, 0xEB0000E7, 0xE1A00004,
	0xE8BD8010, 0xE92D4070, 0xE3A00AFE, 0xE2400601,
	0xE5901474, 0xE3110002, 0xE51F55F4, 0xE3A06000,
	0x0A000005, 0xE3A04004, 0xE1A00005, 0xE3A01012,
	0xEBFFFE13, 0xEBFFFE20, 0xEA000027, 0xE5901424,
	0xE3110402, 0x0A000003, 0xE5901474, 0xE3110C02,
	0x0AFFFFFC, 0xEA000002, 0xE5901474, 0xE3110020,
	0x0AFFFFFC, 0xE5901470, 0xE3C11001, 0xE5801470,
	0xE5901474, 0xE3110001, 0x1AFFFFFC, 0xE1A04000,
	0xE5806470, 0xE1A00005, 0xE3A02048, 0xE3A0100C,
	0xE2411B07, 0xEB000111, 0xE3A01006, 0xE3A0000C,
	0xE2400B07, 0xEBFFFDF6, 0xEBFFFDEE, 0xE3C0033F,
	0xE5840424, 0xE3A0100B, 0xE3A00028, 0xE2400B07,
	0xEBFFFDEF, 0xEBFFFDFC, 0xE3A00003, 0xE5840470,
	0xE5B40474, 0xE2104002, 0x13A04004, 0xE3540000,
	0x1A000003, 0xE1A00005, 0xE3A01012, 0xEB0000B1,
	0xEA000001, 0xE1A00004, 0xEB0000A7, 0xE1A00006,
	0xE8BD8070, 0xE92D4000, 0xEBFFFDEB, 0xE51F172C,
	0xE51F072C, 0xE5900000, 0xE0810180, 0xE5D00007,
	0xE3A01AFE, 0xE2411601, 0xE5A10C00, 0xE3A00000,
	0xEB000099, 0xE3A00000, 0xE8BD8000, 0xE92D4000,
	0xEBFFFDC8, 0xE3A01AFE, 0xE2411601, 0xE5A10C04,
	0xEBFFFDD9, 0xE3A00000, 0xEB00008F, 0xE3A00000,
	0xE8BD8000, 0xE92D41F0, 0xEBFFFDBE, 0xE1A04000,
	0xEBFFFDD1, 0xE3A06000, 0xE35400FF, 0x9A000002,
	0xE3A00007, 0xEB000084, 0xEA00001C, 0xE3A00000,
	0xE51F376C, 0xE3540000, 0x9A000015, 0xE3A07064,
	0xE3A0C080, 0xE3A02AFE, 0xE2422601, 0xE3A0E001,
	0xE3A01000, 0xE5827800, 0xE582C808, 0xE5928A00,
	0xE3180001, 0x0AFFFFFC, 0xE582EA00, 0xE5928C04,
	0xE20880FF, 0xE0885405, 0xE2811001, 0xE3510004,
	0x3AFFFFF3, 0xE7835100, 0xE2800001, 0xE1500004,
	0x3AFFFFEE, 0xE1A01004, 0xE1A00003, 0xEB00006D,
	0xE1A00006, 0xE8BD81F0, 0xE92D47F0, 0xEB0000A1,
	0xE3A09004, 0xE3A08AFE, 0xE2488601, 0xE3A07010,
	0xE1A06088, 0xE2485A01, 0xE3500000, 0x0A000001,
	0xEB000084, 0xEA000015, 0xEB000082, 0xE5889470,
	0xE3A0A000, 0xE59F4040, 0xEBFFFD81, 0xEBFFFD9F,
	0xE3A00001, 0xEB000014, 0xEB000019, 0xE3500002,
	0x0A00000A, 0xE5867804, 0xE5857804, 0xE5889470,
	0xE3A00002, 0xEB00000C, 0xE584A000, 0xE5940000,
	0xE3500000, 0x0AFFFFFC, 0xEAFFFFEE, 0x000017B4,
	0xE5A67804, 0xE5A57804, 0xE5A89470, 0xE3A00003,
	0xEB000001, 0xEAFFFFFF, 0xEAFFFFFE, 0xE1A00C00,
	0xE3A01AFE, 0xE2411601, 0xE5A10600, 0xE1A0F00E,
	0xE1A0F00E, 0xE92D43F0, 0xE3A07001, 0xE3A09AFE,
	0xE2499601, 0xE3A080FF, 0xE59F6030, 0xE59F5030,
	0xE59F4030, 0xE5990618, 0xE3500C01, 0x0A000005,
	0xE5990618, 0xE3500000, 0x9A000007, 0xE5990620,
	0xE3500000, 0x1A000004, 0xE3A00002, 0xEA000019,
	0x00001004, 0x00001000, 0x00000E00, 0xE5990618,
	0xE3500000, 0x0AFFFFFC, 0xEBFFFD4E, 0xE20010FF,
	0xE0082420, 0xE5851000, 0xE1A03800, 0xE1A03823,
	0xE1530820, 0xE5862000, 0x1A000007, 0xE351003E,
	0xD0843181, 0xD5D3C004, 0xD15C0002, 0xCA000002,
	0xE5D33005, 0xE1530002, 0xAA000006, 0xE1A00820,
	0xE1A00800, 0xE3800001, 0xEBFFFD59, 0xEBFFFD7C,
	0xE1A00007, 0xE8BD83F0, 0xE1A0E00F, 0xE794F181,
	0xE3500000, 0x0AFFFFD2, 0xE8BD83F0, 0xE92D4000,
	0xE51F1094, 0xE5911000, 0xE1800801, 0xEBFFFD4C,
	0xE8BD4000, 0xEAFFFD6E, 0xE92D4030, 0xE1A05000,
	0xE1A04001, 0xE1A00C01, 0xE51F10BC, 0xE5911000,
	0xE1800801, 0xEBFFFD42, 0xE1A01004, 0xE1A00005,
	0xEBFFFD4D, 0xE8BD4030, 0xEAFFFD61, 0xE59F1020,
	0xE5910000, 0xE59F201C, 0xE1500002, 0x21A0F00E,
	0xE5922000, 0xE5802000, 0xE3E00000, 0xE5810000,
	0xE1A0F00E, 0x000017FC, 0x000017F8, 0xE92D4010,
	0xE1A04000, 0xEB000010, 0xE3540018, 0x03A00080,
	0x0A000002, 0xE354001C, 0x18BD8010, 0xE3A00040,
	0xE8BD4010, 0xEAFFFCD8, 0xE92D4000, 0xE3A01001,
	0xE3A00018, 0xEBFFFFF0, 0xE3A01902, 0xE3A00AFE,
	0xE2400601, 0xE5A01A08, 0xE8BD8000, 0xE3A000C0,
	0xEAFFFCC9, 0xE1A0F00E, 0xE3A03000, 0xE1510002,
	0x21A0F00E, 0xE7803101, 0xE2811001, 0xE1510002,
	0x3AFFFFFB, 0xE1A0F00E, 0xE3A00000, 0xE1A0F00E,
	0xE3A00001, 0xE1A0F00E, 0xE3A00002, 0xE1A0F00E,
	0xE92D4000, 0xE59F001C, 0xE5901000, 0xE59F0018,
	0xEBFFFCEF, 0xEBFFFCFC, 0xE3A00011, 0xEBFFFFB2,
	0xE3A00000, 0xE8BD8000, 0x00001004, 0x00001014,
	0xE1803001, 0xE1833002, 0xE3130003, 0x1A00000B,
	0xE1A0C122, 0xE1A03000, 0xE24C2001, 0xE35C0000,
	0x91A0F00E, 0xE491C004, 0xE483C004, 0xE1A0C002,
	0xE2422001, 0xE35C0000, 0x8AFFFFF9, 0xE1A0F00E,
	0xE1A03000, 0xE1A0C002, 0xE2422001, 0xE35C0000,
	0x91A0F00E, 0xE4D1C001, 0xE4C3C001, 0xE1A0C002,
	0xE2422001, 0xE35C0000, 0x8AFFFFF9, 0xE1A0F00E,
	0x000002A0, 0x00000000, 0x000002E0, 0x10C80000,
	0x000002E0, 0x11C80000, 0x00000334, 0x00C00000,
	0x00000380, 0x40C04000, 0x00000380, 0x40C14000,
	0x00000380, 0x40C24000, 0x00000380, 0x20C52000,
	0x00000380, 0x20C72000, 0x00000380, 0x22C42200,
	0x00000380, 0x22C62200, 0x00000380, 0x22C32200,
	0x000003FC, 0x34C80000, 0x000003FC, 0x35C80000,
	0x000003FC, 0x36C80000, 0x000003FC, 0x37C80000,
	0x000002E0, 0x10D80000, 0x000002E0, 0x11D80000,
	0x00000334, 0x00D00000, 0x00000380, 0x40D04000,
	0x00000380, 0x40D14000, 0x00000380, 0x40D24000,
	0x00000380, 0x20D52000, 0x00000380, 0x20D72000,
	0x00000380, 0x22D42200, 0x00000380, 0x22D62200,
	0x00000380, 0x22D32200, 0x000003FC, 0x34D80000,
	0x000003FC, 0x35D80000, 0x000003FC, 0x36D80000,
	0x000003FC, 0x37D80000, 0x0000047C, 0x00000000,
	0x0000066C, 0x01000000, 0x000004A4, 0x00000202,
	0x00000528, 0x00000000, 0x000005B0, 0x01700101,
	0x0000066C, 0x01700000, 0x000005B0, 0x01240101,
	0x0000066C, 0x01240000, 0x000005B0, 0x021C0202,
	0x0000066C, 0x021C0000, 0x000005B0, 0x02140202,
	0x0000066C, 0x02140000, 0x000005B0, 0x020C0202,
	0x0000066C, 0x020C0000, 0x000005B0, 0x02280202,
	0x0000066C, 0x02280000, 0x000005B0, 0x02440202,
	0x0000066C, 0x02440000, 0x000005B0, 0x024C0202,
	0x0000066C, 0x024C0000, 0x000005B0, 0x05300505,
	0x0000066C, 0x05300000, 0x0000071C, 0x0000FF00,
	0x0000082C, 0x00000000, 0x000008B4, 0x00001212,
	0x000009B4, 0x01000000, 0x000009B4, 0x00000000,
	0x000009EC, 0x00000101, 0x00000A14, 0x00000101,
	0x00000D58, 0x00004040, 0x00000D60, 0x00002020,
	0x00000D60, 0x0000FF00, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000
};
