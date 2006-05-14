/*	$OpenBSD: src/lib/libcurses/base/keyok.c,v 1.4 2006/05/14 09:01:06 espie Exp $	*/

/****************************************************************************
 * Copyright (c) 1998,2000 Free Software Foundation, Inc.                   *
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
 *  Author: Thomas E. Dickey <dickey@clark.net> 1997                        *
 ****************************************************************************/

#include <curses.priv.h>
#include <limits.h>

MODULE_ID("$From: keyok.c,v 1.5 2000/12/10 02:43:26 tom Exp $")

/*
 * Enable (or disable) ncurses' interpretation of a keycode by adding (or
 * removing) the corresponding 'tries' entry.
 *
 * Do this by storing a second tree of tries, which records the disabled keys. 
 * The simplest way to copy is to make a function that returns the string (with
 * nulls set to 0200), then use that to reinsert the string into the
 * corresponding tree.
 */

NCURSES_EXPORT(int)
keyok(int c, bool flag)
{
    int code = ERR;
    int count = 0;
    char *s;

    T((T_CALLED("keyok(%d,%d)"), c, flag));
    if (c >= 0 && c <= (int)USHRT_MAX) {
	    unsigned short k = (unsigned short) c;
	    if (flag) {
		while ((s = _nc_expand_try(SP->_key_ok, k, &count, 0)) != 0
		       && _nc_remove_key(&(SP->_key_ok), k)) {
		    _nc_add_to_try(&(SP->_keytry), s, k);
		    free(s);
		    code = OK;
		    count = 0;
		}
	    } else {
		while ((s = _nc_expand_try(SP->_keytry, k, &count, 0)) != 0
		       && _nc_remove_key(&(SP->_keytry), k)) {
		    _nc_add_to_try(&(SP->_key_ok), s, k);
		    free(s);
		    code = OK;
		    count = 0;
		}
	    }
    }
    returnCode(code);
}
