#	$OpenBSD: src/sys/arch/arc/conf/Attic/files.arc,v 1.11 1996/12/11 12:59:29 niklas Exp $
#
# maxpartitions must be first item in files.${ARCH}
#
maxpartitions 16

maxusers 2 8 64

#	Required files

file	arch/arc/arc/autoconf.c
file	arch/arc/arc/conf.c
file	arch/arc/arc/cpu_exec.c
file	arch/arc/arc/disksubr.c
file	arch/arc/dev/dma.c
file	arch/arc/arc/machdep.c
file	arch/arc/arc/minidebug.c
file	arch/arc/arc/mem.c
file	arch/arc/arc/pmap.c
file	arch/arc/arc/process_machdep.c
file	arch/arc/arc/sys_machdep.c
file	arch/arc/arc/trap.c
file	arch/arc/arc/vm_machdep.c

file	arch/arc/arc/arcbios.c

#
#	Machine-independent ATAPI drivers 
#
include "../../../dev/atapi/files.atapi"
major	{ acd = 5 }

#
#	System BUS types
#
define	mainbus {}
device	mainbus
attach	mainbus at root
file	arch/arc/arc/mainbus.c	mainbus

#	Our CPU configurator
device	cpu
attach	cpu at mainbus			# not optional
file arch/arc/arc/cpu.c			cpu

#
#	PICA bus autoconfiguration devices
#
device	pica {}
attach	pica at mainbus			# 
file	arch/arc/pica/picabus.c		pica

#
#	ISA Bus bridge
#
device	isabr {} : isabus
attach	isabr at mainbus
file	arch/arc/isa/isabus.c		isabr

#	Ethernet chip
device	sn
attach	sn at pica: ifnet, ether
file	arch/arc/dev/if_sn.c		sn	needs-count

#	Use machine independent SCSI driver routines
include	"../../../scsi/files.scsi"
major	{sd = 0}
major	{cd = 3}

#	Machine dependent SCSI interface driver
device	asc: scsi
attach	asc at pica
file	arch/arc/dev/asc.c		asc	needs-count

#	Floppy disk controller
device	fdc {drive = -1}
attach	fdc at pica
device	fd: disk
attach	fd at fdc
file	arch/arc/dev/fd.c		fdc	needs-flag
major	{fd = 7}

#
#	Stock ISA bus support
#
define  pcmcia {}			# XXX dummy decl...
define  pci {}				# XXX dummy decl...

include	"../../../dev/isa/files.isa"
major	{ wd = 4 }

#	Real time clock, must have one..
device	clock
attach	clock at pica with clock_pica
attach	clock at isa with clock_isa
file	arch/arc/arc/clock.c	clock & (clock_isa | clock_pica) needs-flag
file	arch/arc/arc/clock_mc.c	clock & (clock_isa | clock_pica) needs-flag

#	Console driver on PC-style graphics
device	pc: tty
attach	pc at pica with pc_pica
attach	pc at isa with pc_isa
device	pms: tty
attach	pms at pica
file	arch/arc/dev/pccons.c		pc & (pc_pica | pc_isa)	needs-flag

# BusLogic BT-445C VLB SCSI Controller. Special on TYNE local bus.
device  btl: scsi
attach  btl at isa
file    arch/arc/dti/btl.c                    btl needs-count

# 8250/16[45]50-based "com" ports
attach	com at pica with com_pica
file	arch/arc/pica/com_pica.c	com_pica

# National Semiconductor DS8390/WD83C690-based boards
# (WD/SMC 80x3 family, SMC Ultra [8216], 3Com 3C503, NE[12]000, and clones)
# XXX conflicts with other ports; can't be in files.isa
device  ed: ether, ifnet
attach  ed at isa with ed_isa
attach  ed at pcmcia with ed_pcmcia
file    dev/isa/if_ed.c                 ed & (ed_isa | ed_pcmcia) needs-flag

# PC parallel ports (XXX what chip?)
attach	lpt at pica with lpt_pica
file	arch/arc/pica/lpt_pica.c	lpt_pica

#

file	dev/cons.c
file	dev/cninit.c
file	netinet/in_cksum.c
file	netns/ns_cksum.c			ns

file	compat/ultrix/ultrix_misc.c		compat_ultrix
file	compat/ultrix/ultrix_syscalls.c		compat_ultrix
file	compat/ultrix/ultrix_sysent.c		compat_ultrix

