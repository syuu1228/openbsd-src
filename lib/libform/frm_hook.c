/*	$OpenBSD: src/lib/libform/frm_hook.c,v 1.3 1997/12/03 05:40:13 millert Exp $	*/

/*-----------------------------------------------------------------------------+
|           The ncurses form library is  Copyright (C) 1995-1997               |
|             by Juergen Pfeifer <Juergen.Pfeifer@T-Online.de>                 |
|                          All Rights Reserved.                                |
|                                                                              |
| Permission to use, copy, modify, and distribute this software and its        |
| documentation for any purpose and without fee is hereby granted, provided    |
| that the above copyright notice appear in all copies and that both that      |
| copyright notice and this permission notice appear in supporting             |
| documentation, and that the name of the above listed copyright holder(s) not |
| be used in advertising or publicity pertaining to distribution of the        |
| software without specific, written prior permission.                         | 
|                                                                              |
| THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD TO  |
| THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-  |
| NESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR   |
| ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RE- |
| SULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, |
| NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH    |
| THE USE OR PERFORMANCE OF THIS SOFTWARE.                                     |
+-----------------------------------------------------------------------------*/

#include "form.priv.h"

MODULE_ID("Id: frm_hook.c,v 1.5 1997/05/01 16:47:54 juergen Exp $")

/* "Template" macro to generate function to set application specific hook */
#define GEN_HOOK_SET_FUNCTION( typ, name ) \
int set_ ## typ ## _ ## name (FORM *form, Form_Hook func)\
{\
   (Normalize_Form( form ) -> typ ## name) = func ;\
   RETURN(E_OK);\
}

/* "Template" macro to generate function to get application specific hook */
#define GEN_HOOK_GET_FUNCTION( typ, name ) \
Form_Hook typ ## _ ## name ( const FORM *form )\
{\
   return ( Normalize_Form( form ) -> typ ## name );\
}

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_field_init(FORM *form, Form_Hook f)
|   
|   Description   :  Assigns an application defined initialization function
|                    to be called when the form is posted and just after
|                    the current field changes.
|
|   Return Values :  E_OK      - success
+--------------------------------------------------------------------------*/
GEN_HOOK_SET_FUNCTION(field,init)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  Form_Hook field_init(const FORM *form)
|   
|   Description   :  Retrieve field initialization routine address.
|
|   Return Values :  The address or NULL if no hook defined.
+--------------------------------------------------------------------------*/
GEN_HOOK_GET_FUNCTION(field,init)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_field_term(FORM *form, Form_Hook f)
|   
|   Description   :  Assigns an application defined finalization function
|                    to be called when the form is unposted and just before
|                    the current field changes.
|
|   Return Values :  E_OK      - success
+--------------------------------------------------------------------------*/
GEN_HOOK_SET_FUNCTION(field,term)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  Form_Hook field_term(const FORM *form)
|   
|   Description   :  Retrieve field finalization routine address.
|
|   Return Values :  The address or NULL if no hook defined.
+--------------------------------------------------------------------------*/
GEN_HOOK_GET_FUNCTION(field,term)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_form_init(FORM *form, Form_Hook f)
|   
|   Description   :  Assigns an application defined initialization function
|                    to be called when the form is posted and just after
|                    a page change.
|
|   Return Values :  E_OK       - success
+--------------------------------------------------------------------------*/
GEN_HOOK_SET_FUNCTION(form,init)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  Form_Hook form_init(const FORM *form)
|   
|   Description   :  Retrieve form initialization routine address.
|
|   Return Values :  The address or NULL if no hook defined.
+--------------------------------------------------------------------------*/
GEN_HOOK_GET_FUNCTION(form,init)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  int set_form_term(FORM *form, Form_Hook f)
|   
|   Description   :  Assigns an application defined finalization function
|                    to be called when the form is unposted and just before
|                    a page change.
|
|   Return Values :  E_OK       - success
+--------------------------------------------------------------------------*/
GEN_HOOK_SET_FUNCTION(form,term)

/*---------------------------------------------------------------------------
|   Facility      :  libnform  
|   Function      :  Form_Hook form_term(const FORM *form)
|   
|   Description   :  Retrieve form finalization routine address.
|
|   Return Values :  The address or NULL if no hook defined.
+--------------------------------------------------------------------------*/
GEN_HOOK_GET_FUNCTION(form,term)

/* frm_hook.c ends here */
