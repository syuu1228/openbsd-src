/*	$OpenBSD: src/regress/usr.bin/xlint/test-14.c,v 1.1 2005/12/12 23:37:51 cloder Exp $	*/

/*
 * Placed in the public domain by Chad Loder <cloder@openbsd.org>.
 *
 * Test lint warnings regarding constant in conditional contexts.
 */

/* ARGSUSED */
int
main(int argc, char *argv[])
{
	do {
		argc++;
	} while (0);			/* do not warn */

	do {
		if (argc++)
			break;
	} while (1);			/* do not warn */


	do {
		if (argc++)
			break;
	} while (2);			/* warn because of 2 */

	if (0) {			/* do not warn */
		argc++;
	}

	if (1) {			/* do not warn */
		argc++;
	}

	if (argc && 1) {		/* warn because of compound expression */
		argc++;
	}

	return 0;
}


