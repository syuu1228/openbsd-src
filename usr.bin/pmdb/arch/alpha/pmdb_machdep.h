/*	$OpenBSD: src/usr.bin/pmdb/arch/alpha/pmdb_machdep.h,v 1.2 2002/03/15 18:33:03 mickey Exp $	*/

#define BREAKPOINT		{ 0x80, 0, 0, 0 }
#define BREAKPOINT_LEN		4
#define BREAKPOINT_DECR_PC	4
