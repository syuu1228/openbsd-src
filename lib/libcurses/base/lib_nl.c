/*	$OpenBSD: src/lib/libcurses/base/lib_nl.c,v 1.5 2001/01/22 18:01:42 millert Exp $	*/

/****************************************************************************
 * Copyright (c) 1998,1999,2000 Free Software Foundation, Inc.              *
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
 *	nl.c
 *
 *	Routines:
 *		nl()
 *		nonl()
 *
 */

#include <curses.priv.h>

MODULE_ID("$From: lib_nl.c,v 1.8 2000/12/10 02:43:27 tom Exp $")

#ifdef __EMX__
#include <io.h>
#endif

NCURSES_EXPORT(int)
nl(void)
{
    T((T_CALLED("nl()")));

    SP->_nl = TRUE;

#ifdef __EMX__
    _nc_flush();
    _fsetmode(NC_OUTPUT, "t");
#endif

    returnCode(OK);
}

NCURSES_EXPORT(int)
nonl(void)
{
    T((T_CALLED("nonl()")));

    SP->_nl = FALSE;

#ifdef __EMX__
    _nc_flush();
    _fsetmode(NC_OUTPUT, "b");
#endif

    returnCode(OK);
}
