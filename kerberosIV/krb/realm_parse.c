/*	$OpenBSD: src/kerberosIV/krb/Attic/realm_parse.c,v 1.5 1998/07/07 19:07:00 art Exp $	*/
/*	$KTH: realm_parse.c,v 1.14 1997/12/15 17:19:13 assar Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska H�gskolan
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Kungliga Tekniska
 *      H�gskolan and its contributors.
 * 
 * 4. Neither the name of the Institute nor the names of its contributors
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

#include "krb_locl.h"

static int
realm_parse(char *realm, int length, const char *file)
{
    FILE *F;
    char tr[128];
    char *p;
    
    if ((F = fopen(file,"r")) == NULL)
	return -1;
    
    while(fgets(tr, sizeof(tr), F)){
	char *unused = NULL;
	p = strtok_r(tr, " \t\n\r", &unused);
	if(p && strcasecmp(p, realm) == 0){
	    fclose(F);
	    strncpy(realm, p, length);
	    realm[length - 1] = '\0';
	    return 0;
	}
    }
    fclose(F);
    return -1;
}

static const char *const files[] = KRB_CNF_FILES;

int
krb_realm_parse(char *realm, int length)
{
    int i;
    char file[MAXPATHLEN];

    for(i = 0; krb_get_krbconf(i, file, sizeof(file)) == 0; i++)
	if (realm_parse(realm, length, file) == 0)
	    return 0;

    return -1;
}
