/* $OpenBSD: src/lib/libkeynote/base64.c,v 1.6 1999/10/26 22:31:37 angelos Exp $ */
/*
 * Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Portions Copyright (c) 1995 by International Business Machines, Inc.
 *
 * International Business Machines, Inc. (hereinafter called IBM) grants
 * permission under its copyrights to use, copy, modify, and distribute this
 * Software with or without fee, provided that the above copyright notice and
 * all paragraphs of this notice appear in all copies, and that the name of IBM
 * not be used in connection with the marketing of any product incorporating
 * the Software or modifications thereof, without specific, written prior
 * permission.
 *
 * To the extent it has a right to do so, IBM grants an immunity from suit
 * under its patents, if any, for the use, sale or manufacture of products to
 * the extent that such products are used for performing Domain Name System
 * dynamic updates in TCP/IP networks by means of the Software.  No immunity is
 * granted for any product per se or for any other function of any product.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", AND IBM DISCLAIMS ALL WARRANTIES,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, EVEN
 * IF IBM IS APPRISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if STDC_HEADERS
#include <string.h>
#if !defined(HAVE_STRCHR)
#define strchr index
#endif /* !HAVE_STRCHR */
#endif /* STDC_HEADERS */

#include "header.h"
#include "keynote.h"

#if defined(HAVE___B64_NTOP)
int __b64_ntop __P((u_char const *, size_t, char *, size_t));
int __b64_pton __P((char const *, u_char *, size_t));

int
kn_encode_base64(src, srclength, target, targsize)
unsigned char const *src;
unsigned int srclength;
char *target;
unsigned int targsize;
{
    int i;

    i = __b64_ntop(src, srclength, target, targsize);
    if (i == -1)
      keynote_errno = ERROR_SYNTAX;
    return i;
}

int
kn_decode_base64(src, target, targsize)
char const *src;
unsigned char *target;
unsigned int targsize;
{
    int i;

    i = __b64_pton(src, target, targsize);
    if (i == -1)
      keynote_errno = ERROR_SYNTAX;
    return i;
}
#else /* HAVE___B64_NTOP */
#define Assert(Cond) if (!(Cond)) { keynote_errno = ERROR_SYNTAX; return -1; }

static const char Base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

/* (From RFC1521 and draft-ietf-dnssec-secext-03.txt)
   The following encoding technique is taken from RFC 1521 by Borenstein
   and Freed.  It is reproduced here in a slightly edited form for
   convenience.

   A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string.

                         Table 1: The Base64 Alphabet

      Value Encoding  Value Encoding  Value Encoding  Value Encoding
          0 A            17 R            34 i            51 z
          1 B            18 S            35 j            52 0
          2 C            19 T            36 k            53 1
          3 D            20 U            37 l            54 2
          4 E            21 V            38 m            55 3
          5 F            22 W            39 n            56 4
          6 G            23 X            40 o            57 5
          7 H            24 Y            41 p            58 6
          8 I            25 Z            42 q            59 7
          9 J            26 a            43 r            60 8
         10 K            27 b            44 s            61 9
         11 L            28 c            45 t            62 +
         12 M            29 d            46 u            63 /
         13 N            30 e            47 v
         14 O            31 f            48 w         (pad) =
         15 P            32 g            49 x
         16 Q            33 h            50 y

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a quantity.  When fewer than 24 input
   bits are available in an input group, zero bits are added (on the
   right) to form an integral number of 6-bit groups.  Padding at the
   end of the data is performed using the '=' character.

   Since all base64 input is an integral number of octets, only the
         -------------------------------------------------                       
   following cases can arise:
   
       (1) the final quantum of encoding input is an integral
           multiple of 24 bits; here, the final unit of encoded
	   output will be an integral multiple of 4 characters
	   with no "=" padding,
       (2) the final quantum of encoding input is exactly 8 bits;
           here, the final unit of encoded output will be two
	   characters followed by two "=" padding characters, or
       (3) the final quantum of encoding input is exactly 16 bits;
           here, the final unit of encoded output will be three
	   characters followed by one "=" padding character.
   */

int
kn_encode_base64(src, srclength, target, targsize)
unsigned char const *src;
unsigned int srclength;
char *target;
unsigned int targsize;
{
    unsigned int datalength = 0;
    unsigned char input[3];
    unsigned char output[4];
    int i;

    keynote_errno = 0;
    while (2 < srclength) {
	input[0] = *src++;
	input[1] = *src++;
	input[2] = *src++;
	srclength -= 3;

	output[0] = input[0] >> 2;
	output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
	output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
	output[3] = input[2] & 0x3f;
	Assert(output[0] < 64);
	Assert(output[1] < 64);
	Assert(output[2] < 64);
	Assert(output[3] < 64);

	if (datalength + 4 > targsize)
	{
	  keynote_errno = ERROR_SYNTAX;
	  return (-1);
	}
	target[datalength++] = Base64[output[0]];
	target[datalength++] = Base64[output[1]];
	target[datalength++] = Base64[output[2]];
	target[datalength++] = Base64[output[3]];
    }
    
    /* Now we worry about padding. */
    if (0 != srclength) {
	/* Get what's left. */
	input[0] = input[1] = input[2] = '\0';
	for (i = 0; i < srclength; i++)
	  input[i] = *src++;
	
	output[0] = input[0] >> 2;
	output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
	output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
	Assert(output[0] < 64);
	Assert(output[1] < 64);
	Assert(output[2] < 64);

	if (datalength + 4 > targsize)
	{
	  keynote_errno = ERROR_SYNTAX;
	  return (-1);
	}
	target[datalength++] = Base64[output[0]];
	target[datalength++] = Base64[output[1]];
	if (srclength == 1)
	  target[datalength++] = Pad64;
	else
	  target[datalength++] = Base64[output[2]];
	target[datalength++] = Pad64;
    }
    if (datalength >= targsize)
    {
      keynote_errno = ERROR_SYNTAX;
      return (-1);
    }
    target[datalength] = '\0';	/* Returned value doesn't count \0. */
    return (datalength);
}

/* skips all whitespace anywhere.
   converts characters, four at a time, starting at (or after)
   src from base - 64 numbers into three 8 bit bytes in the target area.
   it returns the number of data bytes stored at the target, or -1 on error.
 */

int
kn_decode_base64(src, target, targsize)
char const *src;
unsigned char *target;
unsigned int targsize;
{
    int tarindex, state, ch;
    char *pos;

    keynote_errno = 0;
    state = 0;
    tarindex = 0;

    while ((ch = *src++) != '\0') {
	if (isspace(ch))	/* Skip whitespace anywhere. */
	  continue;

	if (ch == Pad64)
	  break;

	pos = strchr(Base64, ch);
	if (pos == 0) 		/* A non-base64 character. */
	{
	  keynote_errno = ERROR_SYNTAX;
	  return (-1);
	}
	switch (state) {
	    case 0:
		if (target) {
		    if (tarindex >= targsize)
		    {
		      keynote_errno = ERROR_SYNTAX;
		      return (-1);
		    }
		    target[tarindex] = (pos - Base64) << 2;
		}
		state = 1;
		break;
	    case 1:
		if (target) {
		    if (tarindex + 1 >= targsize)
		    {
		      keynote_errno = ERROR_SYNTAX;
		      return (-1);
		    }
		    target[tarindex]   |=  (pos - Base64) >> 4;
		    target[tarindex+1]  = ((pos - Base64) & 0x0f)
					  << 4 ;
		}
		tarindex++;
		state = 2;
		break;
	    case 2:
		if (target) {
		    if (tarindex + 1 >= targsize)
		    {
		      keynote_errno = ERROR_SYNTAX;
		      return (-1);
		    }
		    target[tarindex]   |=  (pos - Base64) >> 2;
		    target[tarindex+1]  = ((pos - Base64) & 0x03)
					  << 6;
		}
		tarindex++;
		state = 3;
		break;
	    case 3:
		if (target) {
		    if (tarindex >= targsize)
		    {
		      keynote_errno = ERROR_SYNTAX;
		      return (-1);
		    }
		    target[tarindex] |= (pos - Base64);
		}
		tarindex++;
		state = 0;
		break;
	}
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == Pad64) {		/* We got a pad char. */
	ch = *src++;		/* Skip it, get next. */
	switch (state) {
	    case 0:		/* Invalid = in first position */
	    case 1:		/* Invalid = in second position */
	 	keynote_errno = ERROR_SYNTAX;
		return (-1);

	    case 2:		/* Valid, means one byte of info */
		/* Skip any number of spaces. */
		for (; ch != '\0'; ch = *src++)
		  if (!isspace(ch))
		    break;
		/* Make sure there is another trailing = sign. */
		if (ch != Pad64)
		{
		  keynote_errno = ERROR_SYNTAX;
		  return (-1);
		}
		ch = *src++;		/* Skip the = */
		/* Fall through to "single trailing =" case. */
		/* FALLTHROUGH */

	    case 3:		/* Valid, means two bytes of info */
		/*
		 * We know this char is an =.  Is there anything but
		 * whitespace after it?
		 */
		for (; ch != '\0'; ch = *src++)
		  if (!isspace(ch))
		  {
		    keynote_errno = ERROR_SYNTAX;
		    return (-1);
		  }

		/*
		 * Now make sure for cases 2 and 3 that the "extra"
		 * bits that slopped past the last full byte were
		 * zeros.  If we don't check them, they become a
		 * subliminal channel.
		 */
		if (target && target[tarindex] != 0)
		{
		  keynote_errno = ERROR_SYNTAX;
		  return (-1);
		}
	}
    } else {
	/*
	 * We ended by seeing the end of the string.  Make sure we
	 * have no partial bytes lying around.
	 */
	if (state != 0)
	{
	  keynote_errno = ERROR_SYNTAX;
	  return (-1);
	}
    }

    return (tarindex);
}
#endif /* HAVE___B64_NTOP */
