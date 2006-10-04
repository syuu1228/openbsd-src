/* crypto/dsa/dsa.h */
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

/*
 * The DSS routines are based on patches supplied by
 * Steven Schoch <schoch@sheba.arc.nasa.gov>.  He basically did the
 * work and I have just tweaked them a little to fit into my
 * stylistic vision for SSLeay :-) */

#ifndef HEADER_DSA_H
#define HEADER_DSA_H

#ifdef OPENSSL_NO_DSA
#error DSA is disabled.
#endif

#ifndef OPENSSL_NO_BIO
#include <openssl/bio.h>
#endif
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/ossl_typ.h>
#ifndef OPENSSL_NO_DH
# include <openssl/dh.h>
#endif

#define OPENSSL_DSA_MAX_MODULUS_BITS	3072

#define DSA_FLAG_CACHE_MONT_P	0x01
#define DSA_FLAG_NO_EXP_CONSTTIME       0x02 /* new with 0.9.7h; the built-in DSA
                                              * implementation now uses constant time
                                              * modular exponentiation for secret exponents
                                              * by default. This flag causes the
                                              * faster variable sliding window method to
                                              * be used for all exponents.
                                              */

/* If this flag is set external DSA_METHOD callbacks are allowed in FIPS mode
 * it is then the applications responsibility to ensure the external method
 * is compliant.
 */

#define DSA_FLAG_FIPS_EXTERNAL_METHOD_ALLOW	0x04

#if defined(OPENSSL_FIPS)
#define FIPS_DSA_SIZE_T	int
#endif

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct dsa_st DSA;

typedef struct DSA_SIG_st
	{
	BIGNUM *r;
	BIGNUM *s;
	} DSA_SIG;

typedef struct dsa_method {
	const char *name;
	DSA_SIG * (*dsa_do_sign)(const unsigned char *dgst, int dlen, DSA *dsa);
	int (*dsa_sign_setup)(DSA *dsa, BN_CTX *ctx_in, BIGNUM **kinvp,
								BIGNUM **rp);
	int (*dsa_do_verify)(const unsigned char *dgst, int dgst_len,
							DSA_SIG *sig, DSA *dsa);
	int (*dsa_mod_exp)(DSA *dsa, BIGNUM *rr, BIGNUM *a1, BIGNUM *p1,
			BIGNUM *a2, BIGNUM *p2, BIGNUM *m, BN_CTX *ctx,
			BN_MONT_CTX *in_mont);
	int (*bn_mod_exp)(DSA *dsa, BIGNUM *r, BIGNUM *a, const BIGNUM *p,
				const BIGNUM *m, BN_CTX *ctx,
				BN_MONT_CTX *m_ctx); /* Can be null */
	int (*init)(DSA *dsa);
	int (*finish)(DSA *dsa);
	int flags;
	char *app_data;
} DSA_METHOD;

struct dsa_st
	{
	/* This first variable is used to pick up errors where
	 * a DSA is passed instead of of a EVP_PKEY */
	int pad;
	long version;
	int write_params;
	BIGNUM *p;
	BIGNUM *q;	/* == 20 */
	BIGNUM *g;

	BIGNUM *pub_key;  /* y public key */
	BIGNUM *priv_key; /* x private key */

	BIGNUM *kinv;	/* Signing pre-calc */
	BIGNUM *r;	/* Signing pre-calc */

	int flags;
	/* Normally used to cache montgomery values */
	char *method_mont_p;
	int references;
	CRYPTO_EX_DATA ex_data;
	const DSA_METHOD *meth;
	/* functional reference if 'meth' is ENGINE-provided */
	ENGINE *engine;
	};

#define DSAparams_dup(x) (DSA *)ASN1_dup((int (*)())i2d_DSAparams, \
		(char *(*)())d2i_DSAparams,(char *)(x))
#define d2i_DSAparams_fp(fp,x) (DSA *)ASN1_d2i_fp((char *(*)())DSA_new, \
		(char *(*)())d2i_DSAparams,(fp),(unsigned char **)(x))
#define i2d_DSAparams_fp(fp,x) ASN1_i2d_fp(i2d_DSAparams,(fp), \
		(unsigned char *)(x))
#define d2i_DSAparams_bio(bp,x) (DSA *)ASN1_d2i_bio((char *(*)())DSA_new, \
		(char *(*)())d2i_DSAparams,(bp),(unsigned char **)(x))
#define i2d_DSAparams_bio(bp,x) ASN1_i2d_bio(i2d_DSAparams,(bp), \
		(unsigned char *)(x))


DSA_SIG * DSA_SIG_new(void);
void	DSA_SIG_free(DSA_SIG *a);
int	i2d_DSA_SIG(const DSA_SIG *a, unsigned char **pp);
DSA_SIG * d2i_DSA_SIG(DSA_SIG **v, const unsigned char **pp, long length);

DSA_SIG * DSA_do_sign(const unsigned char *dgst,int dlen,DSA *dsa);
int	DSA_do_verify(const unsigned char *dgst,int dgst_len,
		      DSA_SIG *sig,DSA *dsa);

const DSA_METHOD *DSA_OpenSSL(void);

void	DSA_set_default_method(const DSA_METHOD *);
const DSA_METHOD *DSA_get_default_method(void);
int	DSA_set_method(DSA *dsa, const DSA_METHOD *);

DSA *	DSA_new(void);
DSA *	DSA_new_method(ENGINE *engine);
void	DSA_free (DSA *r);
/* "up" the DSA object's reference count */
int	DSA_up_ref(DSA *r);
int	DSA_size(const DSA *);
	/* next 4 return -1 on error */
int	DSA_sign_setup( DSA *dsa,BN_CTX *ctx_in,BIGNUM **kinvp,BIGNUM **rp);
int	DSA_sign(int type,const unsigned char *dgst,int dlen,
		unsigned char *sig, unsigned int *siglen, DSA *dsa);
int	DSA_verify(int type,const unsigned char *dgst,int dgst_len,
		const unsigned char *sigbuf, int siglen, DSA *dsa);
int DSA_get_ex_new_index(long argl, void *argp, CRYPTO_EX_new *new_func,
	     CRYPTO_EX_dup *dup_func, CRYPTO_EX_free *free_func);
int DSA_set_ex_data(DSA *d, int idx, void *arg);
void *DSA_get_ex_data(DSA *d, int idx);

DSA *	d2i_DSAPublicKey(DSA **a, const unsigned char **pp, long length);
DSA *	d2i_DSAPrivateKey(DSA **a, const unsigned char **pp, long length);
DSA * 	d2i_DSAparams(DSA **a, const unsigned char **pp, long length);
DSA *	DSA_generate_parameters(int bits,
		unsigned char *seed,int seed_len,
		int *counter_ret, unsigned long *h_ret,void
		(*callback)(int, int, void *),void *cb_arg);
int	DSA_generate_key(DSA *a);
int	i2d_DSAPublicKey(const DSA *a, unsigned char **pp);
int 	i2d_DSAPrivateKey(const DSA *a, unsigned char **pp);
int	i2d_DSAparams(const DSA *a,unsigned char **pp);

#ifndef OPENSSL_NO_BIO
int	DSAparams_print(BIO *bp, const DSA *x);
int	DSA_print(BIO *bp, const DSA *x, int off);
#endif
#ifndef OPENSSL_NO_FP_API
int	DSAparams_print_fp(FILE *fp, const DSA *x);
int	DSA_print_fp(FILE *bp, const DSA *x, int off);
#endif

#define DSS_prime_checks 50
/* Primality test according to FIPS PUB 186[-1], Appendix 2.1:
 * 50 rounds of Rabin-Miller */
#define DSA_is_prime(n, callback, cb_arg) \
	BN_is_prime(n, DSS_prime_checks, callback, NULL, cb_arg)

#ifndef OPENSSL_NO_DH
/* Convert DSA structure (key or just parameters) into DH structure
 * (be careful to avoid small subgroup attacks when using this!) */
DH *DSA_dup_DH(const DSA *r);
#endif

/* BEGIN ERROR CODES */
/* The following lines are auto generated by the script mkerr.pl. Any changes
 * made after this point may be overwritten when the script is next run.
 */
void ERR_load_DSA_strings(void);

/* Error codes for the DSA functions. */

/* Function codes. */
#define DSA_F_D2I_DSA_SIG				 110
#define DSA_F_DSAPARAMS_PRINT				 100
#define DSA_F_DSAPARAMS_PRINT_FP			 101
#define DSA_F_DSA_DO_SIGN				 112
#define DSA_F_DSA_DO_VERIFY				 113
#define DSA_F_DSA_NEW_METHOD				 103
#define DSA_F_DSA_PRINT					 104
#define DSA_F_DSA_PRINT_FP				 105
#define DSA_F_DSA_SIGN					 106
#define DSA_F_DSA_SIGN_SETUP				 107
#define DSA_F_DSA_SIG_NEW				 109
#define DSA_F_DSA_VERIFY				 108
#define DSA_F_I2D_DSA_SIG				 111
#define DSA_F_SIG_CB					 114

/* Reason codes. */
#define DSA_R_BAD_Q_VALUE				 102
#define DSA_R_DATA_TOO_LARGE_FOR_KEY_SIZE		 100
#define DSA_R_MISSING_PARAMETERS			 101
#define DSA_R_MODULUS_TOO_LARGE				 103

#ifdef  __cplusplus
}
#endif
#endif
