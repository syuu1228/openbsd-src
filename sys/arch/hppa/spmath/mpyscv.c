/*	$OpenBSD: src/sys/arch/hppa/spmath/mpyscv.c,v 1.7 2003/04/10 17:27:58 mickey Exp $	*/
/*
  (c) Copyright 1986 HEWLETT-PACKARD COMPANY
  To anyone who acknowledges that this file is provided "AS IS"
  without any express or implied warranty:
      permission to use, copy, modify, and distribute this file
  for any purpose is hereby granted without fee, provided that
  the above copyright notice and this notice appears in all
  copies, and that the name of Hewlett-Packard Company not be
  used in advertising or publicity pertaining to distribution
  of the software without specific, written prior permission.
  Hewlett-Packard Company makes no representations about the
  suitability of this software for any purpose.
*/
/* @(#)mpyscv.c: Revision: 1.6.88.1 Date: 93/12/07 15:06:45 */

#include "md.h"

void
mpyscv(opnd1,opnd2,result)
	int opnd1, opnd2;
	struct mdsfu_register *result;
{
	s_xmpy(&opnd1,&opnd2,result);
	overflow = FALSE;
}
