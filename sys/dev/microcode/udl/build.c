/*	$OpenBSD: src/sys/dev/microcode/udl/build.c,v 1.1 2009/08/25 19:04:49 mglocker Exp $ */

/*
 * Copyright (c) 2009 Marcus Glocker <mglocker@openbsd.org>
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

#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include "udl_huffman.h"

#define FILENAME "udl_huffman"

int
main(void)
{
	int fd, i;

	fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		err(1, "%s", FILENAME);

	for (i = 0; i < UDL_HUFFMAN_RECORDS; i++) {
		udl_huffman[i].value = htobe32(udl_huffman[i].value);
		write(fd, &udl_huffman[i], 5);
	}

	close(fd);

	return (0);
}
