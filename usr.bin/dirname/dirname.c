/*	$OpenBSD: src/usr.bin/dirname/dirname.c,v 1.10 2003/07/10 00:06:50 david Exp $	*/

/*
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static char rcsid[] = "$OpenBSD: src/usr.bin/dirname/dirname.c,v 1.10 2003/07/10 00:06:50 david Exp $";                                         
#endif /* not lint */                                                      

#include <err.h>
#include <libgen.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char *dir;
	extern char *__progname;

	setlocale(LC_ALL, "");

	if (argc != 2) {
		(void)fprintf(stderr, "Usage: %s pathname\n", __progname);
		exit(1);
	}

	if ((dir = dirname(argv[1])) == NULL)
		err(1, NULL);
	puts(dir);
	exit(0);
}
