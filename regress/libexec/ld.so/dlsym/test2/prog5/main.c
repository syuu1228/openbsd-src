/*	$OpenBSD: src/regress/libexec/ld.so/dlsym/test2/prog5/main.c,v 1.1.1.1 2005/09/19 03:23:51 kurt Exp $	*/

/*
 * Copyright (c) 2005 Kurt Miller <kurt@openbsd.org>
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

#include <dlfcn.h>
#include <stdio.h>

int mainSymbol;

/*
 * confirm that libcc gets promoted to RTLD_GLOBAL when opened as part of
 * libbb with RTLD_GLOBAL
 */
int
main()
{
	int ret = 0;
	void *libcc = dlopen("libcc.so", RTLD_LAZY);
	void *libbb = dlopen("libbb.so", RTLD_LAZY|RTLD_GLOBAL);

	if (libbb == NULL) {
		printf("dlopen(\"libbb.so\", RTLD_LAZY) FAILED\n");
		return (1);
	}

	if (libcc == NULL) {
		printf("dlopen(\"libcc.so\", RTLD_LAZY) FAILED\n");
		return (1);
	}

	/* RTLD_DEFAULT should see ccSymbol */
	if (dlsym(RTLD_DEFAULT, "ccSymbol") == NULL) {
		printf("dlsym(RTLD_DEFAULT, \"ccSymbol\") == NULL\n");
		ret = 1;
	}

	dlclose(libbb);
	dlclose(libcc);

	return (ret);
}
