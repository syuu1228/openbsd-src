/*	$OpenBSD: src/games/battlestar/init.c,v 1.7 2000/09/24 21:55:25 pjanzen Exp $	*/
/*	$NetBSD: init.c,v 1.4 1995/03/21 15:07:35 cgd Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#if 0
static char sccsid[] = "@(#)init.c	8.4 (Berkeley) 4/30/95";
#else
static char rcsid[] = "$OpenBSD: src/games/battlestar/init.c,v 1.7 2000/09/24 21:55:25 pjanzen Exp $";
#endif
#endif /* not lint */

#include "extern.h"

static int checkout __P((const char *));
static const char *getutmp __P((void));
static int wizard __P((const char *));

void
initialize(filename)
	const char    *filename;
{
	const struct objs *p;
	char *savefile;

	puts("Version 4.2, fall 1984.");
	puts("First Adventure game written by His Lordship, the honorable");
	puts("Admiral D.W. Riggle\n");
	location = dayfile;
	srandom(getpid());
	username = getutmp();
	wordinit();
	if (filename == NULL) {
		direction = NORTH;
		ourtime = 0;
		snooze = CYCLE * 1.5;
		position = 22;
		SetBit(wear, PAJAMAS);
		fuel = TANKFULL;
		torps = TORPEDOES;
		for (p = dayobjs; p->room != 0; p++)
			SetBit(location[p->room].objects, p->obj);
	} else {
		savefile = save_file_name(filename, strlen(filename));
		restore(savefile);
		free(savefile);
	}
	wiz = wizard(username);
	signal(SIGINT, die);
}

static const char *
getutmp()
{
	struct passwd *ptr;

	ptr = getpwuid(getuid());
	if (ptr == NULL)
		return("");
	else
		return(strdup(ptr->pw_name));
}

/* Hereditary wizards.  A configuration file might make more sense. */
static const char *const list[] = {
	"riggle",
	"chris",
	"edward",
	"comay",
	"yee",
	"dmr",
	"ken",
	0
};

static const char *const badguys[] = {
	"wnj",
	"root",
	"ted",
	0
};

static int
wizard(username)
	const char   *username;
{
	int     flag;

	if ((flag = checkout(username)) != 0)
		printf("You are the Great wizard %s.\n", username);
	return flag;
}

static int
checkout(username)
	const char   *username;
{
	const char  *const *ptr;

	for (ptr = list; *ptr; ptr++)
		if (strcmp(*ptr, username) == 0)
			return 1;
	for (ptr = badguys; *ptr; ptr++)
		if (strcmp(*ptr, username) == 0) {
			printf("You are the Poor anti-wizard %s.  Good Luck!\n",
			    username);
			if (location != NULL) {
				CUMBER = 3;
				WEIGHT = 9;	/* that'll get him! */
				ourclock = 10;
				SetBit(location[7].objects, WOODSMAN);	/* viper room */
				SetBit(location[20].objects, WOODSMAN);	/* laser " */
				SetBit(location[13].objects, DARK);	/* amulet " */
				SetBit(location[8].objects, ELF);	/* closet */
			}
			return 0;	/* anything else, Chris? */
		}
	return 0;
}
