/*	$OpenBSD: src/sys/arch/sgi/sgi/machdep.c,v 1.1 2004/08/06 21:12:19 pefo Exp $ */

/*
 * Copyright (c) 2003-2004 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/exec_olf.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef MFS
#include <ufs/mfs/mfs_extern.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <ddb/db_interface.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pio.h>
#include <machine/psl.h>
#include <machine/autoconf.h>
#include <machine/memconf.h>
#include <machine/regnum.h>

#include <machine/rm7000.h> 

#include <sys/exec_ecoff.h>

#include <dev/cons.h>

#include <mips64/archtype.h>
#include <machine/bus.h>

#include <sgi/localbus/macebus.h>

extern struct consdev *cn_tab;
extern char kernel_text[];
extern void makebootdev __P((char *));
extern void stacktrace __P((void));

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* machine "architecture" */
char	machine_arch[] = MACHINE_ARCH;	/* cpu "architecture" */
char	cpu_model[30];
#ifdef APERTURE
#if defined(INSECURE) || defined(DEBUG)
int allowaperture = 1;
#else
int allowaperture = 0;
#endif
#endif

/*
 * Declare these as initialized data so we can patch them.
 */
#ifndef	NBUF
#define NBUF 0			/* Can be changed in config */
#endif
#ifndef	BUFPAGES
#define BUFPAGES 0		/* Can be changed in config */
#endif

int	nswbuf = 0;
int	nbuf = NBUF;
int	bufpages = BUFPAGES;

vm_map_t exec_map;
vm_map_t mb_map;
vm_map_t phys_map;

#if 0
register_t tlbtrcptr;
#endif

int	msgbufmapped;		/* set when safe to use msgbuf */
int	physmem;		/* max supported memory, changes to actual */
int	ncpu = 1;		/* At least one cpu in the system */
struct	user *proc0paddr;
struct	user *curprocpaddr;
int	console_ok;		/* set when console initialized */

int32_t *environment;
char	eth_hw_addr[6];		/* HW ether addr not stored elsewhere */
struct sys_rec sys_config;


/* ddb symbol init stuff */
caddr_t	ssym;
caddr_t	esym;
caddr_t	ekern;

struct mem_descriptor mem_layout[MAXMEMSEGS];
vaddr_t     avail_end;      /* PA of last available physical page */

caddr_t mips_init __P((int, int32_t *, int32_t *));	
void initcpu(void);
void dumpsys(void);
void dumpconf(void);
caddr_t allocsys(caddr_t);

static char *getenv(char *env);
static void dobootopts(char *cp);
static void get_eth_hw_addr(char *, char *);
static int atoi(char *, int);
char gt_ethaddr[6];

#if BYTE_ORDER == BIG_ENDIAN
int	my_endian = 1;
#else
int	my_endian = 0;
#endif


/*
 * Do all the stuff that locore normally does before calling main().
 * Reset mapping and set up mapping to hardware and init "wired" reg.
 */

caddr_t
mips_init(int argc, int32_t *argv, int32_t *envv)
{
	char *cp;
	char *arg0;
	int i;
	unsigned firstaddr;
	caddr_t sd;
	struct tlb tlb;
	extern char edata[], end[];
	extern char tlb_miss[], e_tlb_miss[];
	extern char tlb_miss_tramp[], e_tlb_miss_tramp[];
	extern char xtlb_miss_tramp[], e_xtlb_miss_tramp[];
	extern char exception[], e_exception[];

	/*
	 * Clear the compiled BSS segment in OpenBSD code
	 */
	bzero(edata, (int)end - (int)edata);

	/*
	 *  Reserve symol table space. If invalid pointers no table.
	 */
	ssym = (char *)(long)*(int *)end;
	esym = (char *)(long)*((int *)end + 1);
	ekern = esym;
	if (((int)ssym - (int)end) < 0 ||
	    ((int)ssym - (int)end) > 0x1000 ||
	    ssym[0] != ELFMAG0 || ssym[1] != ELFMAG1 ||
	    ssym[2] != ELFMAG2 || ssym[3] != ELFMAG3 ) {
		ssym = NULL;
		esym = NULL;
		ekern = end;
	}

	/*
	 * Point environment for getenv() lookups.
	 */
	environment = envv;

	/*
	 * Determine system type and set up configuration record data.
	 */
	sys_config.system_type = -1;
	cp = getenv("systype");
/*XXX*/    cp = "moosehead";
	if(cp && strncasecmp("moosehead", cp, 9) == 0) {
		sys_config.system_type = SGI_O2;
		strlcpy(cpu_model, "SGI O2", sizeof(cpu_model));
		sys_config.cons_ioaddr[0] = 0x00390000;	/*XXX*/
		sys_config.cons_ioaddr[1] = 0x00398000;	/*XXX*/
		sys_config.cons_baudclk = 1843200;		/*XXX*/
		sys_config.cons_iot = &macebus_tag;
		sys_config.local.bus_base = 0x0;		/*XXX*/
#if defined(_LP64)
		sys_config.pci_io[0].bus_base = 0xffffffff00000000;/*XXX*/
		sys_config.pci_mem[0].bus_base = 0xffffffff00000000;/*XXX*/
#else
		sys_config.pci_io[0].bus_base = 0x00000000;/*XXX*/
		sys_config.pci_mem[0].bus_base = 0x00000000;/*XXX*/
#endif
		sys_config.pci_mem[0].bus_base_dma = 0x00000000;/*XXX*/
		sys_config.pci_mem[0].bus_reverse = my_endian;
		sys_config.cpu.tlbwired = 3;
	}

	/*
	 * Determine CPU clock frequency for timer and delay setup
	 */
	if(getenv("cpuclock") != 0) {
		sys_config.cpu.clock = atoi(getenv("cpuclock"), 10);
	}
	else {
		sys_config.cpu.clock = 180000000;  /* Reasonable default */
	}

	/*
	 *  Initialize virtual memory system.
         */
	if(getenv("memsize") != 0) {
		physmem = atop(atoi(getenv("memsize"), 10) * 1024 *1024);
	}
	else {
		physmem = atop(1024 * 1024 * 64);  /* Reasonable default */
	}
#if 0
#if defined(DDB) || defined(DEBUG)
	physmem = atop(1024 * 1024 * 256);
#if defined(_LP64)
	tlbtrcptr = 0xffffffff90000000;
	memset((void *)0xffffffff90000000, 0, 1024*1024);
#else
	tlbtrcptr = 0x90000000;
	memset((void *)0x90000000, 0, 1024*1024);
#endif
#endif
#endif

	/* Set pagesize to enable use of page macros and functions */
	uvmexp.pagesize = 4096;
	uvm_setpagesize();

	/* Build up memory description and commit to UVM system */
	mem_layout[0].mem_start = atop(0x20000);	/* Skip int vectors */
	mem_layout[0].mem_size = atop(KSEG0_TO_PHYS(kernel_text));
	mem_layout[0].mem_size -= mem_layout[0].mem_start;

	mem_layout[1].mem_start = atop(round_page(KSEG0_TO_PHYS((long)ekern)));
	mem_layout[1].mem_size = physmem - mem_layout[1].mem_start;

	avail_end = ptoa(physmem);

	for(i = 1; i < MAXMEMSEGS && mem_layout[i].mem_size != 0; i++) {
		vaddr_t fp, lp;

		fp = mem_layout[i].mem_start;
		lp = mem_layout[i].mem_start + mem_layout[i].mem_size;
		uvm_page_physload(fp, lp, fp, lp, VM_FREELIST_DEFAULT); 
	}

	/*
	 *  Figure out where we was booted from.
	 */
argc = 0;
	if(argc > 1)
		arg0 = (char *)(long)argv[1];
	else
		arg0 = getenv("bootdev");

	if(arg0 == 0)
		arg0 = "unknown";
	makebootdev(arg0);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * Default to SINGLE and ASKNAME if no args or
	 * SINGLE and DFLTROOT if this is a ramdisk kernel.
	 */
#ifdef RAMDISK_HOOKS
	boothowto = RB_SINGLE | RB_DFLTROOT;
#else
	boothowto = RB_SINGLE | RB_ASKNAME;
#endif /* RAMDISK_HOOKS */

	get_eth_hw_addr(getenv("ethaddr"), eth_hw_addr);
	dobootopts(getenv("osloadoptions"));

	/*  Check any extra arguments which override.  */
	for(i = 2; i < argc; i++) {
		if(*((char *)(long)argv[i]) == '-') {
			dobootopts((char *)(long)argv[i] + 1);
		}
	}

	/* Check l3cache size and disable (hard) if non present. */
	if(getenv("l3cache") != 0) {
		i = atoi(getenv("l3cache"), 10);
		CpuTertiaryCacheSize = 1024 * 1024 * i;
	} else {
		CpuTertiaryCacheSize = 0;
	}
	if(CpuTertiaryCacheSize == 0) {
		CpuExternalCacheOn = 0;		/* No L3 detected */
	} else {
		CpuExternalCacheOn = 1;
	}

	cp = getenv("ecache_on");
	if(cp && (*cp == 0 || *cp == 'n' || *cp == 'N')) {
		CpuExternalCacheOn = 0;		/* Override config setting */
	}

	cp = getenv("ocache_on");
	if(cp && (*cp == 0 || *cp == 'n' || *cp == 'N')) {
		CpuOnboardCacheOn = 0;		/* Override HW setting */
	} else {
		CpuOnboardCacheOn = 1;
	}

	sys_config.cpu.cfg_reg = Mips_ConfigCache();
	sys_config.cpu.type = cpu_id.cpu.cp_imp;
	sys_config.cpu.vers_maj = cpu_id.cpu.cp_majrev;
	sys_config.cpu.vers_min = cpu_id.cpu.cp_minrev;

	/*
	 *  Configure TLB.
	 */
	switch(sys_config.cpu.type) {
	case MIPS_RM7000:
		if(sys_config.cpu.vers_maj < 2) {
			sys_config.cpu.tlbsize = 48;
		} else {
			sys_config.cpu.tlbsize = 64;
		}
		break;

	default:
		sys_config.cpu.tlbsize = 48;
		break;
	}

	if(getenv("tlbwired")) {
		i = atoi(getenv("tlbwired"), 10);
		if((i < sys_config.cpu.tlbwired) || (i >= sys_config.cpu.tlbsize)) {
		} else {
			sys_config.cpu.tlbwired = i;
		}
	}
	tlb_set_wired(0);
	tlb_flush(sys_config.cpu.tlbsize);
	tlb_set_wired(sys_config.cpu.tlbwired);
	
	/*
	 *  Set up some fixed mappings. These are so frequently
	 *  used so faulting them in will waste to many cycles.
	 */
	if (sys_config.system_type == MOMENTUM_CP7000G ||
	    sys_config.system_type == MOMENTUM_CP7000 ||
	    sys_config.system_type == GALILEO_EV64240) {
		tlb.tlb_mask = PG_SIZE_16M;
#if defined(LP64)
		tlb.tlb_hi = vad_to_vpn(0xfffffffffc000000) | 1;
		tlb.tlb_lo0 = vad_to_pfn(0xfffffffff4000000) | PG_IOPAGE;
#else
		tlb.tlb_hi = vad_to_vpn(0xfc000000) | 1;
		tlb.tlb_lo0 = vad_to_pfn(0xf4000000) | PG_IOPAGE;
#endif
		tlb.tlb_lo1 = vad_to_pfn(sys_config.cons_ioaddr[0]) | PG_IOPAGE;
		tlb_write_indexed(2, &tlb);

		if (sys_config.system_type == GALILEO_EV64240) {
			tlb.tlb_mask = PG_SIZE_16M;
			tlb.tlb_hi = vad_to_vpn(0xf8000000) | 1;
			tlb.tlb_lo0 = vad_to_pfn(sys_config.pci_io[0].bus_base) | PG_IOPAGE;
			tlb.tlb_lo1 = vad_to_pfn(sys_config.pci_mem[0].bus_base) | PG_IOPAGE;
			tlb_write_indexed(3, &tlb);
		}
	}

	/*
	 *  Get a console, very early but after initial mapping setup.
	 */
	consinit();

	if (sys_config.system_type < 0) {
		printf("'systype' = '%s' not known!\n", cp ? cp : "NULL");
		panic("unidentified system");
	}


	/*
	 * Allocate U page(s) for proc[0], pm_tlbpid 1.
	 */
	proc0.p_addr = proc0paddr = curprocpaddr =
		(struct user *)pmap_steal_memory(USPACE, NULL,NULL);
	proc0.p_md.md_regs = (struct trap_frame *)&proc0paddr->u_pcb.pcb_regs;
	firstaddr = KSEG0_TO_PHYS(proc0.p_addr);
	tlb_set_pid(1);

	/*
	 * Allocate system data structures.
	 */
	i = (vsize_t)allocsys(NULL);
        sd = (caddr_t)pmap_steal_memory(i, NULL, NULL);
        allocsys(sd);

	/*
	 * Bootstrap VM system.
	 */
	pmap_bootstrap();

	/*
	 * Copy down exception vector code. If code is to large
	 * copy down trampolines instead of doing a panic.
	 */
	if (e_tlb_miss - tlb_miss > 0x100) {
		bcopy(tlb_miss_tramp, (char *)TLB_MISS_EXC_VEC,
		    e_tlb_miss_tramp - tlb_miss_tramp);
		bcopy(xtlb_miss_tramp, (char *)XTLB_MISS_EXC_VEC,
		    e_xtlb_miss_tramp - xtlb_miss_tramp);
	} else {
		bcopy(tlb_miss, (char *)TLB_MISS_EXC_VEC,
		    e_tlb_miss - tlb_miss);
	}

	bcopy(exception, (char *)CACHE_ERR_EXC_VEC, e_exception - exception);

	/*
	 *  Keep PMON2000 exceptions if requested.
	 */
	if(!getenv("pmonexept")) {
		bcopy(exception, (char *)GEN_EXC_VEC, e_exception - exception);
	}

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Clear out the I and D caches.
	 */
	Mips_SyncCache();

	/*
	 * Initialize error message buffer.
	 */
	initmsgbuf((caddr_t)0xffffffff80002000, MSGBUFSIZE);

	/*
	 *  Return new stack pointer.
	 */
	return ((caddr_t)proc0paddr + USPACE - 64);
}

/*
 * Allocate space for system data structures. Doesn't need to be mapped.
 */
caddr_t
allocsys(caddr_t v)
{
	caddr_t start;

	start = v;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

#ifndef BUFCACHEPERCENT
#define BUFCACHEPERCENT 5
#endif

	/*
	 * Determine how many buffers to allocate.
	 */
	if (bufpages == 0) {
		bufpages = (physmem / (100/BUFCACHEPERCENT));
	}
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	/* Restrict to at most 70% filled kvm */
	if (nbuf > (VM_MAX_KERNEL_ADDRESS-VM_MIN_KERNEL_ADDRESS) / MAXBSIZE * 7 / 10) {
		nbuf = (VM_MAX_KERNEL_ADDRESS-VM_MIN_KERNEL_ADDRESS) / MAXBSIZE * 7 / 10;
	}

	/* More buffer pages than fits into the buffers is senseless.  */
	if (bufpages > nbuf * MAXBSIZE / PAGE_SIZE) {
		bufpages = nbuf * MAXBSIZE / PAGE_SIZE;
	}

	if (nswbuf == 0) { 
		nswbuf = (nbuf / 2) &~ 1;	/* even */
		if (nswbuf > 256) {
			nswbuf = 256;
		}
	}

	valloc(buf, struct buf, nbuf);

	/*
	 * Clear allocated memory.
	 */
	if(start != 0) {
		bzero(start, v - start);
	}

	return(v);
}


/*
 * Return a pointer to the given environment variable.
 */
static char *
getenv(envname)
	char *envname;
{
	int32_t *env = environment;
	char *envp;
	int i;

return(NULL);
	i = strlen(envname);

	while(*env) {
		envp = (char *)(long)*env;
		if(strncasecmp(envname, envp, i) == 0 && envp[i] == '=') {
			return(&envp[i+1]);
		}
		env++;
	}
	return(NULL);
}

/*
 *  Decode boot options.
 */
static void
dobootopts(cp)
	char *cp;
{
	while(cp && *cp) {
		switch (*cp++) {
		case 'm': /* multiuser */
			boothowto &= ~RB_SINGLE;
			break;

		case 's': /* singleuser */
			boothowto |= RB_SINGLE;
			break;

		case 'd': /* use compiled in default root */
			boothowto |= RB_DFLTROOT;
			break;

		case 'a': /* ask for names */
			boothowto |= RB_ASKNAME;
			break;

		case 'A': /* don't ask for names */
			boothowto &= ~RB_ASKNAME;
			break;

		case 't': /* use serial console */
			boothowto |= RB_SERCONS;
			break;

		case 'c': /* boot configure */
			boothowto |= RB_CONFIG;
			break;

		case 'B': /* Enter debugger */
			boothowto |= RB_KDB;
			break;
		}

	}
}


/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	if (console_ok) {
		return;
	}
	cninit();
	console_ok = 1;
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
#ifdef DEBUG
	extern int pmapdebugflag;
	int opmapdebugflag = pmapdebugflag;

	pmapdebugflag = 0;	/* Shut up pmap debug during bootstrap */
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("real mem = %d\n", ptoa(physmem));

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
			NULL, UVM_UNKNOWN_OFFSET, 0,
			UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
			UVM_ADV_NORMAL, 0)) != KERN_SUCCESS) {
		panic("cpu_startup: cannot allocate VM for buffers");
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE / PAGE_SIZE) {
		/* don't want to alloc more physical mem than needed */
		base = MAXBSIZE / PAGE_SIZE;
		residual = 0;
	}

	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = PAGE_SIZE * (i < residual ? base+1 : base);

		while (curbufsize) {
			struct vm_page *pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for"
					" buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
					VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr, 16 * NCARGS,
					TRUE, FALSE, NULL);
	/* Allocate a submap for physio */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, TRUE, FALSE, NULL);

	/* Finally, allocate mbuf pool. */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				VM_MBUF_SIZE, FALSE, FALSE, NULL);
#ifdef DEBUG
	pmapdebugflag = opmapdebugflag;
#endif
	printf("avail mem = %d\n", ptoa(uvmexp.free));
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * PAGE_SIZE);
	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	if (boothowto & RB_CONFIG) {
#ifdef BOOT_CONFIG
		user_config();
#else
		printf("kernel does not support -c; continuing..\n");
#endif
	}
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return ENOTDIR;		/* overloaded */

	switch (name[0]) {
	case CPU_ALLOWAPERTURE:
#ifdef APERTURE
	if (securelevel > 0)
		return sysctl_rdint(oldp, oldlenp, newp, allowaperture);
	else
		return sysctl_int(oldp, oldlenp, newp, newlen, &allowaperture);
#else
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
#endif
	default:
		return EOPNOTSUPP;
	}
}

/*
 * Set registers on exec for native exec format. For o64/64.
 */
void
setregs(p, pack, stack, retval)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	extern struct proc *machFPCurProcPtr;
#if 0
/* XXX should check validity of header and perhaps be 32/64 indep. */
        Elf64_Ehdr *eh = pack->ep_hdr;

        if ((((eh->e_flags & EF_MIPS_ABI) != E_MIPS_ABI_NONE) &&
            ((eh->e_flags & EF_MIPS_ABI) != E_MIPS_ABI_O32)) ||
            ((eh->e_flags & EF_MIPS_ARCH) >= E_MIPS_ARCH_3) ||
            (eh->e_ident[EI_CLASS] != ELFCLASS32)) {
		p->p_md.md_flags |= MDP_O32;
        }
#endif

#if !defined(_LP64)
	p->p_md.md_flags |= MDP_O32;
#endif

	bzero((caddr_t)p->p_md.md_regs, sizeof(struct trap_frame));
	p->p_md.md_regs->sp = stack;
	p->p_md.md_regs->pc = pack->ep_entry & ~3;
	p->p_md.md_regs->t9 = pack->ep_entry & ~3; /* abicall req */
#if 0
	p->p_md.md_regs->sr = SR_FR_32|SR_KSU_USER|SR_UX|SR_EXL|SR_INT_ENAB;
#else
	p->p_md.md_regs->sr = SR_KSU_USER|SR_XX|SR_EXL|SR_INT_ENAB;
#endif
	p->p_md.md_regs->sr |= (idle_mask << 8) & SR_INT_MASK;
	p->p_md.md_regs->ic = idle_mask & IC_INT_MASK;
	p->p_md.md_flags &= ~MDP_FPUSED;
	if (machFPCurProcPtr == p)
		machFPCurProcPtr = (struct proc *)0;
	p->p_md.md_ss_addr = 0;
	p->p_md.md_pc_ctrl = 0;
	p->p_md.md_watch_1 = 0;
	p->p_md.md_watch_2 = 0;
}


int	waittime = -1;

void
boot(howto)
	register int howto;
{

	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx(curproc->p_addr, 0);

#ifdef DEBUG
	if (panicstr)
		stacktrace();
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		extern struct proc proc0;
		/* fill curproc with live object */
		if (curproc == NULL)
			curproc = &proc0;
		/*
		 * Synchronize the disks....
		 */
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
	(void) splhigh();		/* extreme priority */

	if (howto & RB_HALT) {
		printf("System halted.\n");
		if(sys_config.system_type == ALGOR_P5064 && howto & RB_POWERDOWN) {
			printf("Shutting off!\n");
			*(int *)(0xffffffffbffa000c) = 1;
		}
		while(1); /* Forever */
	}
	else {
		if (howto & RB_DUMP)
			dumpsys();
		printf("System restart.\n");
#if defined(LP64)
		__asm__(" li $2, 0xffffffff80010100; jr $2; nop\n");
#else
		__asm__(" li $2, 0x80010100; jr $2; nop\n");
#endif
		while(1); /* Forever */
	}
	/*NOTREACHED*/
}

int	dumpmag = (int)0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

void
dumpconf()
{
	int nblks;

	dumpsize = ptoa(physmem);
	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(physmem));
	}
	/*
	 * Don't dump on the first page
	 * in case the dump device includes a disk label.
	 */
	if (dumplo < btodb(PAGE_SIZE))
		dumplo = btodb(PAGE_SIZE);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
void
dumpsys()
{

	msgbufmapped = 0;
	if (dumpdev == NODEV)
		return;
	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
	printf("dump not yet implemented");
#if 0 /* XXX HAVE TO FIX XXX */
	switch (error = (*bdevsw[major(dumpdev)].d_dump)(dumpdev, dumplo,)) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("error %d\n", error);
		break;

	case 0:
		printf("succeeded\n");
	}
#endif
}

void
initcpu()
{
}

/*
 * Convert "xx:xx:xx:xx:xx:xx" string to ethernet hardware address.
 */
static void
get_eth_hw_addr(char *s, char *a)
{
	int i;
	if(s != NULL) {
		for(i = 0; i < 6; i++) {
			a[i] = atoi(s, 16);
			s += 3;		/* Don't get to fancy here :-) */
		}
	}
}

/*
 * Convert an ASCII string into an integer.
 */
static int
atoi(s, b)
	char *s;
	int   b;
{
	int c;
	unsigned base = b, d;
	int neg = 0, val = 0;

	if (s == 0 || (c = *s++) == 0)
		goto out;

	/* skip spaces if any */
	while (c == ' ' || c == '\t')
		c = *s++;

	/* parse sign, allow more than one (compat) */
	while (c == '-') {
		neg = !neg;
		c = *s++;
	}

	/* parse base specification, if any */
	if (c == '0') {
		c = *s++;
		switch (c) {
		case 'X':
		case 'x':
			base = 16;
			c = *s++;
			break;
		case 'B':
		case 'b':
			base = 2;
			c = *s++;
			break;
		default:
			base = 8;
		}
	}

	/* parse number proper */
	for (;;) {
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'a' && c <= 'z')
			d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			d = c - 'A' + 10;
		else
			break;
		val *= base;
		val += d;
		c = *s++;
	}
	if (neg)
		val = -val;
out:
	return val;	
}

/*
 *  RM7000 Performance counter support.
 */

int
rm7k_perfcntr(cmd, arg1, arg2, arg3)
	int cmd;
	long  arg1, arg2, arg3;
{
	int result;
	quad_t cntval;
	struct proc *p = curproc;


	switch(cmd) {
	case PCNT_FNC_SELECT:
		if((arg1 & 0xff) > PCNT_SRC_MAX || 
		   (arg1 & ~(PCNT_CE|PCNT_UM|PCNT_KM|0xff)) != 0) {
			result = EINVAL;
			break;
		}
printf("perfcnt select %x, proc %p\n", arg1, p);
		p->p_md.md_pc_count = 0;
		p->p_md.md_pc_spill = 0;
		p->p_md.md_pc_ctrl = arg1;
		result = 0;
		break;

	case PCNT_FNC_READ:
		cntval = p->p_md.md_pc_count;
		cntval += (quad_t)p->p_md.md_pc_spill << 31;
		result = copyout(&cntval, (void *)arg1, sizeof(cntval));
printf("perfcnt read %d:%d -> %p\n", p->p_md.md_pc_count, p->p_md.md_pc_spill, arg1);
		break;

	default:
printf("perfcnt error %d\n", cmd);
		result = -1;
		break;
	}
	return(result);
}

/*
 *  Called when the performance counter d31 gets set.
 *  Increase spill value and reset d31.
 */
void
rm7k_perfintr(trapframe)
	struct trap_frame *trapframe;
{
	struct proc *p = curproc;

	printf("perfintr proc %p!\n", p);
	cp0_setperfcount(cp0_getperfcount() & 0x7fffffff);
	if(p != NULL) {
		p->p_md.md_pc_spill++;
	}
	
}

int
rm7k_watchintr(trapframe)
	struct trap_frame *trapframe;
{
	return(0);
}
