/*	$OpenBSD: src/usr.bin/patch/Attic/INTERN.h,v 1.4 2003/07/21 14:32:21 deraadt Exp $	*/

#ifdef EXT
#undef EXT
#endif
#define EXT

#ifdef INIT
#undef INIT
#endif
#define INIT(x) = x

#define DOINIT
