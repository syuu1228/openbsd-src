/*
 * Copyright (c) 1997 - 1999 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include <krb5_locl.h>

RCSID("$KTH: aname_to_localname.c,v 1.3 1999/12/02 17:05:07 joda Exp $");

krb5_error_code
krb5_aname_to_localname (krb5_context context,
			 krb5_const_principal aname,
			 size_t lnsize,
			 char *lname)
{
    krb5_error_code ret;
    krb5_realm *lrealms, *r;
    int foo = 1;
    size_t len;
    char *res;

    ret = krb5_get_default_realms (context, &lrealms);
    if (ret)
	return ret;

    for (r = lrealms; *r != NULL; ++r) {
	foo = strcmp (*r, aname->realm);
	if (foo == 0)
	    break;
    }
    krb5_free_host_realm (context, lrealms);
    if (foo != 0)
	return KRB5_NO_LOCALNAME;

    if (aname->name.name_string.len == 1)
	res = aname->name.name_string.val[0];
    else if (aname->name.name_string.len == 2
	     && strcmp (aname->name.name_string.val[1], "root") == 0)
	res = "root";
    else
	return KRB5_NO_LOCALNAME;

    len = strlen (res);
    if (len >= lnsize)
	return ERANGE;
    strlcpy (lname, res, lnsize);
    return 0;
}
