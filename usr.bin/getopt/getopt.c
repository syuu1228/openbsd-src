/*	$OpenBSD: src/usr.bin/getopt/getopt.c,v 1.2 1996/06/26 05:33:45 deraadt Exp $	*/

#ifndef lint
static char rcsid[] = "$OpenBSD: src/usr.bin/getopt/getopt.c,v 1.2 1996/06/26 05:33:45 deraadt Exp $";
#endif /* not lint */

#include <stdio.h>

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int c;
	int status = 0;

	optind = 2;	/* Past the program name and the option letters. */
	while ((c = getopt(argc, argv, argv[1])) != EOF)
		switch (c) {
		case '?':
			status = 1;	/* getopt routine gave message */
			break;
		default:
			if (optarg != NULL)
				printf(" -%c %s", c, optarg);
			else
				printf(" -%c", c);
			break;
		}
	printf(" --");
	for (; optind < argc; optind++)
		printf(" %s", argv[optind]);
	printf("\n");
	exit(status);
}
