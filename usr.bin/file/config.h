/*
 * Hand-made config.h file for OpenBSD, so we don't have to run
 * the dratted configure script every time we build this puppy,
 * but can still carefully import stuff from Christos' version.
 *
 * This file is in the public domain. Original Author Ian F. Darwin.
 * $OpenBSD: src/usr.bin/file/config.h,v 1.6 2011/01/10 20:59:42 deraadt Exp $
 */

/* header file issues. */
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_LOCALE_H 1
#define HAVE_SYS_STAT_H 1
#define	HAVE_INTTYPES_H 1
#define HAVE_GETOPT_H 1
#define HAVE_LIMITS_H 1
/* #define	HAVE_ZLIB_H	1	DO NOT ENABLE YET -- chl */
/* #define	HAVE_LIBZ	1	DO NOT ENABLE YET -- ian */

#define HAVE_STRTOUL
#define HAVE_STRERROR
#define HAVE_VSNPRINTF
#define HAVE_SNPRINTF

/* Compiler issues */
#define SIZEOF_LONG_LONG 8

/* Library issues */
#define HAVE_GETOPT_LONG 1	/* in-tree as of 3.2 */
#define HAVE_ST_RDEV 1

/* ELF support */
#define BUILTIN_ELF 1
#define ELFCORE 1
