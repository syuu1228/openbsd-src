/*
 * Copyright (c) 1995, 1996, 1997, 1998 Kungliga Tekniska H�gskolan
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

/*
 * hash.h. Header file for hash table functions
 */

/* $KTH: hash.h,v 1.6.2.1 2001/08/31 18:09:17 ahltorp Exp $ */

#include <bool.h>

struct hashentry {		/* Entry in bucket */
     struct hashentry **prev;
     struct hashentry *next;
     void *ptr;
};

typedef struct hashentry Hashentry;

struct hashtab {		/* Hash table */
     int (*cmp)(void *, void *); /* Compare function */
     unsigned (*hash)(void *);	/* hash function */
     int sz;			/* Size */
     Hashentry *tab[1];		/* The table */
};

typedef struct hashtab Hashtab;
typedef int (*hashtabnew_arg2_t)(void *, void *);
typedef unsigned (*hashtabnew_arg3_t)(void *);

/* prototypes */

Hashtab *hashtabnew(int sz, 
		    int (*cmp)(void *, void *),
		    unsigned (*hash)(void *));	/* Make new hash table */

void *hashtabsearch(Hashtab *htab, /* The hash table */
		    void *ptr);	/*  The key */


void *hashtabaddreplace(Hashtab *htab,	/* The hash table */
			void *ptr);	/* The element */

void *hashtabadd(Hashtab *htab,
		 void *ptr);

int _hashtabdel(Hashtab *htab,	/* The table */
		void *ptr,	/* Key */
		int freep);	/* Free data part? */

void hashtabforeach(Hashtab *htab,
		    Bool (*func)(void *ptr, void *arg),
		    void *arg);

void hashtabcleantab(Hashtab * htab, 
		     Bool(*cond) (void *ptr, void *arg),
		     void *arg);

void hashtabrelease(Hashtab *htab);

unsigned hashadd(const char *s);		/* Standard hash function */
unsigned hashcaseadd(const char *s);		/* Standard hash function */
unsigned hashjpw(const char *s);		/* another hash function */

/* macros */

 /* Don't free space */
#define hashtabdel(htab,key)  _hashtabdel(htab,key,FALSE)

#define hashtabfree(htab,key) _hashtabdel(htab,key,TRUE) /* Do! */
