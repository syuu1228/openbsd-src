/*	$OpenBSD: src/lib/libcurses/Attic/lib_touch.c,v 1.3 1997/12/03 05:21:35 millert Exp $	*/


/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          Zeyd M. Ben-Halim                               *
*                          zmbenhal@netcom.com                             *
*                          Eric S. Raymond                                 *
*                          esr@snark.thyrsus.com                           *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, and is not    *
*        removed from any of its header files. Mention of ncurses in any   *
*        applications linked with it is highly appreciated.                *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/

/*
**	lib_touch.c
**
**	   The routines	untouchwin(),
**			wtouchln(),
**			is_linetouched()
**			is_wintouched().
**
*/

#include <curses.priv.h>

MODULE_ID("Id: lib_touch.c,v 1.4 1997/09/19 22:16:33 juergen Exp $")

int is_linetouched(WINDOW *win, int line)
{
	T((T_CALLED("is_linetouched(%p,%d)"), win, line));

	/* XSI doesn't define any error */
	if (!win || (line > win->_maxy) || (line < 0))
		returnCode(ERR);

	returnCode(win->_line[line].firstchar != _NOCHANGE ? TRUE : FALSE);
}

int is_wintouched(WINDOW *win)
{
int i;

	T((T_CALLED("is_wintouched(%p)"), win));

	if (win)
	        for (i = 0; i <= win->_maxy; i++)
		        if (win->_line[i].firstchar != _NOCHANGE)
			        returnCode(TRUE);
	returnCode(FALSE);
}

int wtouchln(WINDOW *win, int y, int n, int changed)
{
int i;

	T((T_CALLED("wtouchln(%p,%d,%d,%d)"), win, y, n, changed));

	if (!win || (n<0) || (y<0) || (y>win->_maxy))
	  returnCode(ERR);

	for (i = y; i < y+n; i++) {
	        if (i>win->_maxy) break;
		win->_line[i].firstchar = changed ? 0 : _NOCHANGE;
		win->_line[i].lastchar = changed ? win->_maxx : _NOCHANGE;
	}
	returnCode(OK);
}
