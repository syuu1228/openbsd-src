/*	$OpenBSD: src/lib/libform/fld_user.c,v 1.5 1999/05/17 03:04:15 millert Exp $	*/

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
 *   Author: Juergen Pfeifer <juergen.pfeifer@gmx.net> 1995,1997            *
 ****************************************************************************/

#include "form.priv.h"

MODULE_ID("$From: fld_user.c,v 1.8 1999/05/16 17:20:09 juergen Exp $")

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_field_userptr(FIELD *field, void *usrptr)
|   
|   Description   :  Set the pointer that is reserved in any field to store
|                    application relevant informations
|
|   Return Values :  E_OK         - on success
+--------------------------------------------------------------------------*/
int set_field_userptr(FIELD * field, void  *usrptr)
{
  Normalize_Field( field )->usrptr = usrptr;
  RETURN(E_OK);
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  void *field_userptr(const FIELD *field)
|   
|   Description   :  Return the pointer that is reserved in any field to
|                    store application relevant informations.
|
|   Return Values :  Value of pointer. If no such pointer has been set,
|                    NULL is returned
+--------------------------------------------------------------------------*/
void *field_userptr(const FIELD *field)
{
  return Normalize_Field( field )->usrptr;
}

/* fld_user.c ends here */
