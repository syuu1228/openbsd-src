/*	$OpenBSD: src/lib/libcurses/Attic/lib_erase.c,v 1.3 1997/12/03 05:21:17 millert Exp $	*/


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
**	lib_erase.c
**
**	The routine werase().
**
*/

#include <curses.priv.h>

MODULE_ID("Id: lib_erase.c,v 1.10 1997/09/20 15:02:34 juergen Exp $")

int  werase(WINDOW	*win)
{
int     code = ERR;
int	y;
chtype	blank;
chtype	*sp, *end, *start;

	T((T_CALLED("werase(%p)"), win));

	if (win) {
	  blank = _nc_background(win);
	  for (y = 0; y <= win->_maxy; y++) {
	    start = win->_line[y].text;
	    end = &start[win->_maxx];
	    
	    for (sp = start; sp <= end; sp++)
	      *sp = blank;
	    
	    win->_line[y].firstchar = 0;
	    win->_line[y].lastchar = win->_maxx;
	  }
	  win->_curx = win->_cury = 0;
	  win->_flags &= ~_WRAPPED;
	  _nc_synchook(win);
	  code = OK;
	}
	returnCode(code);
}
