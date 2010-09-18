/*	$OpenBSD: src/regress/lib/libc/gcvt/gcvt_test.c,v 1.4 2010/09/18 20:29:15 millert Exp $	*/

/*
 * Public domain, 2010, Todd C. Miller <Todd.Miller@courtesan.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct test_vector {
	double d;
	int ndig;
	char *expect;
} test_vectors[] = {
	/* adapted from perl's Configure test */
	{ 0.1, 8, "0.1" },
	{ 0.01, 8, "0.01" },
	{ 0.001, 8, "0.001" },
	{ 0.0001, 8, "0.0001" },
	{ 0.00009, 8, "9e-05" },
	{ 1.0, 8, "1" },
	{ 1.1, 8, "1.1" },
	{ 1.01, 8, "1.01" },
	{ 1.001, 8, "1.001" },
	{ 1.0001, 8, "1.0001" },
	{ 1.00001, 8, "1.00001" },
	{ 1.000001, 8, "1.000001" },
	{ 0.0, 8, "0" },
	{ -1.0, 8, "-1" },
	{ 100000.0, 8, "100000" },
	{ -100000.0, 8, "-100000" },
	{ 123.456, 8, "123.456" },
	{ 1e34, 8, "1e+34" },
	/* adapted from emx */
	{ 0.0, -1, "0" },
	{ 0.0, 0, "0" },
	{ 0.0, 1, "0" },
	{ 0.0, 2, "0" },
	{ 1.0, -1, "1" },
	{ 1.0, 0, "1" },
	{ 1.0, 2, "1" },
	{ 1.0, 10, "1" },
	{ 1.236, -1, "1.236" },
	{ 1.236, 0, "1" },
	{ 1.236, 1, "1" },
	{ 1.236, 2, "1.2" },
	{ 1.236, 3, "1.24" },
	{ 1.236, 4, "1.236" },
	{ 1.236, 5, "1.236" },
	{ 1.236, 6, "1.236" },
	{ 12.36, -1, "12.36" },
	{ 12.36, 0, "1e+01" },
	{ 12.36, 1, "1e+01" },
	{ 12.36, 2, "12" },
	{ 12.36, 3, "12.4" },
	{ 12.36, 4, "12.36" },
	{ 12.36, 5, "12.36" },
	{ 12.36, 6, "12.36" },
	{ 123.6, -1, "123.6" },
	{ 123.6, 0, "1e+02" },
	{ 123.6, 1, "1e+02" },
	{ 123.6, 2, "1.2e+02" },
	{ 123.6, 3, "124" },
	{ 123.6, 4, "123.6" },
	{ 123.6, 5, "123.6" },
	{ 123.6, 6, "123.6" },
	{ 1236.0, -1, "1236" },
	{ 1236.0, 0, "1e+03" },
	{ 1236.0, 1, "1e+03" },
	{ 1236.0, 2, "1.2e+03" },
	{ 1236.0, 3, "1.24e+03" },
	{ 1236.0, 4, "1236" },
	{ 1236.0, 5, "1236" },
	{ 1236.0, 6, "1236" },
	{ 1e100, 10, "1e+100" },
	{ 1e100, 20, "1.0000000000000000159e+100" },
	{ 0.01236, -1, "0.01236" },
	{ 0.01236, 0, "0.01" },
	{ 0.01236, 1, "0.01" },
	{ 0.01236, 2, "0.012" },
	{ 0.01236, 3, "0.0124" },
	{ 0.01236, 4, "0.01236" },
	{ 1e-100, 20, "1.00000000000000002e-100" },
	{ 1e-100, -1, "1e-100" },
	{ -1.2, 5, "-1.2" },
	{ -0.03, 5, "-0.03" },
	{ 0.1, 1, "0.1" },
	{ 0.1, 0, "0.1" },
	{ 0.099999, 10, "0.099999" },
	{ 0.99999, 10, "0.99999" },
};

#define NTESTVEC (sizeof(test_vectors) / sizeof(test_vectors[0]))

static int
dotest(struct test_vector *tv)
{
	char buf[256], *got;

	got = gcvt(tv->d, tv->ndig, buf);
	if (strcmp(tv->expect, got) != 0) {
		fprintf(stderr, "%g @ %d: expected %s, got %s\n",
		    tv->d, tv->ndig, tv->expect, got);
		return 1;
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int i, failures = 0;

	for (i = 0; i < NTESTVEC; i++) {
		failures += dotest(&test_vectors[i]);
	}

	return failures;
}
