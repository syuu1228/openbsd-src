/*	$OpenBSD: src/usr.bin/nm/nm.c,v 1.17 2003/04/05 17:15:06 deraadt Exp $	*/
/*	$NetBSD: nm.c,v 1.7 1996/01/14 23:04:03 pk Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hans Huebner.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)nm.c	8.1 (Berkeley) 6/6/93";
#endif
static char rcsid[] = "$OpenBSD: src/usr.bin/nm/nm.c,v 1.17 2003/04/05 17:15:06 deraadt Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <a.out.h>
#include <stab.h>
#include <ar.h>
#include <ranlib.h>
#include <unistd.h>
#include <err.h>
#include <ctype.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* XXX get shared code to handle byte-order swaps */
#include "byte.c"


int demangle = 0;
int ignore_bad_archive_entries = 1;
int print_only_external_symbols;
int print_only_undefined_symbols;
int print_all_symbols;
int print_file_each_line;
int show_extensions = 0;
int fcount;

int rev;
int fname(const void *, const void *);
int rname(const void *, const void *);
int value(const void *, const void *);
int (*sfunc)(const void *, const void *) = fname;
char *otherstring(struct nlist *);
char *typestring(unsigned int);
char typeletter(unsigned int);


/* some macros for symbol type (nlist.n_type) handling */
#define	IS_DEBUGGER_SYMBOL(x)	((x) & N_STAB)
#define	IS_EXTERNAL(x)		((x) & N_EXT)
#define	SYMBOL_TYPE(x)		((x) & (N_TYPE | N_STAB))

void	*emalloc(size_t);
void	 pipe2cppfilt(void);
void	 usage(void);
char	*symname(struct nlist *);
void 	print_symbol(const char *, struct nlist *);

/*
 * main()
 *	parse command line, execute process_file() for each file
 *	specified on the command line.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	int ch, errors;

	while ((ch = getopt(argc, argv, "aBCegnopruw")) != -1) {
		switch (ch) {
		case 'a':
			print_all_symbols = 1;
			break;
		case 'B':
			/* no-op, compat with gnu-nm */
			break;
		case 'C':
			demangle = 1;
			break;
		case 'e':
			show_extensions = 1;
			break;
		case 'g':
			print_only_external_symbols = 1;
			break;
		case 'n':
			sfunc = value;
			break;
		case 'o':
			print_file_each_line = 1;
			break;
		case 'p':
			sfunc = NULL;
			break;
		case 'r':
			rev = 1;
			break;
		case 'u':
			print_only_undefined_symbols = 1;
			break;
		case 'w':
			ignore_bad_archive_entries = 0;
			break;
		case '?':
		default:
			usage();
		}
	}

	if (demangle)
		pipe2cppfilt();
	fcount = argc - optind;
	argv += optind;

	if (rev && sfunc == fname)
		sfunc = rname;

	if (!fcount)
		errors = process_file("a.out");
	else {
		errors = 0;
		do {
			errors |= process_file(*argv);
		} while (*++argv);
	}
	exit(errors);
}

/*
 * process_file()
 *	show symbols in the file given as an argument.  Accepts archive and
 *	object files as input.
 */
process_file(fname)
	char *fname;
{
	struct exec exec_head;
	FILE *fp;
	int retval;
	char magic[SARMAG];
    
	if (!(fp = fopen(fname, "r"))) {
		warn("cannot read %s", fname);
		return(1);
	}

	if (fcount > 1)
		(void)printf("\n%s:\n", fname);
    
	/*
	 * first check whether this is an object file - read a object
	 * header, and skip back to the beginning
	 */
	if (fread((char *)&exec_head, sizeof(exec_head), (size_t)1, fp) != 1) {
		warnx("%s: bad format", fname);
		(void)fclose(fp);
		return(1);
	}
	rewind(fp);

	if (BAD_OBJECT(exec_head)) {
	/* this could be an archive */
		if (fread(magic, sizeof(magic), (size_t)1, fp) != 1 ||
		    strncmp(magic, ARMAG, SARMAG)) {
			warnx("%s: not object file or archive", fname);
			(void)fclose(fp);
			return(1);
		}
		retval = show_archive(fname, fp);
	} else
		retval = show_objfile(fname, fp);
	(void)fclose(fp);
	return(retval);
}

/*
 * show_archive()
 *	show symbols in the given archive file
 */
show_archive(fname, fp)
	char *fname;
	FILE *fp;
{
	struct ar_hdr ar_head;
	struct exec exec_head;
	int i, rval;
	long last_ar_off;
	char *p, *name;
	int baselen, namelen;

	baselen = strlen(fname) + 3;
	namelen = sizeof(ar_head.ar_name);
	name = emalloc(baselen + namelen);

	rval = 0;

	/* while there are more entries in the archive */
	while (fread((char *)&ar_head, sizeof(ar_head), (size_t)1, fp) == 1) {
		/* bad archive entry - stop processing this archive */
		if (strncmp(ar_head.ar_fmag, ARFMAG, sizeof(ar_head.ar_fmag))) {
			warnx("%s: bad format archive header", fname);
			(void)free(name);
			return(1);
		}

		/* remember start position of current archive object */
		last_ar_off = ftell(fp);

		/* skip ranlib entries */
		if (!strncmp(ar_head.ar_name, RANLIBMAG, sizeof(RANLIBMAG) - 1))
			goto skip;

		/*
		 * construct a name of the form "archive.a:obj.o:" for the
		 * current archive entry if the object name is to be printed
		 * on each output line
		 */
		p = name;
		if (print_file_each_line) {
			snprintf(p, baselen, "%s:", fname);
			p += strlen(p);
		}
#ifdef AR_EFMT1
		/*
		 * BSD 4.4 extended AR format: #1/<namelen>, with name as the
		 * first <namelen> bytes of the file
		 */
		if (		(ar_head.ar_name[0] == '#') &&
				(ar_head.ar_name[1] == '1') &&
				(ar_head.ar_name[2] == '/') && 
				(isdigit(ar_head.ar_name[3]))) {

			int len = atoi(&ar_head.ar_name[3]);
			if (len > namelen) {
				p -= (long)name;
				if ((name = realloc(name, baselen+len)) == NULL)
					err(1, NULL);
				namelen = len;
				p += (long)name;
			}
			if (fread(p, len, 1, fp) != 1) {
				warnx("%s: premature EOF", name);
				(void)free(name);
				return 1;
			}
			p += len;
		} else
#endif
		for (i = 0; i < sizeof(ar_head.ar_name); ++i)
			if (ar_head.ar_name[i] && ar_head.ar_name[i] != ' ')
				*p++ = ar_head.ar_name[i];
		*p++ = '\0';

		/* get and check current object's header */
		if (fread((char *)&exec_head, sizeof(exec_head),
		    (size_t)1, fp) != 1) {
			warnx("%s: premature EOF", name);
			(void)free(name);
			return(1);
		}

		if (BAD_OBJECT(exec_head)) {
			if (!ignore_bad_archive_entries) {
				 warnx("%s: bad format", name);
				rval = 1;
			}
		} else {
			(void)fseek(fp, (long)-sizeof(exec_head), SEEK_CUR);
			if (!print_file_each_line)
				(void)printf("\n%s:\n", name);
			rval |= show_objfile(name, fp);
		}

		/*
		 * skip to next archive object - it starts at the next
	 	 * even byte boundary
		 */
#define even(x) (((x) + 1) & ~1)
skip:		if (fseek(fp, last_ar_off + even(atol(ar_head.ar_size)),
		    SEEK_SET)) {
			warn("%s", fname);
			(void)free(name);
			return(1);
		}
	}
	(void)free(name);
	return(rval);
}

/*
 * show_objfile()
 *	show symbols from the object file pointed to by fp.  The current
 *	file pointer for fp is expected to be at the beginning of an a.out
 *	header.
 */
show_objfile(objname, fp)
	char *objname;
	FILE *fp;
{
	struct nlist *names, *np;
	struct nlist **snames;
	int i, nnames, nrawnames;
	struct exec head;
	long stabsize;
	char *stab;

	/* read a.out header */
	if (fread((char *)&head, sizeof(head), (size_t)1, fp) != 1) {
		warnx("%s: cannot read header", objname);
		return(1);
	}

	/*
	 * skip back to the header - the N_-macros return values relative
	 * to the beginning of the a.out header
	 */
	if (fseek(fp, (long)-sizeof(head), SEEK_CUR)) {
		warn("%s", objname);
		return(1);
	}

	/* stop if this is no valid object file, or a format we don't dare
	 * playing with
	 */
	if (BAD_OBJECT(head)) {
		warnx("%s: bad format", objname);
		return(1);
	}

	fix_header_order(&head);

	/* stop if the object file contains no symbol table */
	if (!head.a_syms) {
		warnx("%s: no name list", objname);
		return(1);
	}

	if (fseek(fp, (long)N_SYMOFF(head), SEEK_CUR)) {
		warn("%s", objname);
		return(1);
	}

	/* get memory for the symbol table */
	names = emalloc((size_t)head.a_syms);
	nrawnames = head.a_syms / sizeof(*names);
	snames = emalloc(nrawnames*sizeof(struct nlist *));
	if (fread((char *)names, (size_t)head.a_syms, (size_t)1, fp) != 1) {
		warnx("%s: cannot read symbol table", objname);
		(void)free((char *)names);
		return(1);
	}
	fix_nlists_order(names, nrawnames, N_GETMID(head));

	/*
	 * Following the symbol table comes the string table.  The first
	 * 4-byte-integer gives the total size of the string table
	 * _including_ the size specification itself.
	 */
	if (fread((char *)&stabsize, sizeof(stabsize), (size_t)1, fp) != 1) {
		warnx("%s: cannot read stab size", objname);
		(void)free((char *)names);
		return(1);
	}
	stabsize = fix_long_order(stabsize, N_GETMID(head));
	stab = emalloc((size_t)stabsize);

	/*
	 * read the string table offset by 4 - all indices into the string
	 * table include the size specification.
	 */
	stabsize -= 4;		/* we already have the size */
	if (fread(stab + 4, (size_t)stabsize, (size_t)1, fp) != 1) {
		warnx("%s: stab truncated..", objname);
		(void)free((char *)names);
		(void)free(stab);
		return(1);
	}

	/*
	 * fix up the symbol table and filter out unwanted entries
	 *
	 * common symbols are characterized by a n_type of N_UNDF and a
	 * non-zero n_value -- change n_type to N_COMM for all such
	 * symbols to make life easier later.
	 *
	 * filter out all entries which we don't want to print anyway
	 */
	for (np = names, i = nnames = 0; i < nrawnames; np++, i++) {
		/*
		 * make n_un.n_name a character pointer by adding the string
		 * table's base to n_un.n_strx
		 *
		 * don't mess with zero offsets
		 */
		if (np->n_un.n_strx)
			np->n_un.n_name = stab + np->n_un.n_strx;
		else
			np->n_un.n_name = "";
		if (SYMBOL_TYPE(np->n_type) == N_UNDF && np->n_value)
			np->n_type = N_COMM | (np->n_type & N_EXT);
		if (!print_all_symbols && IS_DEBUGGER_SYMBOL(np->n_type))
			continue;
		if (print_only_external_symbols && !IS_EXTERNAL(np->n_type))
			continue;
		if (print_only_undefined_symbols &&
		    SYMBOL_TYPE(np->n_type) != N_UNDF)
			continue;

		snames[nnames++] = np;
	}

	/* sort the symbol table if applicable */
	if (sfunc)
		qsort(snames, (size_t)nnames, sizeof(*snames), sfunc);

	/* print out symbols */
	for (i = 0; i < nnames; i++) {
		if (show_extensions && snames[i] != names &&
		    SYMBOL_TYPE((snames[i] -1)->n_type) == N_INDR)
			continue;
		print_symbol(objname, snames[i]);
	}

	(void)free(snames);
	(void)free(names);
	(void)free(stab);
	return(0);
}

char *
symname(sym)
	struct nlist *sym;
{
	if (demangle && sym->n_un.n_name[0] == '_') 
		return sym->n_un.n_name + 1;
	else
		return sym->n_un.n_name;
}

/*
 * print_symbol()
 *	show one symbol
 */
void
print_symbol(objname, sym)
	const char *objname;
	struct nlist *sym;
{
	if (print_file_each_line)
		(void)printf("%s:", objname);

	/*
	 * handle undefined-only format especially (no space is
	 * left for symbol values, no type field is printed)
	 */
	if (!print_only_undefined_symbols) {
		/* print symbol's value */
		if (SYMBOL_TYPE(sym->n_type) == N_UNDF || 
		    (show_extensions && SYMBOL_TYPE(sym->n_type) == N_INDR && 
		     sym->n_value == 0))
			(void)printf("        ");
		else
			(void)printf("%08lx", sym->n_value);

		/* print type information */
		if (IS_DEBUGGER_SYMBOL(sym->n_type))
			(void)printf(" - %02x %04x %5s ", sym->n_other,
			    sym->n_desc&0xffff, typestring(sym->n_type));
		else if (show_extensions)
			(void)printf(" %c%2s ", typeletter(sym->n_type),
			    otherstring(sym));
		else
			(void)printf(" %c ", typeletter(sym->n_type));
	}

	if (SYMBOL_TYPE(sym->n_type) == N_INDR && show_extensions) {
		printf("%s -> %s\n", symname(sym), symname(sym+1));
	}
	else
		(void)puts(symname(sym));
}

char *
otherstring(sym)
	struct nlist *sym;
{
	static char buf[3];
	char *result;

	result = buf;

	if (N_BIND(sym) == BIND_WEAK)
		*result++ = 'w';
	if (N_AUX(sym) == AUX_OBJECT)
		*result++ = 'o';
	else if (N_AUX(sym) == AUX_FUNC)
		*result++ = 'f';
	*result++ = 0;
	return buf;
}

/*
 * typestring()
 *	return the a description string for an STAB entry
 */
char *
typestring(type)
	unsigned int type;
{
	switch(type) {
	case N_BCOMM:
		return("BCOMM");
	case N_ECOML:
		return("ECOML");
	case N_ECOMM:
		return("ECOMM");
	case N_ENTRY:
		return("ENTRY");
	case N_FNAME:
		return("FNAME");
	case N_FUN:
		return("FUN");
	case N_GSYM:
		return("GSYM");
	case N_LBRAC:
		return("LBRAC");
	case N_LCSYM:
		return("LCSYM");
	case N_LENG:
		return("LENG");
	case N_LSYM:
		return("LSYM");
	case N_PC:
		return("PC");
	case N_PSYM:
		return("PSYM");
	case N_RBRAC:
		return("RBRAC");
	case N_RSYM:
		return("RSYM");
	case N_SLINE:
		return("SLINE");
	case N_SO:
		return("SO");
	case N_SOL:
		return("SOL");
	case N_SSYM:
		return("SSYM");
	case N_STSYM:
		return("STSYM");
	}
	return("???");
}

/*
 * typeletter()
 *	return a description letter for the given basic type code of an
 *	symbol table entry.  The return value will be upper case for
 *	external, lower case for internal symbols.
 */
char
typeletter(type)
	unsigned int type;
{
	switch(SYMBOL_TYPE(type)) {
	case N_ABS:
		return(IS_EXTERNAL(type) ? 'A' : 'a');
	case N_BSS:
		return(IS_EXTERNAL(type) ? 'B' : 'b');
	case N_COMM:
		return(IS_EXTERNAL(type) ? 'C' : 'c');
	case N_DATA:
		return(IS_EXTERNAL(type) ? 'D' : 'd');
	case N_FN:
		/* NOTE: N_FN == N_WARNING,
		 * in this case, the N_EXT bit is to considered as
		 * part of the symbol's type itself.
		 */
		return(IS_EXTERNAL(type) ? 'F' : 'W');
	case N_TEXT:
		return(IS_EXTERNAL(type) ? 'T' : 't');
	case N_INDR:
		return(IS_EXTERNAL(type) ? 'I' : 'i');
	case N_SIZE:
		return(IS_EXTERNAL(type) ? 'S' : 's');
	case N_UNDF:
		return(IS_EXTERNAL(type) ? 'U' : 'u');
	}
	return('?');
}

int
fname(a0, b0)
	const void *a0, *b0;
{
	struct nlist * const *a = a0, * const *b = b0;

	return(strcmp((*a)->n_un.n_name, (*b)->n_un.n_name));
}

int
rname(a0, b0)
	const void *a0, *b0;
{
	struct nlist * const *a = a0, * const *b = b0;

	return(strcmp((*b)->n_un.n_name, (*a)->n_un.n_name));
}

int
value(a0, b0)
	const void *a0, *b0;
{
	struct nlist * const *a = a0, * const *b = b0;

	if (SYMBOL_TYPE((*a)->n_type) == N_UNDF)
		if (SYMBOL_TYPE((*b)->n_type) == N_UNDF)
			return(0);
		else
			return(-1);
	else if (SYMBOL_TYPE((*b)->n_type) == N_UNDF)
		return(1);
	if (rev) {
		if ((*a)->n_value == (*b)->n_value)
			return(rname(a0, b0));
		return((*b)->n_value > (*a)->n_value ? 1 : -1);
	} else {
		if ((*a)->n_value == (*b)->n_value)
			return(fname(a0, b0));
		return((*a)->n_value > (*b)->n_value ? 1 : -1);
	}
}

void *
emalloc(size)
	size_t size;
{
	char *p;

	/* NOSTRICT */
	if (p = malloc(size))
		return(p);
	err(1, NULL);
}

#define CPPFILT	"/usr/bin/c++filt"

void
pipe2cppfilt()
{
	int pip[2];
	char *argv[2];

	argv[0] = "c++filt";
	argv[1] = NULL;

	if (pipe(pip) == -1)
		err(1, "pipe");
	switch(fork()) {
	case -1:
		err(1, "fork");
	default:
		dup2(pip[0], 0);
		close(pip[0]);
		close(pip[1]);
		execve(CPPFILT, argv, NULL);
		err(1, "execve");
	case 0:
		dup2(pip[1], 1);
		close(pip[1]);
		close(pip[0]);
	}
}

void
usage()
{
	(void)fprintf(stderr, "usage: nm [-aCgnopruw] [file ...]\n");
	exit(1);
}
