/*
 * Copyright (c) 1997 - 2004 Kungliga Tekniska H�gskolan
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

#include "kadmin_locl.h"
#include "kadmin-commands.h"

RCSID("$KTH: ext.c,v 1.11 2004/07/06 04:21:26 lha Exp $");

struct ext_keytab_data {
    krb5_keytab keytab;
};

static int
do_ext_keytab(krb5_principal principal, void *data)
{
    krb5_error_code ret;
    kadm5_principal_ent_rec princ;
    struct ext_keytab_data *e = data;
    krb5_keytab_entry *keys = NULL;
    krb5_keyblock *k = NULL;
    int i, n_k;
    
    ret = kadm5_get_principal(kadm_handle, principal, &princ, 
			      KADM5_PRINCIPAL|KADM5_KVNO|KADM5_KEY_DATA);
    if(ret)
	return ret;

    if (princ.n_key_data) {
	keys = malloc(sizeof(*keys) * princ.n_key_data);
	if (keys == NULL) {
	    kadm5_free_principal_ent(kadm_handle, &princ);
	    krb5_clear_error_string(context);
	    return ENOMEM;
	}
	for (i = 0; i < princ.n_key_data; i++) {
	    krb5_key_data *k = &princ.key_data[i];

	    keys[i].principal = princ.principal;
	    keys[i].vno = k->key_data_kvno;
	    keys[i].keyblock.keytype = k->key_data_type[0];
	    keys[i].keyblock.keyvalue.length = k->key_data_length[0];
	    keys[i].keyblock.keyvalue.data = k->key_data_contents[0];
	    keys[i].timestamp = time(NULL);
	}

	n_k = princ.n_key_data;
    } else {
	ret = kadm5_randkey_principal(kadm_handle, principal, &k, &n_k);
	if (ret) {
	    kadm5_free_principal_ent(kadm_handle, &princ);
	    return ret;
	}
	keys = malloc(sizeof(*keys) * n_k);
	if (keys == NULL) {
	    kadm5_free_principal_ent(kadm_handle, &princ);
	    krb5_clear_error_string(context);
	    return ENOMEM;
	}
	for (i = 0; i < n_k; i++) {
	    keys[i].principal = principal;
	    keys[i].vno = princ.kvno + 1; /* XXX get entry again */
	    keys[i].keyblock = k[i];
	    keys[i].timestamp = time(NULL);
	}
    }

    for(i = 0; i < n_k; i++) {
	ret = krb5_kt_add_entry(context, e->keytab, &keys[i]);
	if(ret)
	    krb5_warn(context, ret, "krb5_kt_add_entry(%d)", i);
    }

    if (k) {
	memset(k, 0, n_k * sizeof(*k));
	free(k);
    }
    if (keys)
	free(keys);
    kadm5_free_principal_ent(kadm_handle, &princ);
    return 0;
}

int
ext_keytab(struct ext_keytab_options *opt, int argc, char **argv)
{
    krb5_error_code ret;
    int i;
    struct ext_keytab_data data;

    if (opt->keytab_string == NULL)
	ret = krb5_kt_default(context, &data.keytab);
    else
	ret = krb5_kt_resolve(context, opt->keytab_string, &data.keytab);

    if(ret){
	krb5_warn(context, ret, "krb5_kt_resolve");
	return 0;
    }

    for(i = 0; i < argc; i++) 
	foreach_principal(argv[i], do_ext_keytab, "ext", &data);

    krb5_kt_close(context, data.keytab);

    return 0;
}
