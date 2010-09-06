/* $OpenBSD: src/lib/libcurses/widechar/lib_in_wch.c,v 1.1 2010/09/06 17:26:17 nicm Exp $ */

/****************************************************************************
 * Copyright (c) 2002-2004,2006 Free Software Foundation, Inc.              *
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
 * Author: Thomas Dickey                                                    *
 ****************************************************************************/

/*
**	lib_in_wch.c
**
**	The routine win_wch().
**
*/

#include <curses.priv.h>

MODULE_ID("$Id: lib_in_wch.c,v 1.4 2006/09/03 15:41:22 tom Exp $")

NCURSES_EXPORT(int)
win_wch(WINDOW *win, cchar_t *wcval)
{
    int row, col;
    int code = OK;

    TR(TRACE_CCALLS, (T_CALLED("win_wch(%p,%p)"), win, wcval));
    if (win != 0
	&& wcval != 0) {
	getyx(win, row, col);

	*wcval = win->_line[row].text[col];
	TR(TRACE_CCALLS, ("data %s", _tracecchar_t(wcval)));
    } else {
	code = ERR;
    }
    TR(TRACE_CCALLS, (T_RETURN("%d"), code));
    return (code);
}
