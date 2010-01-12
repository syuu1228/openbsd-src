/* $OpenBSD: src/lib/libpanel/p_show.c,v 1.6 2010/01/12 23:22:08 nicm Exp $ */

/****************************************************************************
 * Copyright (c) 1998-2000,2005 Free Software Foundation, Inc.              *
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
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1995                    *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/

/* p_show.c
 * Place a panel on top of the stack; may already be in the stack 
 */
#include "panel.priv.h"

MODULE_ID("$Id: p_show.c,v 1.11 2005/02/19 16:42:02 tom Exp $")

NCURSES_EXPORT(int)
show_panel(PANEL * pan)
{
  int err = OK;

  T((T_CALLED("show_panel(%p)"), pan));

  if (!pan)
    returnCode(ERR);

  if (Is_Top(pan))
    returnCode(OK);

  dBug(("--> show_panel %s", USER_PTR(pan->user)));

  HIDE_PANEL(pan, err, OK);

  dStack("<lt%d>", 1, pan);
  assert(_nc_bottom_panel == _nc_stdscr_pseudo_panel);

  _nc_top_panel->above = pan;
  pan->below = _nc_top_panel;
  pan->above = (PANEL *) 0;
  _nc_top_panel = pan;

  dStack("<lt%d>", 9, pan);

  returnCode(OK);
}
