/* $OpenBSD: src/sys/dev/ic/Attic/aic7xxxreg.h,v 1.8 2002/02/16 04:36:33 smurph Exp $ */
/*
 * DO NOT EDIT - This file is automatically generated
 *               from the following source files:
 *
 * $Id: src/sys/dev/microcode/aic7xxx/aic7xxx.seq#32 $
 * $Id: src/sys/dev/microcode/aic7xxx/aic7xxx.reg#19 $
 */

#define	SCSISEQ         		0x00
#define		TEMODE          	0x80
#define		SCSIRSTO        	0x01

#define	SXFRCTL0        		0x01
#define		DFON            	0x80
#define		DFPEXP          	0x40
#define		FAST20          	0x20
#define		CLRSTCNT        	0x10
#define		SPIOEN          	0x08
#define		SCAMEN          	0x04
#define		CLRCHN          	0x02

#define	SXFRCTL1        		0x02
#define		BITBUCKET       	0x80
#define		SWRAPEN         	0x40
#define		STIMESEL        	0x18
#define		ENSTIMER        	0x04
#define		ACTNEGEN        	0x02
#define		STPWEN          	0x01

#define	SCSISIGI        		0x03
#define		P_DATAIN_DT     	0x60
#define		P_DATAOUT_DT    	0x20
#define		ATNI            	0x10
#define		SELI            	0x08
#define		BSYI            	0x04
#define		REQI            	0x02
#define		ACKI            	0x01

#define	SCSISIGO        		0x03
#define		CDO             	0x80
#define		IOO             	0x40
#define		MSGO            	0x20
#define		ATNO            	0x10
#define		SELO            	0x08
#define		BSYO            	0x04
#define		REQO            	0x02
#define		ACKO            	0x01

#define	SCSIRATE        		0x04
#define		WIDEXFER        	0x80
#define		SXFR            	0x70
#define		ENABLE_CRC      	0x40
#define		SINGLE_EDGE     	0x10
#define		SXFR_ULTRA2     	0x0f
#define		SOFS            	0x0f

#define	SCSIID          		0x05
#define	SCSIOFFSET      		0x05
#define		SOFS_ULTRA2     	0x7f

#define	SCSIDATL        		0x06

#define	SCSIDATH        		0x07

#define	OPTIONMODE      		0x08
#define		AUTORATEEN      	0x80
#define		AUTOACKEN       	0x40
#define		ATNMGMNTEN      	0x20
#define		BUSFREEREV      	0x10
#define		EXPPHASEDIS     	0x08
#define		SCSIDATL_IMGEN  	0x04
#define		OPTIONMODE_DEFAULTS	0x03
#define		AUTO_MSGOUT_DE  	0x02
#define		DIS_MSGIN_DUALEDGE	0x01

#define	STCNT           		0x08

#define	TARGCRCCNT      		0x0a

#define	CLRSINT0        		0x0b
#define		CLRSELDO        	0x40
#define		CLRSELDI        	0x20
#define		CLRSELINGO      	0x10
#define		CLRIOERR        	0x08
#define		CLRSWRAP        	0x08
#define		CLRSPIORDY      	0x02

#define	SSTAT0          		0x0b
#define		TARGET          	0x80
#define		SELDO           	0x40
#define		SELDI           	0x20
#define		SELINGO         	0x10
#define		IOERR           	0x08
#define		SWRAP           	0x08
#define		SDONE           	0x04
#define		SPIORDY         	0x02
#define		DMADONE         	0x01

#define	CLRSINT1        		0x0c
#define		CLRSELTIMEO     	0x80
#define		CLRATNO         	0x40
#define		CLRSCSIRSTI     	0x20
#define		CLRBUSFREE      	0x08
#define		CLRSCSIPERR     	0x04
#define		CLRPHASECHG     	0x02
#define		CLRREQINIT      	0x01

#define	SSTAT1          		0x0c
#define		SELTO           	0x80
#define		ATNTARG         	0x40
#define		SCSIRSTI        	0x20
#define		PHASEMIS        	0x10
#define		BUSFREE         	0x08
#define		SCSIPERR        	0x04
#define		PHASECHG        	0x02
#define		REQINIT         	0x01

#define	SSTAT2          		0x0d
#define		OVERRUN         	0x80
#define		SHVALID         	0x40
#define		SFCNT           	0x1f
#define		EXP_ACTIVE      	0x10
#define		CRCVALERR       	0x08
#define		CRCENDERR       	0x04
#define		CRCREQERR       	0x02
#define		DUAL_EDGE_ERR   	0x01

#define	SSTAT3          		0x0e
#define		SCSICNT         	0xf0
#define		OFFCNT          	0x0f

#define	SCSIID_ULTRA2   		0x0f

#define	SIMODE0         		0x10
#define		ENSELDO         	0x40
#define		ENSELDI         	0x20
#define		ENSELINGO       	0x10
#define		ENIOERR         	0x08
#define		ENSWRAP         	0x08
#define		ENSDONE         	0x04
#define		ENSPIORDY       	0x02
#define		ENDMADONE       	0x01

#define	SIMODE1         		0x11
#define		ENSELTIMO       	0x80
#define		ENATNTARG       	0x40
#define		ENSCSIRST       	0x20
#define		ENPHASEMIS      	0x10
#define		ENBUSFREE       	0x08
#define		ENSCSIPERR      	0x04
#define		ENPHASECHG      	0x02
#define		ENREQINIT       	0x01

#define	SCSIBUSL        		0x12

#define	SCSIBUSH        		0x13

#define	SHADDR          		0x14

#define	SELTIMER        		0x18
#define	TARGIDIN        		0x18
#define		STAGE6          	0x20
#define		STAGE5          	0x10
#define		STAGE4          	0x08
#define		STAGE3          	0x04
#define		STAGE2          	0x02
#define		STAGE1          	0x01

#define	SELID           		0x19
#define		SELID_MASK      	0xf0
#define		ONEBIT          	0x08

#define	SCAMCTL         		0x1a
#define		ENSCAMSELO      	0x80
#define		CLRSCAMSELID    	0x40
#define		ALTSTIM         	0x20
#define		DFLTTID         	0x10
#define		SCAMLVL         	0x03

#define	TARGID          		0x1b

#define	SPIOCAP         		0x1b
#define		SOFT1           	0x80
#define		SOFT0           	0x40
#define		SOFTCMDEN       	0x20
#define		HAS_BRDCTL      	0x10
#define		SEEPROM         	0x08
#define		EEPROM          	0x04
#define		ROM             	0x02
#define		SSPIOCPS        	0x01

#define	BRDCTL          		0x1d
#define		BRDDAT7         	0x80
#define		BRDDAT6         	0x40
#define		BRDDAT5         	0x20
#define		BRDDAT4         	0x10
#define		BRDSTB          	0x10
#define		BRDDAT3         	0x08
#define		BRDCS           	0x08
#define		BRDDAT2         	0x04
#define		BRDRW           	0x04
#define		BRDCTL1         	0x02
#define		BRDRW_ULTRA2    	0x02
#define		BRDCTL0         	0x01
#define		BRDSTB_ULTRA2   	0x01

#define	SEECTL          		0x1e
#define		EXTARBACK       	0x80
#define		EXTARBREQ       	0x40
#define		SEEMS           	0x20
#define		SEERDY          	0x10
#define		SEECS           	0x08
#define		SEECK           	0x04
#define		SEEDO           	0x02
#define		SEEDI           	0x01

#define	SBLKCTL         		0x1f
#define		DIAGLEDEN       	0x80
#define		DIAGLEDON       	0x40
#define		AUTOFLUSHDIS    	0x20
#define		ENAB40          	0x08
#define		SELBUSB         	0x08
#define		ENAB20          	0x04
#define		SELWIDE         	0x02
#define		XCVR            	0x01

#define	SRAM_BASE       		0x20

#define	BUSY_TARGETS    		0x20
#define	TARG_SCSIRATE   		0x20

#define	ULTRA_ENB       		0x30
#define	CMDSIZE_TABLE   		0x30

#define	DISC_DSB        		0x32

#define	CMDSIZE_TABLE_TAIL		0x34

#define	MWI_RESIDUAL    		0x38

#define	NEXT_QUEUED_SCB 		0x39

#define	MSG_OUT         		0x3a

#define	DMAPARAMS       		0x3b
#define		PRELOADEN       	0x80
#define		WIDEODD         	0x40
#define		SCSIEN          	0x20
#define		SDMAENACK       	0x10
#define		SDMAEN          	0x10
#define		HDMAENACK       	0x08
#define		HDMAEN          	0x08
#define		DIRECTION       	0x04
#define		FIFOFLUSH       	0x02
#define		FIFORESET       	0x01

#define	SEQ_FLAGS       		0x3c
#define		IDENTIFY_SEEN   	0x80
#define		TARGET_CMD_IS_TAGGED	0x40
#define		DPHASE          	0x20
#define		TARG_CMD_PENDING	0x10
#define		CMDPHASE_PENDING	0x08
#define		DPHASE_PENDING  	0x04
#define		SPHASE_PENDING  	0x02
#define		NO_DISCONNECT   	0x01

#define	SAVED_SCSIID    		0x3d

#define	SAVED_LUN       		0x3e

#define	LASTPHASE       		0x3f
#define		PHASE_MASK      	0xe0
#define		P_MESGIN        	0xe0
#define		P_STATUS        	0xc0
#define		P_MESGOUT       	0xa0
#define		P_COMMAND       	0x80
#define		CDI             	0x80
#define		IOI             	0x40
#define		P_DATAIN        	0x40
#define		MSGI            	0x20
#define		P_BUSFREE       	0x01
#define		P_DATAOUT       	0x00

#define	WAITING_SCBH    		0x40

#define	DISCONNECTED_SCBH		0x41

#define	FREE_SCBH       		0x42

#define	COMPLETE_SCBH   		0x43

#define	HSCB_ADDR       		0x44

#define	SHARED_DATA_ADDR		0x48

#define	KERNEL_QINPOS   		0x4c

#define	QINPOS          		0x4d

#define	QOUTPOS         		0x4e

#define	KERNEL_TQINPOS  		0x4f

#define	TQINPOS         		0x50

#define	ARG_1           		0x51
#define	RETURN_1        		0x51
#define		SEND_MSG        	0x80
#define		SEND_SENSE      	0x40
#define		SEND_REJ        	0x20
#define		MSGOUT_PHASEMIS 	0x10
#define		EXIT_MSG_LOOP   	0x08
#define		CONT_MSG_LOOP   	0x04
#define		CONT_TARG_SESSION	0x02

#define	ARG_2           		0x52
#define	RETURN_2        		0x52

#define	LAST_MSG        		0x53

#define	SCSISEQ_TEMPLATE		0x54
#define		ENSELO          	0x40
#define		ENSELI          	0x20
#define		ENRSELI         	0x10
#define		ENAUTOATNO      	0x08
#define		ENAUTOATNI      	0x04
#define		ENAUTOATNP      	0x02

#define	DATA_COUNT_ODD  		0x55

#define	INITIATOR_TAG   		0x56

#define	SEQ_FLAGS2      		0x57
#define		SCB_DMA         	0x01

#define	SCSICONF        		0x5a
#define		TERM_ENB        	0x80
#define		RESET_SCSI      	0x40
#define		ENSPCHK         	0x20
#define		HWSCSIID        	0x0f
#define		HSCSIID         	0x07

#define	INTDEF          		0x5c
#define		EDGE_TRIG       	0x80
#define		VECTOR          	0x0f

#define	HOSTCONF        		0x5d

#define	HA_274_BIOSCTRL 		0x5f
#define		BIOSMODE        	0x30
#define		BIOSDISABLED    	0x30
#define		CHANNEL_B_PRIMARY	0x08

#define	SEQCTL          		0x60
#define		PERRORDIS       	0x80
#define		PAUSEDIS        	0x40
#define		FAILDIS         	0x20
#define		FASTMODE        	0x10
#define		BRKADRINTEN     	0x08
#define		STEP            	0x04
#define		SEQRESET        	0x02
#define		LOADRAM         	0x01

#define	SEQRAM          		0x61

#define	SEQADDR0        		0x62

#define	SEQADDR1        		0x63
#define		SEQADDR1_MASK   	0x01

#define	ACCUM           		0x64

#define	SINDEX          		0x65

#define	DINDEX          		0x66

#define	ALLONES         		0x69

#define	NONE            		0x6a

#define	ALLZEROS        		0x6a

#define	FLAGS           		0x6b
#define		ZERO            	0x02
#define		CARRY           	0x01

#define	SINDIR          		0x6c

#define	DINDIR          		0x6d

#define	FUNCTION1       		0x6e

#define	STACK           		0x6f

#define	TARG_OFFSET     		0x70

#define	BCTL            		0x84
#define		ACE             	0x08
#define		ENABLE          	0x01

#define	DSCOMMAND0      		0x84
#define		CACHETHEN       	0x80
#define		DPARCKEN        	0x40
#define		MPARCKEN        	0x20
#define		EXTREQLCK       	0x10
#define		INTSCBRAMSEL    	0x08
#define		RAMPS           	0x04
#define		USCBSIZE32      	0x02
#define		CIOPARCKEN      	0x01

#define	BUSTIME         		0x85
#define		BOFF            	0xf0
#define		BON             	0x0f

#define	DSCOMMAND1      		0x85
#define		DSLATT          	0xfc
#define		HADDLDSEL1      	0x02
#define		HADDLDSEL0      	0x01

#define	BUSSPD          		0x86
#define		DFTHRSH         	0xc0
#define		DFTHRSH_75      	0x80
#define		STBOFF          	0x38
#define		STBON           	0x07

#define	HS_MAILBOX      		0x86
#define		HOST_MAILBOX    	0xf0
#define		HOST_TQINPOS    	0x80
#define		HOST_REQ_INT    	0x10
#define		SEQ_MAILBOX     	0x0f

#define	DSPCISTATUS     		0x86
#define		DFTHRSH_100     	0xc0

#define	HCNTRL          		0x87
#define		POWRDN          	0x40
#define		SWINT           	0x10
#define		IRQMS           	0x08
#define		PAUSE           	0x04
#define		INTEN           	0x02
#define		CHIPRSTACK      	0x01
#define		CHIPRST         	0x01

#define	HADDR           		0x88

#define	HCNT            		0x8c

#define	SCBPTR          		0x90

#define	INTSTAT         		0x91
#define		SEQINT_MASK     	0xf1
#define		OUT_OF_RANGE    	0xe1
#define		NO_FREE_SCB     	0xd1
#define		SCB_MISMATCH    	0xc1
#define		MISSED_BUSFREE  	0xb1
#define		MKMSG_FAILED    	0xa1
#define		DATA_OVERRUN    	0x91
#define		PERR_DETECTED   	0x81
#define		BAD_STATUS      	0x71
#define		HOST_MSG_LOOP   	0x61
#define		PDATA_REINIT    	0x51
#define		IGN_WIDE_RES    	0x41
#define		NO_MATCH        	0x31
#define		NO_IDENT        	0x21
#define		SEND_REJECT     	0x11
#define		INT_PEND        	0x0f
#define		BRKADRINT       	0x08
#define		SCSIINT         	0x04
#define		CMDCMPLT        	0x02
#define		SEQINT          	0x01
#define		BAD_PHASE       	0x01

#define	ERROR           		0x92
#define		CIOPARERR       	0x80
#define		PCIERRSTAT      	0x40
#define		MPARERR         	0x20
#define		DPARERR         	0x10
#define		SQPARERR        	0x08
#define		ILLOPCODE       	0x04
#define		ILLSADDR        	0x02
#define		ILLHADDR        	0x01

#define	CLRINT          		0x92
#define		CLRPARERR       	0x10
#define		CLRBRKADRINT    	0x08
#define		CLRSCSIINT      	0x04
#define		CLRCMDINT       	0x02
#define		CLRSEQINT       	0x01

#define	DFCNTRL         		0x93

#define	DFSTATUS        		0x94
#define		PRELOAD_AVAIL   	0x80
#define		DFCACHETH       	0x40
#define		FIFOQWDEMP      	0x20
#define		MREQPEND        	0x10
#define		HDONE           	0x08
#define		DFTHRESH        	0x04
#define		FIFOFULL        	0x02
#define		FIFOEMP         	0x01

#define	DFWADDR         		0x95

#define	DFRADDR         		0x97

#define	DFDAT           		0x99

#define	SCBCNT          		0x9a
#define		SCBAUTO         	0x80
#define		SCBCNT_MASK     	0x1f

#define	QINFIFO         		0x9b

#define	QINCNT          		0x9c

#define	QOUTFIFO        		0x9d

#define	CRCCONTROL1     		0x9d
#define		CRCONSEEN       	0x80
#define		CRCVALCHKEN     	0x40
#define		CRCENDCHKEN     	0x20
#define		CRCREQCHKEN     	0x10
#define		TARGCRCENDEN    	0x08
#define		TARGCRCCNTEN    	0x04

#define	QOUTCNT         		0x9e

#define	SCSIPHASE       		0x9e
#define		STATUS_PHASE    	0x20
#define		COMMAND_PHASE   	0x10
#define		MSG_IN_PHASE    	0x08
#define		MSG_OUT_PHASE   	0x04
#define		DATA_PHASE_MASK 	0x03
#define		DATA_IN_PHASE   	0x02
#define		DATA_OUT_PHASE  	0x01

#define	SFUNCT          		0x9f
#define		ALT_MODE        	0x80

#define	SCB_CDB_PTR     		0xa0
#define	SCB_TARGET_INFO 		0xa0
#define	SCB_RESIDUAL_DATACNT		0xa0
#define	SCB_CDB_STORE   		0xa0

#define	SCB_BASE        		0xa0

#define	SCB_RESIDUAL_SGPTR		0xa4

#define	SCB_SCSI_STATUS 		0xa8

#define	SCB_CDB_STORE_PAD		0xa9

#define	SCB_DATAPTR     		0xac

#define	SCB_DATACNT     		0xb0
#define		SG_LAST_SEG     	0x80
#define		SG_HIGH_ADDR_BITS	0x7f

#define	SCB_SGPTR       		0xb4
#define		SG_RESID_VALID  	0x04
#define		SG_FULL_RESID   	0x02
#define		SG_LIST_NULL    	0x01

#define	SCB_CONTROL     		0xb8
#define		TARGET_SCB      	0x80
#define		DISCENB         	0x40
#define		TAG_ENB         	0x20
#define		MK_MESSAGE      	0x10
#define		ULTRAENB        	0x08
#define		DISCONNECTED    	0x04
#define		SCB_TAG_TYPE    	0x03

#define	SCB_SCSIID      		0xb9
#define		TID             	0xf0
#define		TWIN_CHNLB      	0x80
#define		TWIN_TID        	0x70
#define		OID             	0x0f

#define	SCB_LUN         		0xba
#define		LID             	0xff

#define	SCB_TAG         		0xbb

#define	SCB_CDB_LEN     		0xbc

#define	SCB_SCSIRATE    		0xbd

#define	SCB_SCSIOFFSET  		0xbe

#define	SCB_NEXT        		0xbf

#define	SEECTL_2840     		0xc0
#define		CS_2840         	0x04
#define		CK_2840         	0x02
#define		DO_2840         	0x01

#define	SCB_64_SPARE    		0xc0

#define	STATUS_2840     		0xc1
#define		EEPROM_TF       	0x80
#define		BIOS_SEL        	0x60
#define		ADSEL           	0x1e
#define		DI_2840         	0x01

#define	SCB_64_BTT      		0xd0

#define	CCHADDR         		0xe0

#define	CCHCNT          		0xe8

#define	CCSGRAM         		0xe9

#define	CCSGADDR        		0xea

#define	CCSGCTL         		0xeb
#define		CCSGDONE        	0x80
#define		CCSGEN          	0x08
#define		SG_FETCH_NEEDED 	0x02
#define		CCSGRESET       	0x01

#define	CCSCBRAM        		0xec

#define	CCSCBADDR       		0xed

#define	CCSCBCTL        		0xee
#define		CCSCBDONE       	0x80
#define		ARRDONE         	0x40
#define		CCARREN         	0x10
#define		CCSCBEN         	0x08
#define		CCSCBDIR        	0x04
#define		CCSCBRESET      	0x01

#define	CCSCBCNT        		0xef

#define	SCBBADDR        		0xf0

#define	CCSCBPTR        		0xf1

#define	HNSCB_QOFF      		0xf4

#define	SNSCB_QOFF      		0xf6

#define	SDSCB_QOFF      		0xf8

#define	QOFF_CTLSTA     		0xfa
#define		SCB_AVAIL       	0x40
#define		SNSCB_ROLLOVER  	0x20
#define		SDSCB_ROLLOVER  	0x10
#define		SCB_QSIZE       	0x07
#define		SCB_QSIZE_256   	0x06

#define	DFF_THRSH       		0xfb
#define		WR_DFTHRSH      	0x70
#define		WR_DFTHRSH_MAX  	0x70
#define		WR_DFTHRSH_90   	0x60
#define		WR_DFTHRSH_85   	0x50
#define		WR_DFTHRSH_75   	0x40
#define		WR_DFTHRSH_63   	0x30
#define		WR_DFTHRSH_50   	0x20
#define		WR_DFTHRSH_25   	0x10
#define		RD_DFTHRSH_MAX  	0x07
#define		RD_DFTHRSH      	0x07
#define		RD_DFTHRSH_90   	0x06
#define		RD_DFTHRSH_85   	0x05
#define		RD_DFTHRSH_75   	0x04
#define		RD_DFTHRSH_63   	0x03
#define		RD_DFTHRSH_50   	0x02
#define		RD_DFTHRSH_25   	0x01
#define		RD_DFTHRSH_MIN  	0x00
#define		WR_DFTHRSH_MIN  	0x00

#define	SG_CACHE_SHADOW 		0xfc
#define		SG_ADDR_MASK    	0xf8
#define		ODD_SEG         	0x04
#define		LAST_SEG        	0x02
#define		LAST_SEG_DONE   	0x01

#define	SG_CACHE_PRE    		0xfc


#define	HOST_MSG	0xff
#define	BUS_32_BIT	0x02
#define	CMD_GROUP_CODE_SHIFT	0x05
#define	BUS_8_BIT	0x00
#define	CCSGRAM_MAXSEGS	0x10
#define	TARGET_DATA_IN	0x01
#define	STATUS_QUEUE_FULL	0x28
#define	STATUS_BUSY	0x08
#define	MAX_OFFSET_8BIT	0x0f
#define	BUS_16_BIT	0x01
#define	TID_SHIFT	0x04
#define	SCB_DOWNLOAD_SIZE_64	0x30
#define	SCB_UPLOAD_SIZE	0x20
#define	HOST_MAILBOX_SHIFT	0x04
#define	SCB_INITIATOR_TAG	0x03
#define	SCB_TARGET_STATUS	0x02
#define	SCB_TARGET_DATA_DIR	0x01
#define	SCB_TARGET_PHASES	0x00
#define	MAX_OFFSET_ULTRA2	0x7f
#define	MAX_OFFSET_16BIT	0x08
#define	TARGET_CMD_CMPLT	0xfe
#define	SCB_LIST_NULL	0xff
#define	SG_SIZEOF	0x08
#define	SCB_DOWNLOAD_SIZE	0x20
#define	SEQ_MAILBOX_SHIFT	0x00
#define	CCSGADDR_MAX	0x80


/* Downloaded Constant Definitions */
#define	SG_PREFETCH_ADDR_MASK	0x06
#define	SG_PREFETCH_ALIGN_MASK	0x05
#define	QOUTFIFO_OFFSET	0x00
#define	SG_PREFETCH_CNT	0x04
#define	INVERTED_CACHESIZE_MASK	0x03
#define	CACHESIZE_MASK	0x02
#define	QINFIFO_OFFSET	0x01
