/*	$OpenBSD: src/sys/arch/vax/vsa/lcgreg.h,v 1.3 2006/08/22 21:05:03 miod Exp $	*/
/* $NetBSD: lcgreg.h,v 1.4 2005/12/11 12:19:34 christos Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * The registers of the LCG used in the VS4000/60 and VS4000/VLC.
 * All relative to 0x20100000
 */

/* Memory Control, Flow Control, Configuration Registers
 */
#define	LCG_REG_MEM_CONFIG		0x001800
#define	LCG_REG_MEM_STATUS		0x001804
#define	LCG_REG_MEM_CURRENT_STATE	0x001808
#define	LCG_REG_MEM_ERROR		0x00180c
#define	LCG_REG_SLOW_CONTROL_STATUS	0x001810

/* Video Control Registers
 */
#define	LCG_REG_VIDEO_CONFIG		0x001e00
#define	VIDEO_VSTATE				0xc0000000
#define	VIDEO_VFRONT_PORCH			0x00000000
#define	VIDEO_VSYNC				0x40000000
#define	VIDEO_VBACK_PORCH			0x80000000
#define	VIDEO_VACTIVE				0xc0000000
#define	VIDEO_HSTATE				0x30000000
#define	VIDEO_HFRONT_PORCH			0x00000000
#define	VIDEO_HSYNC				0x10000000
#define	VIDEO_HBACK_PORCH			0x20000000
#define	VIDEO_HACTIVE				0x30000000
#define	VIDEO_CONSOLE_LUT			0x02000000
#define	VIDEO_CONTROL_LUT			0x01000000
#define	VIDEO_CURSOR_ACTIVE			0x00400000
#define	VIDEO_CURSOR_SCANLINE			0x003f0000
#define	VIDEO_RESET				0x00008000
#define	VIDEO_LUT_LOAD_SIZE			0x00002000
#define	VIDEO_SYNC_ENABLE			0x00001000
#define	VIDEO_LUT_SHIFT_SEL			0x00000800
#define	VIDEO_CLOCK_SEL				0x00000400
#define	VIDEO_MEM_REFRESH_SEL_MASK		0x00000300
#define	VIDEO_MEM_REFRESH_SEL_SHIFT	8
#define	VIDEO_REFRESH_SEL			0x000000c0
#define	VIDEO_SHIFT_SEL				0x00000020
#define	VIDEO_CURSOR_PIN_TYPE			0x00000010
#define	VIDEO_LUT_LOAD_ENABLE			0x00000008
#define	VIDEO_CURSOR_ENABLE			0x00000004
#define	VIDEO_ENABLE_VIDEO			0x00000002
#define	VIDEO_TIMING_ENABLE			0x00000001
#define	LCG_REG_VIDEO_HTIMING		0x001e10
#define	LCG_REG_VIDEO_VTIMING		0x001e14
#define	LCG_REG_VIDEO_TIMING		0x001e18
#define	LCG_REG_VIDEO_X			0x000e30
#define	LCG_REG_VIDEO_Y			0x000e30
#define	LCG_REG_VIDEO_REFRESH_BASE	0x000e34
#define	LCG_REG_VIDEO_REFRESH_SHIFT	0x000e40
#define	LCG_REG_VIDEO_LUT_LOAD_COUNT	0x000e40
#define	LCG_REG_CURSOR_SCANLINE_LW0	0x000e50
#define	LCG_REG_CURSOR_SCANLINE_LW1	0x000e54
#define	LCG_REG_CURSOR_SCANLINE_LW2	0x000e58
#define	LCG_REG_CURSOR_SCANLINE_LW3	0x000e5c
#define	LCG_REG_CURSOR_BASE		0x000e80
#define	LCG_REG_CURSOR_XY		0x000e84
#define	LCG_REG_CURSOR_X		0x000e84
#define	LCG_REG_CURSOR_Y		0x000e84
#define	LCG_REG_LUT_CONSOLE_SEL		0x000ee0
#define	LUT_SEL_CONSOLE				0x00000000
#define	LUT_SEL_COLOR				0x00000001
#define	LCG_REG_LUT_COLOR_BASE_W	0x0006e4
#define	LCG_REG_LUT_COLOR_BASE_R	0x0006e4
#define	LCG_REG_LUT_CONTROL_BASE	0x000ee8
#define	LCG_REG_VIDEO_COUNTER_TEST	0x000f00
#define	LCG_REG_MEM_REFRESH_BASE	0x000f04

/* Graphics Control and VM Registers
 */
#define	LCG_REG_LCG_GO			0x000c80
#define	GO_VM					0x00000008
#define	GO_AG					0x00000002
#define	GO_FIFO					0x00000001
#define	LCG_REG_NEXT_ADDRESS		0x001334
#define	LCG_REG_PA_SPTE_PTE		0x001338
#define	LCG_REG_TB_INVALIDATE_SINGLE	0x001a00
#define	LCG_REG_TB_INVALIDATE_ALL	0x001a08
#define	LCG_REG_TB_INVALIDATE_STATUS	0x001a10
#define	LCG_REG_TB_STATUS		0x001c00
#define	LCG_REG_TB_VPN_COUNT		0x001c04
#define	LCG_REG_TB_DEST_VPN		0x001c14
#define	LCG_REG_TB_SOURCE_VPN		0x001c18
#define	LCG_REG_TB_STENCIL_VPN		0x001c1c
#define	LCG_REG_TB_DEST_DATA_PFN_R	0x001c24
#define	LCG_REG_TB_DEST_DATA_PFN_W	0x001c24
#define	LCG_REG_TB_SOURCE_DATA_PFN_R	0x001c28
#define	LCG_REG_TB_SOURCE_DATA_PFN_W	0x001c28
#define	LCG_REG_TB_STENCIL_DATA_PFN_R	0x001c2c
#define	LCG_REG_TB_STENCIL_DATA_PFN_W	0x001c2c
#define	LCG_REG_TB_DEST_PRE_PFN_R	0x001c34
#define	LCG_REG_TB_DEST_PRE_PFN_W	0x001c34
#define	LCG_REG_TB_SOURCE_PTE_PFN_R	0x001c38
#define	LCG_REG_TB_SOURCE_PTE_PFN_W	0x001c38
#define	LCG_REG_TB_STENCIL_PTE_PFN_R	0x001c3c
#define	LCG_REG_TB_STENCIL_PTE_PFN_W	0x001c3c
#define	LCG_REG_GRAPHICS_CONFIG		0x001c90
#define	LCG_REG_GRAPHICS_INT_STATUS	0x001c94
#define	LCG_REG_GRAPHICS_INT_SET_ENABLE	0x001c98
#define	LCG_REG_GRAPHICS_INT_CLR_ENABLE	0x001c9c
#define	LCG_REG_GRAPHICS_SUB_STATUS	0x001ca0
#define	GSS_AG_BUSY				0x80000000
#define	GSS_SHORT_CIRCUIT			0x20000000
#define	GSS_VALID_PACKET			0x10000000
#define	GSS_1ST_LONGWORD			0x08000000
#define	GSS_FIFO_ARBITRATE			0x04000000
#define	GSS_FIFO_COMMANDER			0x03000000
#define	GSS_AG_BACKWARDS			0x00800000
#define	GSS_AG_VIRTUAL				0x00400000
#define	GSS_AG_ARBITRATE			0x00200000
#define	GSS_AG_READ_ID				0x00100000
#define	GSS_AG_ACCESS_TYPE			0x000c0000
#define	GSS_AG_ACCESS_SIZE			0x00030000
#define	GSS_RESIDUE_LW0				0x00008000
#define	GSS_RESIDUE_LW1				0x00004000
#define	GSS_RESIDUE_LW2				0x00002000
#define	GSS_FIFO_TAIL_BITS			0x00001800
#define	GSS_FIFO_IDU				0x00000400
#define	GSS_EXECUTING_CLIP			0x00000200
#define	GSS_FIFO_BPT_STALL			0x00000100
#define	GSS_FIFO_WFSYNC_STALL			0x00000080
#define	GSS_FIFO_AGBUSY_STALL			0x00000040
#define	GSS_ADRS_BPT_VIRTUAL			0x00000010
#define	GSS_VM					0x00000008
#define	GSS_FIFO_IDLE				0x00000004
#define	GSS_AG_ACCESS				0x00000002
#define	GSS_FIFO				0x00000001
#define	LCG_REG_GRAPHICS_CONTROL	0x001ca4
#define	CTRL_RESET				0x80000000
#define	CTRL_AG					0x40000000
#define	CTRL_CLIP_LIST				0x20000000
#define	CTRL_FIFO				0x10000000
#define	CTRL_SHORT_CIRCUIT			0x08000000
#define	CTRL_VM					0x04000000
#define	CTRL_VM_PROTECTION			0x02000000
#define	CTRL_OPT_INTERFACE			0x01000000
#define	CTRL_OPT_TIMEOUT_SEL			0x00c00000
#define	CTRL_OPT_RESET				0x00200000
#define	CTRL_OPTION_NORESET			0x00100000
#define	CTRL_AG_ACCESS_BPT_ARM			0x00000800
#define	CTRL_PACKET_BPT_ARM			0x00000200
#define	CTRL_ADDRESS_BPT_ARM			0x00000100
#define	LCG_REG_BREAKPT_ADDRESS		0x001cb0
#define	LCG_REG_BREAKPT_VIRTUAL		0x001cb0
#define	LCG_REG_WRITE_PROTECT_LOW_HIGH	0x001cc0
#define	LCG_REG_WRITE_PROTECT_LOW	0x001cc0
#define	LCG_REG_WRITE_PROTECT_HIGH	0x001cc0
#define	LCG_REG_MAX_VIRTUAL_ADDRESS	0x002350
#define	LCG_REG_PA_SPTE_POBR		0x002354

/* Clip List / Command FIFO Registers
 */
#define	LCG_REG_CLIP_LIST_OFFSET	0x0004e4
#define	LCG_REG_CLIP_LIST_BASE		0x0004e4
#define	LCG_REG_CLIP_LIST		0x0004e4
#define	LCG_REG_FIFO_MASKS		0x000570
#define	FIFO_16K				0x00000000
#define	FIFO_32K				0x00004000
#define	FIFO_64K				0x0000c000
#define	FIFO_AFULL_AT_64			0x00000000
#define	FIFO_AFULL_AT_128			0x00002000
#define	FIFO_AFULL_AT_256			0x00003000
#define	FIFO_AFULL_AT_512			0x00003800
#define	FIFO_AFULL_AT_1024			0x00003c00
#define	FIFO_AFULL_AT_2048			0x00003e00
#define	FIFO_AFULL_AT_4096			0x00003f00
#define	FIFO_AEMPTY_AT_32			0x00000000
#define	FIFO_AEMPTY_AT_64			0x00000080
#define	FIFO_AEMPTY_AT_128			0x00000040
#define	FIFO_AEMPTY_AT_256			0x00000020
#define	FIFO_AEMPTY_AT_512			0x00000010
#define	FIFO_AEMPTY_AT_1024			0x00000008
#define	FIFO_AEMPTY_AT_2048			0x00000004
#define	FIFO_AEMPTY_AT_4096			0x00000002
#define	FIFO_AEMPTY_AT_8192			0x00000001
#define	LCG_REG_FIFO_HEAD_OFFSET	0x000574
#define	LCG_REG_FIFO_BASE		0x000574
#define	LCG_REG_FIFO_HEAD		0x000574
#define	LCG_REG_FIFO_TAIL_OFFSET	0x000578
#define	LCG_REG_FIFO_BASE2		0x000578
#define	LCG_REG_FIFO_TAIL		0x000578
#define	LCG_REG_CLIP_LIST_SAVE_OFFSET	0x000ce4
#define	LCG_REG_FIFO_RESIDUE_LW0	0x000d04
#define	LCG_REG_FIFO_RESIDUE_LW1	0x000d08
#define	LCG_REG_FIFO_RESIDUE_LW2	0x000d0c
#define	LCG_REG_FIFO_LENGTH		0x000d70
#define	LCG_REG_FIFO_SAVE_HEAD_OFFSET	0x000d74
#define	LCG_REG_FIFO_WINDOW_BASE	0x080000
#define	LCG_REG_FIFO_WINDOW_END		0x100000

/* Graphics Data Buffer and Pixel SLU Registers
 */
#define	LCG_REG_LOGICAL_FUNCTION	0x000220
#define	LCG_REG_PLANE_MASK		0x000234
#define	LCG_REG_SOURCE_PLANE_INDEX	0x00026c
#define	LCG_REG_FOREGROUND_PIXEL	0x0002c0
#define	LCG_REG_BACKGROUND_PIXEL	0x0004c0
#define	LCG_REG_GDB_LW0			0x000d80
#define	LCG_REG_GDB_LW1			0x000d84
#define	LCG_REG_GDB_LW2			0x000d88
#define	LCG_REG_GDB_LW3			0x000d8c
#define	LCG_REG_GDB_LW4			0x000d90
#define	LCG_REG_GDB_LW5			0x000d94
#define	LCG_REG_GDB_LW6			0x000d98
#define	LCG_REG_GDB_LW7			0x000d9c
#define	LCG_REG_SLU_STATE		0x000da0

/* Address Generator Registers
 */
#define	LCG_REG_CLIP_MIN_Y		0x000244
#define	LCG_REG_CLIP_MIN_MAX_X		0x000248
#define	LCG_REG_CLIP_MIN_X		0x000248
#define	LCG_REG_CLIP_MAX_X		0x000248
#define	LCG_REG_CLIP_MAX_Y		0x00024c
#define	LCG_REG_DEST_X_BIAS		0x000250
#define	LCG_REG_DEST_Y_ORIGIN		0x000254
#define	LCG_REG_DEST_Y_STEP		0x000258
#define	LCG_REG_SOURCE_X_BIAS		0x000260
#define	LCG_REG_SOURCE_Y_BASE		0x000264
#define	LCG_REG_SOURCE_Y_STEP_WIDTH	0x000268
#define	LCG_REG_SOURCE_Y_STEP		0x000268
#define	LCG_REG_SOURCE_WIDTH		0x000268
#define	LCG_REG_STENCIL_X_BIAS		0x000270
#define	LCG_REG_STENCIL_Y_BASE		0x000274
#define	LCG_REG_STENCIL_Y_STEP		0x000278
#define	LCG_REG_DEST_Y_BASE		0x000284
#define	LCG_REG_DEST_X			0x000290
#define	LCG_REG_DEST_WIDTH_HEIGHT	0x000294
#define	LCG_REG_DEST_WIDTH		0x000294
#define	LCG_REG_DEST_HEIGHT		0x000294
#define	LCG_REG_AG_STATUS2		0x000320
#define	LCG_REG_AG_CURRENT_STATE	0x000320
#define	LCG_REG_CURRENT_OPCODE		0x000320
#define	LCG_REG_OP_ACTION_CODE		0x000320
#define	LCG_REG_AG_STATUS		0x000324
#define	LCG_REG_NEXT_X			0x000330
#define	LCG_REG_CLIP_X_DIFF		0x000330
#define	LCG_REG_SOURCE_X_BIAS0		0x000460
#define	LCG_REG_SOURCE_WIDTH0		0x000468
#define	LCG_REG_DEST_X0			0x000490
#define	LCG_REG_DEST_WIDTH0		0x000494
#define	LCG_REG_TILE_ROTATION		0x000660
#define	LCG_REG_TILE_WIDTH		0x000668

/*
 * LUT data bits
 */
#define	LUT_ADRS_REG		0x00	/* write to address register */
#define	LUT_COLOR_AUTOINC	0x01	/* write to LUT and autoincrement */
