/*	$OpenBSD: src/sys/arch/sh/include/reloc.h,v 1.1 2006/11/10 02:39:47 drahn Exp $	*/
/* Processor specific relocation types */

#define	R_SH_NONE				0
#define	R_SH_DIR32				1
#define	R_SH_REL32				2
#define	R_SH_DIR8WPN				3
#define	R_SH_IND12W				4
#define	R_SH_DIR8WPL				5
#define	R_SH_DIR8WPZ				6
#define	R_SH_DIR8BP				7
#define	R_SH_DIR8W				8
#define	R_SH_DIR8L				9

/* GNU extensions */
#define	R_SH_LOOP_START				10
#define	R_SH_LOOP_END				11
#define	R_SH_GNU_VTINHERIT			22
#define	R_SH_GNU_VTENTRY			23
#define	R_SH_SWITCH8				24
#define	R_SH_SWITCH16				25
#define	R_SH_SWITCH32				26
#define	R_SH_USES				27
#define	R_SH_COUNT				28
#define	R_SH_ALIGN				29
#define	R_SH_CODE				30
#define	R_SH_DATA				31
#define	R_SH_LABEL				32

#define	R_SH_DIR16				33
#define	R_SH_DIR8				34
#define	R_SH_DIR8UL				35
#define	R_SH_DIR8UW				36
#define	R_SH_DIR8U				37
#define	R_SH_DIR8SW				38
#define	R_SH_DIR8S				39
#define	R_SH_DIR4UL				40
#define	R_SH_DIR4UW				41
#define	R_SH_DIR4U				42
#define	R_SH_PSHA				43
#define	R_SH_PSHL				44
#define	R_SH_DIR5U				45
#define	R_SH_DIR6U				46
#define	R_SH_DIR6S				47
#define	R_SH_DIR10S				48
#define	R_SH_DIR10SW				49
#define	R_SH_DIR10SL				50
#define	R_SH_DIR10SQ				51
#define	R_SH_DIR16S				53

/* GNU extensions */
#define	R_SH_TLS_GD_32				144
#define	R_SH_TLS_LD_32				145
#define	R_SH_TLS_LDO_32				146
#define	R_SH_TLS_IE_32				147
#define	R_SH_TLS_LE_32				148
#define	R_SH_TLS_DTPMOD32			149
#define	R_SH_TLS_DTPOFF32			150
#define	R_SH_TLS_TPOFF32			151
#define	R_SH_GOT32				160
#define	R_SH_PLT32				161
#define	R_SH_COPY				162
#define	R_SH_GLOB_DAT				163
#define	R_SH_JMP_SLOT				164
#define	R_SH_RELATIVE				165
#define	R_SH_GOTOFF				166
#define	R_SH_GOTPC				167
#define	R_SH_GOTPLT32				168
#define	R_SH_GOT_LOW16				169
#define	R_SH_GOT_MEDLOW16			170
#define	R_SH_GOT_MEDHI16			171
#define	R_SH_GOT_HI16				172
#define	R_SH_GOTPLT_LOW16			173
#define	R_SH_GOTPLT_MEDLOW16			174
#define	R_SH_GOTPLT_MEDHI16			175
#define	R_SH_GOTPLT_HI16			176
#define	R_SH_PLT_LOW16				177
#define	R_SH_PLT_MEDLOW16			178
#define	R_SH_PLT_MEDHI16			179
#define	R_SH_PLT_HI16				180
#define	R_SH_GOTOFF_LOW16			181
#define	R_SH_GOTOFF_MEDLOW16			182
#define	R_SH_GOTOFF_MEDHI16			183
#define	R_SH_GOTOFF_HI16			184
#define	R_SH_GOTPC_LOW16			185
#define	R_SH_GOTPC_MEDLOW16			186
#define	R_SH_GOTPC_MEDHI16			187
#define	R_SH_GOTPC_HI16				188
#define	R_SH_GOT10BY4				189
#define	R_SH_GOTPLT10BY4			190
#define	R_SH_GOT10BY8				191
#define	R_SH_GOTPLT10BY8			192
#define	R_SH_COPY64				193
#define	R_SH_GLOB_DAT64				194
#define	R_SH_JMP_SLOT64				195
#define	R_SH_RELATIVE64				196
#define	R_SH_SHMEDIA_CODE			242
#define	R_SH_PT_16				243
#define	R_SH_IMMS16				244
#define	R_SH_IMMU16				245
#define	R_SH_IMM_LOW16				246
#define	R_SH_IMM_LOW16_PCREL			247
#define	R_SH_IMM_MEDLOW16			248
#define	R_SH_IMM_MEDLOW16_PCREL			249
#define	R_SH_IMM_MEDHI16			250
#define	R_SH_IMM_MEDHI16_PCREL			251
#define	R_SH_IMM_HI16				252
#define	R_SH_IMM_HI16_PCREL			253
#define	R_SH_64					254
#define	R_SH_64_PCREL				255
