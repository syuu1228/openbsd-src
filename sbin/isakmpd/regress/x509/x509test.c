/*	$OpenBSD: src/sbin/isakmpd/regress/x509/Attic/x509test.c,v 1.11 2000/12/19 18:54:59 mickey Exp $	*/
/*	$EOM: x509test.c,v 1.8 1999/09/30 13:40:39 niklas Exp $	*/

/*
 * Copyright (c) 1998, 1999 Niels Provos.  All rights reserved.
 * Copyright (c) 1999 Niklas Hallqvist.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ericsson Radio Systems.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code was written under funding by Ericsson Radio Systems.
 */

/*
 * This program takes a certificate generated by ssleay and a key pair
 * from rsakeygen. It reads the IP address from certificate.txt and
 * includes this as subject alt name extension into the certifcate.
 * The result gets written as new certificate that can be used by
 * isakmpd.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "conf.h"
#include "libcrypto.h"
#include "log.h"
#include "ipsec_num.h"
#include "x509.h"

u_int32_t file_sz;

u_int8_t *
open_file (char *name)
{
  int fd;
  struct stat st;
  u_int8_t *addr;

  if (stat (name, &st) == -1)
    log_fatal ("stat (\"%s\", &st)", name);
  file_sz = st.st_size;
  fd = open (name, O_RDONLY);
  if (fd == -1)
    log_fatal ("open (\"%s\", O_RDONLY)", name);
  addr = mmap (0, file_sz, PROT_READ | PROT_WRITE, MAP_FILE | MAP_PRIVATE,
	       fd, 0);
  if (!addr)
    log_fatal ("mmap (0, %d, PROT_READ | PROT_WRITE, MAP_FILE | MAP_PRIVATE,"
	       "%d, 0)", file_sz, fd);
  close (fd);

  return addr;
}

int
main (int argc, char *argv[])
{
  RSA *pub_key, *priv_key;
  X509 *cert;
  BIO *certfile, *keyfile;
  EVP_PKEY *pkey_pub;
  u_char ipaddr[6];
  struct in_addr saddr;
  char enc[256], dec[256];
  u_int8_t idpayload[8];
  int err, len;

  if (argc < 3 || argc > 4)
    {
      fprintf (stderr, "usage: x509test private-key certificate ip-address\n");
      exit (1);
    }

  /*
   * X509_verify will fail, as will all other functions that call
   * EVP_get_digest_byname.
   */

  libcrypto_init ();

#ifndef USE_LIBCRYPTO
  if (!libcrypto)
    {
      fprintf (stderr, "I did not find the X.509 support, giving up...");
      exit (1);
    }
#endif

  printf ("Reading private key %s\n", argv[1]);
  keyfile = LC (BIO_new, (LC (BIO_s_file, ())));
  if (LC (BIO_read_filename, (keyfile, argv[1])) == -1) 
    {
      perror ("read");
      exit (1);
    }
#if SSLEAY_VERSION_NUMBER >= 0x00904100L
  priv_key = LC (PEM_read_bio_RSAPrivateKey, (keyfile, NULL, NULL, NULL));
#else
  priv_key = LC (PEM_read_bio_RSAPrivateKey, (keyfile, NULL, NULL));
#endif
  LC (BIO_free, (keyfile));
  if (priv_key == NULL)
    {
      printf("PEM_read_bio_RSAPrivateKey () failed\n");
      exit (1);
    }

  /* Use a certificate created by ssleay.  */
  printf ("Reading ssleay created certificate %s\n", argv[2]);
  certfile = LC (BIO_new, (LC (BIO_s_file, ())));
  if (LC (BIO_read_filename, (certfile, argv[2])) == -1) 
    {
      perror ("read");
      exit (1);
    }
#if SSLEAY_VERSION_NUMBER >= 0x00904100L
  cert = LC (PEM_read_bio_X509, (certfile, NULL, NULL, NULL));
#else
  cert = LC (PEM_read_bio_X509, (certfile, NULL, NULL));
#endif
  LC (BIO_free, (certfile));
  if (cert == NULL) 
    {
      printf("PEM_read_bio_X509 () failed\n");
      exit (1);
    }

  pkey_pub = LC (X509_get_pubkey, (cert));
  /* XXX Violation of the interface?  */
  pub_key = pkey_pub->pkey.rsa;
  if (pub_key == NULL)
    {
      exit (1);
    }

  printf ("Testing RSA keys: ");

  err = 0;
  strcpy (dec, "Eine kleine Testmeldung");
  if ((len = LC (RSA_private_encrypt, (strlen (dec), dec, enc, priv_key,
				       RSA_PKCS1_PADDING))) == -1)
  
    printf ("SIGN FAILED ");
  else
    err = LC (RSA_public_decrypt, (len, enc, dec, pub_key, RSA_PKCS1_PADDING));

  if (err == -1 || strcmp (dec, "Eine kleine Testmeldung"))
    printf ("SIGN/VERIFY FAILED");
  else
    printf ("OKAY");
  printf ("\n");
  

  printf ("Validate SIGNED: ");
  err = LC (X509_verify, (cert, pkey_pub));
  printf ("X509 verify: %d ", err);
  if (err == -1)
    printf ("FAILED ");
  else
    printf ("OKAY ");
  printf ("\n");

  if (argc == 4)
    {
      printf ("Verifying extension: ");
      if (inet_aton (argv[3], &saddr) == -1)
	{
	  printf ("inet_aton () failed\n");
	  exit (1);
	}

      saddr.s_addr = htonl (saddr.s_addr);
      ipaddr[0] = 0x87;
      ipaddr[1] = 0x04;
      ipaddr[2] = saddr.s_addr >> 24;
      ipaddr[3] = (saddr.s_addr >> 16) & 0xff;
      ipaddr[4] = (saddr.s_addr >> 8) & 0xff;
      ipaddr[5] = saddr.s_addr & 0xff;
      bzero (idpayload, sizeof idpayload);
      idpayload[0] = IPSEC_ID_IPV4_ADDR;
      bcopy (ipaddr + 2, idpayload + 4, 4);

      if (!x509_check_subjectaltname (idpayload, sizeof idpayload, cert))
	printf("FAILED ");
      else
	printf("OKAY ");
      printf ("\n");
    }

  return 1;
}
