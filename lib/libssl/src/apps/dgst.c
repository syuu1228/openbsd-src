/* apps/dgst.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "apps.h"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/engine.h>

#undef BUFSIZE
#define BUFSIZE	1024*8

#undef PROG
#define PROG	dgst_main

void do_fp(BIO *out, unsigned char *buf, BIO *bp, int sep, char binout,
		EVP_PKEY *key, unsigned char *sigin, int siglen);

int MAIN(int, char **);

int MAIN(int argc, char **argv)
	{
	ENGINE *e = NULL;
	unsigned char *buf=NULL;
	int i,err=0;
	const EVP_MD *md=NULL,*m;
	BIO *in=NULL,*inp;
	BIO *bmd=NULL;
	BIO *out = NULL;
	const char *name;
#define PROG_NAME_SIZE  16
	char pname[PROG_NAME_SIZE];
	int separator=0;
	int debug=0;
	const char *outfile = NULL, *keyfile = NULL;
	const char *sigfile = NULL, *randfile = NULL;
	char out_bin = -1, want_pub = 0, do_verify = 0;
	EVP_PKEY *sigkey = NULL;
	unsigned char *sigbuf = NULL;
	int siglen = 0;
	char *engine=NULL;

	apps_startup();

	if ((buf=(unsigned char *)OPENSSL_malloc(BUFSIZE)) == NULL)
		{
		BIO_printf(bio_err,"out of memory\n");
		goto end;
		}
	if (bio_err == NULL)
		if ((bio_err=BIO_new(BIO_s_file())) != NULL)
			BIO_set_fp(bio_err,stderr,BIO_NOCLOSE|BIO_FP_TEXT);

	/* first check the program name */
	program_name(argv[0],pname,PROG_NAME_SIZE);

	md=EVP_get_digestbyname(pname);

	argc--;
	argv++;
	while (argc > 0)
		{
		if ((*argv)[0] != '-') break;
		if (strcmp(*argv,"-c") == 0)
			separator=1;
		else if (strcmp(*argv,"-rand") == 0)
			{
			if (--argc < 1) break;
			randfile=*(++argv);
			}
		else if (strcmp(*argv,"-out") == 0)
			{
			if (--argc < 1) break;
			outfile=*(++argv);
			}
		else if (strcmp(*argv,"-sign") == 0)
			{
			if (--argc < 1) break;
			keyfile=*(++argv);
			}
		else if (strcmp(*argv,"-verify") == 0)
			{
			if (--argc < 1) break;
			keyfile=*(++argv);
			want_pub = 1;
			do_verify = 1;
			}
		else if (strcmp(*argv,"-prverify") == 0)
			{
			if (--argc < 1) break;
			keyfile=*(++argv);
			do_verify = 1;
			}
		else if (strcmp(*argv,"-signature") == 0)
			{
			if (--argc < 1) break;
			sigfile=*(++argv);
			}
		else if (strcmp(*argv,"-engine") == 0)
			{
			if (--argc < 1) break;
			engine= *(++argv);
			}
		else if (strcmp(*argv,"-hex") == 0)
			out_bin = 0;
		else if (strcmp(*argv,"-binary") == 0)
			out_bin = 1;
		else if (strcmp(*argv,"-d") == 0)
			debug=1;
		else if ((m=EVP_get_digestbyname(&((*argv)[1]))) != NULL)
			md=m;
		else
			break;
		argc--;
		argv++;
		}

	if (md == NULL)
		md=EVP_md5();

	if(do_verify && !sigfile) {
		BIO_printf(bio_err, "No signature to verify: use the -signature option\n");
		err = 1; 
		goto end;
	}

	if ((argc > 0) && (argv[0][0] == '-')) /* bad option */
		{
		BIO_printf(bio_err,"unknown option '%s'\n",*argv);
		BIO_printf(bio_err,"options are\n");
		BIO_printf(bio_err,"-c              to output the digest with separating colons\n");
		BIO_printf(bio_err,"-d              to output debug info\n");
		BIO_printf(bio_err,"-hex            output as hex dump\n");
		BIO_printf(bio_err,"-binary         output in binary form\n");
		BIO_printf(bio_err,"-sign   file    sign digest using private key in file\n");
		BIO_printf(bio_err,"-verify file    verify a signature using public key in file\n");
		BIO_printf(bio_err,"-prverify file  verify a signature using private key in file\n");
		BIO_printf(bio_err,"-signature file signature to verify\n");
		BIO_printf(bio_err,"-binary         output in binary form\n");
		BIO_printf(bio_err,"-engine e       use engine e, possibly a hardware device.\n");

		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm (default)\n",
			LN_md5,LN_md5);
		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm\n",
			LN_md4,LN_md4);
		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm\n",
			LN_md2,LN_md2);
		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm\n",
			LN_sha1,LN_sha1);
		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm\n",
			LN_sha,LN_sha);
		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm\n",
			LN_mdc2,LN_mdc2);
		BIO_printf(bio_err,"-%3s to use the %s message digest algorithm\n",
			LN_ripemd160,LN_ripemd160);
		err=1;
		goto end;
		}

	if (engine != NULL)
		{
		if((e = ENGINE_by_id(engine)) == NULL)
			{
			BIO_printf(bio_err,"invalid engine \"%s\"\n",
				engine);
			goto end;
			}
		if(!ENGINE_set_default(e, ENGINE_METHOD_ALL))
			{
			BIO_printf(bio_err,"can't use that engine\n");
			goto end;
			}
		BIO_printf(bio_err,"engine \"%s\" set.\n", engine);
		/* Free our "structural" reference. */
		ENGINE_free(e);
		}

	in=BIO_new(BIO_s_file());
	bmd=BIO_new(BIO_f_md());
	if (debug)
		{
		BIO_set_callback(in,BIO_debug_callback);
		/* needed for windows 3.1 */
		BIO_set_callback_arg(in,bio_err);
		}

	if ((in == NULL) || (bmd == NULL))
		{
		ERR_print_errors(bio_err);
		goto end;
		}

	if(out_bin == -1) {
		if(keyfile) out_bin = 1;
		else out_bin = 0;
	}

	if(randfile)
		app_RAND_load_file(randfile, bio_err, 0);

	if(outfile) {
		if(out_bin)
			out = BIO_new_file(outfile, "wb");
		else    out = BIO_new_file(outfile, "w");
	} else {
		out = BIO_new_fp(stdout, BIO_NOCLOSE);
#ifdef VMS
		{
		BIO *tmpbio = BIO_new(BIO_f_linebuffer());
		out = BIO_push(tmpbio, out);
		}
#endif
	}

	if(!out) {
		BIO_printf(bio_err, "Error opening output file %s\n", 
					outfile ? outfile : "(stdout)");
		ERR_print_errors(bio_err);
		goto end;
	}

	if(keyfile) {
		BIO *keybio;
		keybio = BIO_new_file(keyfile, "r");
		if(!keybio) {
			BIO_printf(bio_err, "Error opening key file %s\n",
								keyfile);
			ERR_print_errors(bio_err);
			goto end;
		}
		
		if(want_pub) 
			sigkey = PEM_read_bio_PUBKEY(keybio, NULL, NULL, NULL);
		else sigkey = PEM_read_bio_PrivateKey(keybio, NULL, NULL, NULL);
		BIO_free(keybio);
		if(!sigkey) {
			BIO_printf(bio_err, "Error reading key file %s\n",
								keyfile);
			ERR_print_errors(bio_err);
			goto end;
		}
	}

	if(sigfile && sigkey) {
		BIO *sigbio;
		sigbio = BIO_new_file(sigfile, "rb");
		siglen = EVP_PKEY_size(sigkey);
		sigbuf = OPENSSL_malloc(siglen);
		if(!sigbio) {
			BIO_printf(bio_err, "Error opening signature file %s\n",
								sigfile);
			ERR_print_errors(bio_err);
			goto end;
		}
		siglen = BIO_read(sigbio, sigbuf, siglen);
		BIO_free(sigbio);
		if(siglen <= 0) {
			BIO_printf(bio_err, "Error reading signature file %s\n",
								sigfile);
			ERR_print_errors(bio_err);
			goto end;
		}
	}
		


	/* we use md as a filter, reading from 'in' */
	BIO_set_md(bmd,md);
	inp=BIO_push(bmd,in);

	if (argc == 0)
		{
		BIO_set_fp(in,stdin,BIO_NOCLOSE);
		do_fp(out, buf,inp,separator, out_bin, sigkey, sigbuf, siglen);
		}
	else
		{
		name=OBJ_nid2sn(md->type);
		for (i=0; i<argc; i++)
			{
			if (BIO_read_filename(in,argv[i]) <= 0)
				{
				perror(argv[i]);
				err++;
				continue;
				}
			if(!out_bin) BIO_printf(out, "%s(%s)= ",name,argv[i]);
			do_fp(out, buf,inp,separator, out_bin, sigkey, 
								sigbuf, siglen);
			(void)BIO_reset(bmd);
			}
		}
end:
	if (buf != NULL)
		{
		memset(buf,0,BUFSIZE);
		OPENSSL_free(buf);
		}
	if (in != NULL) BIO_free(in);
	BIO_free_all(out);
	EVP_PKEY_free(sigkey);
	if(sigbuf) OPENSSL_free(sigbuf);
	if (bmd != NULL) BIO_free(bmd);
	EXIT(err);
	}

void do_fp(BIO *out, unsigned char *buf, BIO *bp, int sep, char binout,
			EVP_PKEY *key, unsigned char *sigin, int siglen)
	{
	int len;
	int i;

	for (;;)
		{
		i=BIO_read(bp,(char *)buf,BUFSIZE);
		if (i <= 0) break;
		}
	if(sigin)
		{
		EVP_MD_CTX *ctx;
		BIO_get_md_ctx(bp, &ctx);
		i = EVP_VerifyFinal(ctx, sigin, (unsigned int)siglen, key); 
		if(i > 0) BIO_printf(out, "Verified OK\n");
		else if(i == 0) BIO_printf(out, "Verification Failure\n");
		else
			{
			BIO_printf(bio_err, "Error Verifying Data\n");
			ERR_print_errors(bio_err);
			}
		return;
		}
	if(key)
		{
		EVP_MD_CTX *ctx;
		BIO_get_md_ctx(bp, &ctx);
		if(!EVP_SignFinal(ctx, buf, (unsigned int *)&len, key)) 
			{
			BIO_printf(bio_err, "Error Signing Data\n");
			ERR_print_errors(bio_err);
			return;
			}
		}
	else
		len=BIO_gets(bp,(char *)buf,BUFSIZE);

	if(binout) BIO_write(out, buf, len);
	else 
		{
		for (i=0; i<len; i++)
			{
			if (sep && (i != 0))
				BIO_printf(out, ":");
			BIO_printf(out, "%02x",buf[i]);
			}
		BIO_printf(out, "\n");
		}
	}

