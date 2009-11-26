/*	$OpenBSD: src/sys/arch/loongson/dev/bonitoreg.h,v 1.1.1.1 2009/11/26 15:50:07 miod Exp $	*/
/*	$NetBSD: bonitoreg.h,v 1.6 2005/12/24 20:07:19 perry Exp $	*/

/*
 * The Loongson PCI Northbridge and memory controller is a derivative
 * of the Bonito chip.
 *
 * This file is a stripped-down version of the Bonito layout, containing
 * only definitions applying to the Loongson chip.
 */

/*
 * Bonito Register Map
 * Copyright (c) 1999 Algorithmics Ltd
 *
 * Algorithmics gives permission for anyone to use and modify this file
 * without any obligation or license condition except that you retain
 * this copyright message in any source redistribution in whole or part.
 *
 * Updated copies of this and other files can be found at
 * ftp://ftp.algor.co.uk/pub/bonito/
 *
 * Users of the Bonito controller are warmly recommended to contribute
 * any useful changes back to Algorithmics (mail to
 * bonito@algor.co.uk).
 */

/* Revision 1.48 autogenerated on 08/17/99 15:20:01 */

#ifndef	_BONITOREG_H_
#define	_BONITOREG_H_

#define BONITO(x)	(BONITO_REG_BASE + (x))

#define	REGVAL(x)	*((volatile u_int32_t *)PHYS_TO_XKPHYS(x, CCA_NC))

#define BONITO_FLASH_BASE		0x1c000000
#define BONITO_FLASH_SIZE		0x02000000
#define BONITO_FLASH_TOP		(BONITO_FLASH_BASE+BONITO_FLASH_SIZE-1)

#define BONITO_BOOT_BASE		0x1fc00000
#define BONITO_BOOT_SIZE		0x00100000
#define BONITO_BOOT_TOP 		(BONITO_BOOT_BASE+BONITO_BOOT_SIZE-1)
#define BONITO_REG_BASE 		0x1fe00000
#define BONITO_REG_SIZE 		0x00040000
#define BONITO_REG_TOP			(BONITO_REG_BASE+BONITO_REG_SIZE-1)

#define BONITO_PCILO_BASE		0x10000000
#define BONITO_PCILO_SIZE		0x0c000000
#define BONITO_PCILO_TOP		(BONITO_PCILO_BASE+BONITO_PCILO_SIZE-1)
#define BONITO_PCILO0_BASE		0x10000000
#define BONITO_PCILO1_BASE		0x14000000
#define BONITO_PCILO2_BASE		0x18000000
#define BONITO_PCIHI_BASE		0x20000000
#define BONITO_PCIHI_SIZE		0x20000000
#define BONITO_PCIHI_TOP		(BONITO_PCIHI_BASE+BONITO_PCIHI_SIZE-1)
#define BONITO_PCIIO_BASE		0x1fd00000
#define BONITO_PCIIO_SIZE		0x00100000
#define BONITO_PCIIO_TOP		(BONITO_PCIIO_BASE+BONITO_PCIIO_SIZE-1)
#define BONITO_PCICFG_BASE		0x1fe80000
#define BONITO_PCICFG_SIZE		0x00080000
#define BONITO_PCICFG_TOP		(BONITO_PCICFG_BASE+BONITO_PCICFG_SIZE-1)

/* Bonito Register Bases */

#define BONITO_PCICONFIGBASE		0x00
#define BONITO_REGBASE			0x100


/* PCI Configuration  Registers */

#define BONITO_PCI_REG(x)		BONITO(BONITO_PCICONFIGBASE + (x))
#define BONITO_PCIDID			BONITO_PCI_REG(0x00)
#define BONITO_PCICMD			BONITO_PCI_REG(0x04)
#define BONITO_PCICLASS 		BONITO_PCI_REG(0x08)
#define BONITO_PCILTIMER		BONITO_PCI_REG(0x0c)
#define BONITO_PCIBASE0 		BONITO_PCI_REG(0x10)
#define BONITO_PCIBASE1 		BONITO_PCI_REG(0x14)
#define BONITO_PCIBASE2 		BONITO_PCI_REG(0x18)
#define BONITO_PCIEXPRBASE		BONITO_PCI_REG(0x30)
#define BONITO_PCIINT			BONITO_PCI_REG(0x3c)

#define BONITO_PCICMD_PERR_CLR		0x80000000
#define BONITO_PCICMD_SERR_CLR		0x40000000
#define BONITO_PCICMD_MABORT_CLR	0x20000000
#define BONITO_PCICMD_MTABORT_CLR	0x10000000
#define BONITO_PCICMD_TABORT_CLR	0x08000000
#define BONITO_PCICMD_MPERR_CLR 	0x01000000
#define BONITO_PCICMD_PERRRESPEN	0x00000040
#define BONITO_PCICMD_ASTEPEN		0x00000080
#define BONITO_PCICMD_SERREN		0x00000100
#define BONITO_PCILTIMER_BUSLATENCY	0x0000ff00
#define BONITO_PCILTIMER_BUSLATENCY_SHIFT	8


#define	BONITO_REV_FPGA(x)		((x) & 0x80)
#define	BONITO_REV_MAJOR(x)		(((x) >> 4) & 0x7)
#define	BONITO_REV_MINOR(x)		((x) & 0xf)


/* Bonito configuration */

#define BONITO_BONGENCFG_OFFSET		0x4
#define BONITO_BONGENCFG		BONITO(BONITO_REGBASE + BONITO_BONGENCFG_OFFSET)

#define BONITO_BONGENCFG_DEBUGMODE	0x00000001
#define BONITO_BONGENCFG_SNOOPEN	0x00000002
#define BONITO_BONGENCFG_CPUSELFRESET	0x00000004

#define BONITO_BONGENCFG_FORCE_IRQA	0x00000008
#define BONITO_BONGENCFG_IRQA_ISOUT	0x00000010
#define BONITO_BONGENCFG_IRQA_FROM_INT1 0x00000020
#define BONITO_BONGENCFG_BYTESWAP	0x00000040

#define BONITO_BONGENCFG_UNCACHED	0x00000080
#define BONITO_BONGENCFG_PREFETCHEN	0x00000100
#define BONITO_BONGENCFG_WBEHINDEN	0x00000200
#define BONITO_BONGENCFG_CACHEALG	0x00000c00
#define BONITO_BONGENCFG_CACHEALG_SHIFT 10
#define BONITO_BONGENCFG_PCIQUEUE	0x00001000
#define BONITO_BONGENCFG_CACHESTOP	0x00002000
#define BONITO_BONGENCFG_MSTRBYTESWAP	0x00004000
#define BONITO_BONGENCFG_BUSERREN	0x00008000
#define BONITO_BONGENCFG_NORETRYTIMEOUT 0x00010000
#define BONITO_BONGENCFG_SHORTCOPYTIMEOUT 0x00020000

/* PCI address map control */

#define BONITO_PCIMAP			BONITO(BONITO_REGBASE + 0x10)
#define BONITO_PCIMEMBASECFG		BONITO(BONITO_REGBASE + 0x14)
#define BONITO_PCIMAP_CFG		BONITO(BONITO_REGBASE + 0x18)

/* GPIO Regs - r/w */

#define BONITO_GPIODATA_OFFSET 		0x1c
#define BONITO_GPIODATA 		BONITO(BONITO_REGBASE + BONITO_GPIODATA_OFFSET)
#define BONITO_GPIOIE			BONITO(BONITO_REGBASE + 0x20)

/* ICU Configuration Regs - r/w */

#define BONITO_INTEDGE			BONITO(BONITO_REGBASE + 0x24)
#define BONITO_INTSTEER 		BONITO(BONITO_REGBASE + 0x28)
#define BONITO_INTPOL			BONITO(BONITO_REGBASE + 0x2c)

/* ICU Enable Regs - IntEn & IntISR are r/o. */

#define BONITO_INTENSET 		BONITO(BONITO_REGBASE + 0x30)
#define BONITO_INTENCLR 		BONITO(BONITO_REGBASE + 0x34)
#define BONITO_INTEN			BONITO(BONITO_REGBASE + 0x38)
#define BONITO_INTISR			BONITO(BONITO_REGBASE + 0x3c)

/* PCI_Hit*_Sel_* */

#define	LOONGSON_PCI_HIT0_SEL_L		BONITO(BONITO_REGBASE + 0x50)
#define	LOONGSON_PCI_HIT0_SEL_H		BONITO(BONITO_REGBASE + 0x54)
#define	LOONGSON_PCI_HIT1_SEL_L		BONITO(BONITO_REGBASE + 0x58)
#define	LOONGSON_PCI_HIT1_SEL_H		BONITO(BONITO_REGBASE + 0x5c)
#define	LOONGSON_PCI_HIT2_SEL_L		BONITO(BONITO_REGBASE + 0x60)
#define	LOONGSON_PCI_HIT2_SEL_H		BONITO(BONITO_REGBASE + 0x64)

/* ###### Bit Definitions for individual Registers #### */

/* pcimap */

#define BONITO_PCIMAP_PCIMAP_LO0	0x0000003f
#define BONITO_PCIMAP_PCIMAP_LO0_SHIFT	0
#define BONITO_PCIMAP_PCIMAP_LO1	0x00000fc0
#define BONITO_PCIMAP_PCIMAP_LO1_SHIFT	6
#define BONITO_PCIMAP_PCIMAP_LO2	0x0003f000
#define BONITO_PCIMAP_PCIMAP_LO2_SHIFT	12
#define BONITO_PCIMAP_PCIMAP_2		0x00040000
#define BONITO_PCIMAP_WIN(WIN,ADDR)	((((ADDR)>>26) & BONITO_PCIMAP_PCIMAP_LO0) << ((WIN)*6))

#define BONITO_PCIMAP_WINSIZE		(1<<26)
#define BONITO_PCIMAP_WINOFFSET(ADDR)	((ADDR) & (BONITO_PCIMAP_WINSIZE - 1))
#define BONITO_PCIMAP_WINBASE(ADDR)	((ADDR) << 26)

/* PCIMAP Cfg */

#define BONITO_PCIMAPCFG_TYPE1		0x00010000

#endif	/* _BONITOREG_H_ */
