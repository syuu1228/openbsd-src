/*
 * Copyright (c) 2005, PADL Software Pty Ltd.
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
 * 3. Neither the name of PADL Software nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PADL SOFTWARE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL PADL SOFTWARE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "kcm_locl.h"

RCSID("$KTH: cache.c,v 1.3 2005/02/06 01:22:48 lukeh Exp $");

static HEIMDAL_MUTEX ccache_mutex = HEIMDAL_MUTEX_INITIALIZER;
static kcm_ccache_data *ccache_head = NULL;
static unsigned int ccache_nextid = 0; 

char *kcm_ccache_nextid(pid_t pid, uid_t uid, gid_t gid)
{
    unsigned n;
    char *name;

    HEIMDAL_MUTEX_lock(&ccache_mutex);
    n = ++ccache_nextid;
    HEIMDAL_MUTEX_unlock(&ccache_mutex);

    asprintf(&name, "%d:%u", uid, n);

    return name;
}

static krb5_error_code
kcm_ccache_resolve_internal(krb5_context context,
			    const char *name,
			    kcm_ccache *ccache)
{
    kcm_ccache p;
    krb5_error_code ret;

    *ccache = NULL;

    ret = KRB5_FCC_NOFILE;

    HEIMDAL_MUTEX_lock(&ccache_mutex);

    for (p = ccache_head; p != NULL; p = p->next) {
	if ((p->flags & KCM_FLAGS_VALID) == 0)
	    continue;
	if (strcmp(p->name, name) == 0) {
	    ret = 0;
	    break;
	}
    }

    if (ret == 0) {
	kcm_retain_ccache(context, p);
	*ccache = p;
    }

    HEIMDAL_MUTEX_unlock(&ccache_mutex);

    return ret;
}

krb5_error_code kcm_debug_ccache(krb5_context context)
{
    kcm_ccache p;

    for (p = ccache_head; p != NULL; p = p->next) {
	char *cpn = NULL, *spn = NULL;
	int ncreds = 0;
	struct kcm_creds *k;

	if ((p->flags & KCM_FLAGS_VALID) == 0) {
	    kcm_log(7, "cache %08x: empty slot");
	    continue;
	}

	KCM_ASSERT_VALID(p);

	for (k = p->creds; k != NULL; k = k->next)
	    ncreds++;

	if (p->client != NULL)
	    krb5_unparse_name(context, p->client, &cpn);
	if (p->server != NULL)
	    krb5_unparse_name(context, p->server, &spn);
	
	kcm_log(7, "cache %08x: name %s refcnt %d flags %04x mode %04o "
		"uid %d gid %d client %s server %s ncreds %d",
		p, p->name, p->refcnt, p->flags, p->mode, p->uid, p->gid,
		(cpn == NULL) ? "<none>" : cpn,
		(spn == NULL) ? "<none>" : spn,
		ncreds);

	if (cpn != NULL)
	    free(cpn);
	if (spn != NULL)
	    free(spn);
    }

    return 0;
}

static krb5_error_code
kcm_ccache_destroy_internal(krb5_context context, const char *name)
{
    kcm_ccache *p;
    krb5_error_code ret;

    ret = KRB5_FCC_NOFILE;

    HEIMDAL_MUTEX_lock(&ccache_mutex);
    for (p = &ccache_head; *p != NULL; p = &(*p)->next) {
	if (((*p)->flags & KCM_FLAGS_VALID) == 0)
	    continue;
	if (strcmp((*p)->name, name) == 0) {
	    ret = 0;
	    break;
	}
    }

    if (ret)
	goto out;

    kcm_release_ccache(context, p);

out:
    HEIMDAL_MUTEX_unlock(&ccache_mutex);

    return ret;
}

static krb5_error_code
kcm_ccache_alloc(krb5_context context,
		 const char *name,
		 kcm_ccache *ccache)
{
    kcm_ccache slot = NULL, p;
    krb5_error_code ret;
    int new_slot = 0;

    *ccache = NULL;

    /* First, check for duplicates */
    HEIMDAL_MUTEX_lock(&ccache_mutex);
    ret = 0;
    for (p = ccache_head; p != NULL; p = p->next) {
	if (p->flags & KCM_FLAGS_VALID) {
	    if (strcmp(p->name, name) == 0) {
		ret = KRB5_CC_WRITE;
		break;
	    }
	} else if (slot == NULL)
	    slot = p;
    }

    if (ret)
	goto out;

    /*
     * Then try and find an empty slot
     * XXX we need to recycle slots for this to actually do anything
     */
    if (slot == NULL) {
	for (; p != NULL; p = p->next) {
	    if ((p->flags & KCM_FLAGS_VALID) == 0) {
		slot = p;
		break;
	    }
	}

	if (slot == NULL) {
	    slot = (kcm_ccache_data *)malloc(sizeof(*slot));
	    if (slot == NULL) {
		ret = KRB5_CC_NOMEM;
		goto out;
	    }
	    slot->next = ccache_head;
	    HEIMDAL_MUTEX_init(&slot->mutex);
	    new_slot = 1;
	}
    }

    slot->name = strdup(name);
    if (slot->name == NULL) {
	ret = KRB5_CC_NOMEM;
	goto out;
    }

    slot->refcnt = 1;
    slot->flags = KCM_FLAGS_VALID;
    slot->mode = S_IRUSR | S_IWUSR;
    slot->uid = -1;
    slot->gid = -1;
    slot->client = NULL;
    slot->server = NULL;
    slot->creds = NULL;
    slot->n_cursor = 0;
    slot->cursors = NULL;
    slot->key.keytab = NULL;
    slot->tkt_life = 0;
    slot->renew_life = 0;

    if (new_slot)
	ccache_head = slot;

    *ccache = slot;

    HEIMDAL_MUTEX_unlock(&ccache_mutex);
    return 0;

out:
    HEIMDAL_MUTEX_unlock(&ccache_mutex);
    if (new_slot && slot != NULL) {
	HEIMDAL_MUTEX_destroy(&slot->mutex);
	free(slot);
    }
    return ret;
}

krb5_error_code
kcm_ccache_remove_creds_internal(krb5_context context,
				 kcm_ccache ccache)
{
    struct kcm_creds *k;
    struct kcm_cursor *c;

    k = ccache->creds;
    while (k != NULL) {
	struct kcm_creds *old;

	krb5_free_cred_contents(context, &k->cred);
	old = k;
	k = k->next;
	free(old);
    }
    ccache->creds = NULL;

    /* remove anything that would have pointed into the creds too */

    ccache->n_cursor = 0;

    c = ccache->cursors;
    while (c != NULL) {
	struct kcm_cursor *old;

	old = c;
	c = c->next;
	free(old);
    }
    ccache->cursors = NULL;

    return 0;
}

krb5_error_code
kcm_ccache_remove_creds(krb5_context context,
			kcm_ccache ccache)
{
    krb5_error_code ret;

    KCM_ASSERT_VALID(ccache);

    HEIMDAL_MUTEX_lock(&ccache->mutex);
    ret = kcm_ccache_remove_creds_internal(context, ccache);
    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return ret;
}

krb5_error_code
kcm_zero_ccache_data_internal(krb5_context context,
			      kcm_ccache_data *cache)
{
    if (cache->client != NULL) {
	krb5_free_principal(context, cache->client);
	cache->client = NULL;
    }

    if (cache->server != NULL) {
	krb5_free_principal(context, cache->server);
	cache->server = NULL;
    }

    kcm_ccache_remove_creds_internal(context, cache);

    return 0;
}

krb5_error_code
kcm_zero_ccache_data(krb5_context context,
		     kcm_ccache cache)
{
    krb5_error_code ret;

    KCM_ASSERT_VALID(cache);

    HEIMDAL_MUTEX_lock(&cache->mutex);
    ret = kcm_zero_ccache_data_internal(context, cache);
    HEIMDAL_MUTEX_unlock(&cache->mutex);

    return ret;
}

static krb5_error_code
kcm_free_ccache_data_internal(krb5_context context,
			      kcm_ccache_data *cache)
{
    KCM_ASSERT_VALID(cache);

    if (cache->name != NULL) {
	free(cache->name);
	cache->name = NULL;
    }

    if (cache->flags & KCM_FLAGS_USE_KEYTAB) {
	krb5_kt_close(context, cache->key.keytab);
	cache->key.keytab = NULL;
    } else if (cache->flags & KCM_FLAGS_USE_CACHED_KEY) {
	krb5_free_keyblock_contents(context, &cache->key.keyblock);
	krb5_keyblock_zero(&cache->key.keyblock);
    }

    cache->flags = 0;
    cache->mode = 0;
    cache->uid = -1;
    cache->gid = -1;

    kcm_zero_ccache_data_internal(context, cache);

    cache->tkt_life = 0;
    cache->renew_life = 0;

    cache->next = NULL;
    cache->refcnt = 0;

    HEIMDAL_MUTEX_unlock(&cache->mutex);
    HEIMDAL_MUTEX_destroy(&cache->mutex);

    return 0;
}

krb5_error_code
kcm_retain_ccache(krb5_context context,
		  kcm_ccache ccache)
{
    KCM_ASSERT_VALID(ccache);

    HEIMDAL_MUTEX_lock(&ccache->mutex);
    ccache->refcnt++;
    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return 0;
}

krb5_error_code
kcm_release_ccache(krb5_context context,
		   kcm_ccache *ccache)
{
    kcm_ccache c = *ccache;
    krb5_error_code ret = 0;

    KCM_ASSERT_VALID(c);

    HEIMDAL_MUTEX_lock(&c->mutex);
    if (c->refcnt == 1) {
	ret = kcm_free_ccache_data_internal(context, c);
	if (ret == 0)
	    free(c);
    } else {
	c->refcnt--;
	HEIMDAL_MUTEX_unlock(&c->mutex);
    }

    *ccache = NULL;

    return ret;
}

krb5_error_code
kcm_ccache_gen_new(krb5_context context,
		   pid_t pid,
		   uid_t uid,
		   gid_t gid,
		   kcm_ccache *ccache)
{
    krb5_error_code ret;
    char *name;

    name = kcm_ccache_nextid(pid, uid, gid);
    if (name == NULL) {
	return KRB5_CC_NOMEM;
    }

    ret = kcm_ccache_new(context, name, ccache);

    free(name);
    return ret;
}

krb5_error_code
kcm_ccache_new(krb5_context context,
	       const char *name,
	       kcm_ccache *ccache)
{
    krb5_error_code ret;

    ret = kcm_ccache_alloc(context, name, ccache);
    if (ret == 0) {
	/*
	 * one reference is held by the linked list,
	 * one by the caller
	 */
	kcm_retain_ccache(context, *ccache);
    }

    return ret;
}

krb5_error_code
kcm_ccache_resolve(krb5_context context,
		   const char *name,
		   kcm_ccache *ccache)
{
    krb5_error_code ret;

    ret = kcm_ccache_resolve_internal(context, name, ccache);

    return ret;
}

krb5_error_code
kcm_ccache_destroy(krb5_context context,
		   const char *name)
{
    krb5_error_code ret;

    ret = kcm_ccache_destroy_internal(context, name);

    return ret;
}

krb5_error_code
kcm_ccache_destroy_if_empty(krb5_context context,
			    kcm_ccache ccache)
{
    krb5_error_code ret;

    KCM_ASSERT_VALID(ccache);
    
    if (ccache->creds == NULL) {
	ret = kcm_ccache_destroy_internal(context, ccache->name);
    } else
	ret = 0;

    return ret;
}

krb5_error_code
kcm_ccache_store_cred(krb5_context context,
		      kcm_ccache ccache,
		      krb5_creds *creds,
		      int copy)
{
    krb5_error_code ret;
    krb5_creds *tmp;

    KCM_ASSERT_VALID(ccache);
    
    HEIMDAL_MUTEX_lock(&ccache->mutex);
    ret = kcm_ccache_store_cred_internal(context, ccache, creds, copy, &tmp);
    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return ret;
}

krb5_error_code
kcm_ccache_store_cred_internal(krb5_context context,
			       kcm_ccache ccache,
			       krb5_creds *creds,
			       int copy,
			       krb5_creds **credp)
{
    struct kcm_creds **c;
    krb5_error_code ret;

    for (c = &ccache->creds; *c != NULL; c = &(*c)->next)
	;

    *c = (struct kcm_creds *)malloc(sizeof(struct kcm_creds));
    if (*c == NULL) {
	return KRB5_CC_NOMEM;
    }

    *credp = &(*c)->cred;

    if (copy) {
	ret = krb5_copy_creds_contents(context, creds, *credp);
	if (ret) {
	    free(*c);
	    *c = NULL;
	}
    } else {
	**credp = *creds;
	ret = 0;
    }

    (*c)->next = NULL;

    return ret;
}

static void
remove_cred(krb5_context context,
	    struct kcm_creds **c)
{
    struct kcm_creds *cred;

    cred = *c;

    *c = cred->next;

    krb5_free_cred_contents(context, &cred->cred);
    free(cred);
}

krb5_error_code
kcm_ccache_remove_cred_internal(krb5_context context,
				kcm_ccache ccache,
				krb5_flags whichfields,
				const krb5_creds *mcreds)
{
    krb5_error_code ret;
    struct kcm_creds **c;

    ret = KRB5_CC_NOTFOUND;

    for (c = &ccache->creds; *c != NULL; c = &(*c)->next) {
	if (krb5_compare_creds(context, whichfields, mcreds, &(*c)->cred)) {
	    remove_cred(context, c);
	    ret = 0;
	}
    }

    return ret;
}

krb5_error_code
kcm_ccache_remove_cred(krb5_context context,
		       kcm_ccache ccache,
		       krb5_flags whichfields,
		       const krb5_creds *mcreds)
{
    krb5_error_code ret;

    KCM_ASSERT_VALID(ccache);

    HEIMDAL_MUTEX_lock(&ccache->mutex);
    ret = kcm_ccache_remove_cred_internal(context, ccache, whichfields, mcreds);
    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return ret;
}

krb5_error_code
kcm_ccache_retrieve_cred_internal(krb5_context context,
			 	  kcm_ccache ccache,
			 	  krb5_flags whichfields,
			 	  const krb5_creds *mcreds,
			 	  krb5_creds **creds)
{
    krb5_boolean match;
    struct kcm_creds *c;
    krb5_error_code ret;

    memset(creds, 0, sizeof(*creds));

    ret = KRB5_CC_END;

    match = FALSE;
    for (c = ccache->creds; c != NULL; c = c->next) {
	match = krb5_compare_creds(context, whichfields, mcreds, &c->cred);
	if (match)
	    break;
    }

    if (match) {
	ret = 0;
	*creds = &c->cred;
    }

    return ret;
}

krb5_error_code
kcm_ccache_retrieve_cred(krb5_context context,
			 kcm_ccache ccache,
			 krb5_flags whichfields,
			 const krb5_creds *mcreds,
			 krb5_creds **credp)
{
    krb5_error_code ret;

    KCM_ASSERT_VALID(ccache);
    
    HEIMDAL_MUTEX_lock(&ccache->mutex);
    ret = kcm_ccache_retrieve_cred_internal(context, ccache,
					    whichfields, mcreds, credp);
    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return ret;
}
