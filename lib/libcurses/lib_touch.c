/*	$OpenBSD: src/lib/libcurses/Attic/lib_touch.c,v 1.4 1998/07/23 21:19:37 millert Exp $	*/

/****************************************************************************
 * Copyright (c) 1998 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/

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

MODULE_ID("$From: lib_touch.c,v 1.6 1998/04/11 22:55:02 tom Exp $")

bool is_linetouched(WINDOW *win, int line)
{
	T((T_CALLED("is_linetouched(%p,%d)"), win, line));

	/* XSI doesn't define any error */
	if (!win || (line > win->_maxy) || (line < 0))
		returnCode(ERR);

	returnCode(win->_line[line].firstchar != _NOCHANGE ? TRUE : FALSE);
}

bool is_wintouched(WINDOW *win)
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
