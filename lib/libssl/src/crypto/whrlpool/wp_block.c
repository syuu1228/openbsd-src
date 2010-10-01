/**
 * The Whirlpool hashing function.
 *
 * <P>
 * <b>References</b>
 *
 * <P>
 * The Whirlpool algorithm was developed by
 * <a href="mailto:pbarreto@scopus.com.br">Paulo S. L. M. Barreto</a> and
 * <a href="mailto:vincent.rijmen@cryptomathic.com">Vincent Rijmen</a>.
 *
 * See
 *      P.S.L.M. Barreto, V. Rijmen,
 *      ``The Whirlpool hashing function,''
 *      NESSIE submission, 2000 (tweaked version, 2001),
 *      <https://www.cosic.esat.kuleuven.ac.be/nessie/workshop/submissions/whirlpool.zip>
 *
 * Based on "@version 3.0 (2003.03.12)" by Paulo S.L.M. Barreto and
 * Vincent Rijmen. Lookup "reference implementations" on
 * <http://planeta.terra.com.br/informatica/paulobarreto/>
 *
 * =============================================================================
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "wp_locl.h"
#include <string.h>

typedef unsigned char		u8;
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__MINGW32)
typedef unsigned __int64	u64;
#elif defined(__arch64__)
typedef unsigned long		u64;
#else
typedef unsigned long long	u64;
#endif

#define ROUNDS	10

#define STRICT_ALIGNMENT
#if defined(__i386) || defined(__i386__) || \
    defined(__x86_64) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64)
/* Well, formally there're couple of other architectures, which permit
 * unaligned loads, specifically those not crossing cache lines, IA-64
 * and PowerPC... */
#  undef STRICT_ALIGNMENT
#endif

#undef SMALL_REGISTER_BANK
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#  define SMALL_REGISTER_BANK
#  if defined(WHIRLPOOL_ASM)
#    ifndef OPENSSL_SMALL_FOOTPRINT
#      define OPENSSL_SMALL_FOOTPRINT	/* it appears that for elder non-MMX
					   CPUs this is actually faster! */
#    endif
#    define GO_FOR_MMX(ctx,inp,num)	do {			\
	extern unsigned long OPENSSL_ia32cap_P;			\
	void whirlpool_block_mmx(void *,const void *,size_t);	\
	if (!(OPENSSL_ia32cap_P & (1<<23)))	break;		\
        whirlpool_block_mmx(ctx->H.c,inp,num);	return;		\
					} while (0)
#  endif
#endif

#undef ROTATE
#if defined(_MSC_VER)
#  if defined(_WIN64)	/* applies to both IA-64 and AMD64 */
#    pragma intrinsic(_rotl64)
#    define ROTATE(a,n)	_rotl64((a),n)
#  endif
#elif defined(__GNUC__) && __GNUC__>=2
#  if defined(__x86_64) || defined(__x86_64__)
#    if defined(L_ENDIAN)
#      define ROTATE(a,n)	({ u64 ret; asm ("rolq %1,%0"	\
				   : "=r"(ret) : "J"(n),"0"(a) : "cc"); ret; })
#    elif defined(B_ENDIAN)
       /* Most will argue that x86_64 is always little-endian. Well,
        * yes, but then we have stratus.com who has modified gcc to
	* "emulate" big-endian on x86. Is there evidence that they
	* [or somebody else] won't do same for x86_64? Naturally no.
	* And this line is waiting ready for that brave soul:-) */
#      define ROTATE(a,n)	({ u64 ret; asm ("rorq %1,%0"	\
				   : "=r"(ret) : "J"(n),"0"(a) : "cc"); ret; })
#    endif
#  elif defined(__ia64) || defined(__ia64__)
#    if defined(L_ENDIAN)
#      define ROTATE(a,n)	({ u64 ret; asm ("shrp %0=%1,%1,%2"	\
				   : "=r"(ret) : "r"(a),"M"(64-(n))); ret; })
#    elif defined(B_ENDIAN)
#      define ROTATE(a,n)	({ u64 ret; asm ("shrp %0=%1,%1,%2"	\
				   : "=r"(ret) : "r"(a),"M"(n)); ret; })
#    endif
#  endif
#endif

#if defined(OPENSSL_SMALL_FOOTPRINT)
#  if !defined(ROTATE)
#    if defined(L_ENDIAN)	/* little-endians have to rotate left */
#      define ROTATE(i,n)	((i)<<(n) ^ (i)>>(64-n))
#    elif defined(B_ENDIAN)	/* big-endians have to rotate right */
#      define ROTATE(i,n)	((i)>>(n) ^ (i)<<(64-n))
#    endif
#  endif
#  if defined(ROTATE) && !defined(STRICT_ALIGNMENT)
#    define STRICT_ALIGNMENT	/* ensure smallest table size */
#  endif
#endif

/*
 * Table size depends on STRICT_ALIGNMENT and whether or not endian-
 * specific ROTATE macro is defined. If STRICT_ALIGNMENT is not
 * defined, which is normally the case on x86[_64] CPUs, the table is
 * 4KB large unconditionally. Otherwise if ROTATE is defined, the
 * table is 2KB large, and otherwise - 16KB. 2KB table requires a
 * whole bunch of additional rotations, but I'm willing to "trade,"
 * because 16KB table certainly trashes L1 cache. I wish all CPUs
 * could handle unaligned load as 4KB table doesn't trash the cache,
 * nor does it require additional rotations.
 */
/*
 * Note that every Cn macro expands as two loads: one byte load and
 * one quadword load. One can argue that that many single-byte loads
 * is too excessive, as one could load a quadword and "milk" it for
 * eight 8-bit values instead. Well, yes, but in order to do so *and*
 * avoid excessive loads you have to accomodate a handful of 64-bit
 * values in the register bank and issue a bunch of shifts and mask.
 * It's a tradeoff: loads vs. shift and mask in big register bank[!].
 * On most CPUs eight single-byte loads are faster and I let other
 * ones to depend on smart compiler to fold byte loads if beneficial.
 * Hand-coded assembler would be another alternative:-)
 */
#ifdef STRICT_ALIGNMENT
#  if defined(ROTATE)
#    define N	1
#    define LL(c0,c1,c2,c3,c4,c5,c6,c7)	c0,c1,c2,c3,c4,c5,c6,c7
#    define C0(K,i)	(Cx.q[K.c[(i)*8+0]])
#    define C1(K,i)	ROTATE(Cx.q[K.c[(i)*8+1]],8)
#    define C2(K,i)	ROTATE(Cx.q[K.c[(i)*8+2]],16)
#    define C3(K,i)	ROTATE(Cx.q[K.c[(i)*8+3]],24)
#    define C4(K,i)	ROTATE(Cx.q[K.c[(i)*8+4]],32)
#    define C5(K,i)	ROTATE(Cx.q[K.c[(i)*8+5]],40)
#    define C6(K,i)	ROTATE(Cx.q[K.c[(i)*8+6]],48)
#    define C7(K,i)	ROTATE(Cx.q[K.c[(i)*8+7]],56)
#  else
#    define N	8
#    define LL(c0,c1,c2,c3,c4,c5,c6,c7)	c0,c1,c2,c3,c4,c5,c6,c7, \
					c7,c0,c1,c2,c3,c4,c5,c6, \
					c6,c7,c0,c1,c2,c3,c4,c5, \
					c5,c6,c7,c0,c1,c2,c3,c4, \
					c4,c5,c6,c7,c0,c1,c2,c3, \
					c3,c4,c5,c6,c7,c0,c1,c2, \
					c2,c3,c4,c5,c6,c7,c0,c1, \
					c1,c2,c3,c4,c5,c6,c7,c0
#    define C0(K,i)	(Cx.q[0+8*K.c[(i)*8+0]])
#    define C1(K,i)	(Cx.q[1+8*K.c[(i)*8+1]])
#    define C2(K,i)	(Cx.q[2+8*K.c[(i)*8+2]])
#    define C3(K,i)	(Cx.q[3+8*K.c[(i)*8+3]])
#    define C4(K,i)	(Cx.q[4+8*K.c[(i)*8+4]])
#    define C5(K,i)	(Cx.q[5+8*K.c[(i)*8+5]])
#    define C6(K,i)	(Cx.q[6+8*K.c[(i)*8+6]])
#    define C7(K,i)	(Cx.q[7+8*K.c[(i)*8+7]])
#  endif
#else
#  define N	2
#  define LL(c0,c1,c2,c3,c4,c5,c6,c7)	c0,c1,c2,c3,c4,c5,c6,c7, \
					c0,c1,c2,c3,c4,c5,c6,c7
#  define C0(K,i)	(((u64*)(Cx.c+0))[2*K.c[(i)*8+0]])
#  define C1(K,i)	(((u64*)(Cx.c+7))[2*K.c[(i)*8+1]])
#  define C2(K,i)	(((u64*)(Cx.c+6))[2*K.c[(i)*8+2]])
#  define C3(K,i)	(((u64*)(Cx.c+5))[2*K.c[(i)*8+3]])
#  define C4(K,i)	(((u64*)(Cx.c+4))[2*K.c[(i)*8+4]])
#  define C5(K,i)	(((u64*)(Cx.c+3))[2*K.c[(i)*8+5]])
#  define C6(K,i)	(((u64*)(Cx.c+2))[2*K.c[(i)*8+6]])
#  define C7(K,i)	(((u64*)(Cx.c+1))[2*K.c[(i)*8+7]])
#endif

static const
union	{
	u8	c[(256*N+ROUNDS)*sizeof(u64)];
	u64	q[(256*N+ROUNDS)];
	} Cx = { {
	/* Note endian-neutral representation:-) */
	LL(0x18,0x18,0x60,0x18,0xc0,0x78,0x30,0xd8),
	LL(0x23,0x23,0x8c,0x23,0x05,0xaf,0x46,0x26),
	LL(0xc6,0xc6,0x3f,0xc6,0x7e,0xf9,0x91,0xb8),
	LL(0xe8,0xe8,0x87,0xe8,0x13,0x6f,0xcd,0xfb),
	LL(0x87,0x87,0x26,0x87,0x4c,0xa1,0x13,0xcb),
	LL(0xb8,0xb8,0xda,0xb8,0xa9,0x62,0x6d,0x11),
	LL(0x01,0x01,0x04,0x01,0x08,0x05,0x02,0x09),
	LL(0x4f,0x4f,0x21,0x4f,0x42,0x6e,0x9e,0x0d),
	LL(0x36,0x36,0xd8,0x36,0xad,0xee,0x6c,0x9b),
	LL(0xa6,0xa6,0xa2,0xa6,0x59,0x04,0x51,0xff),
	LL(0xd2,0xd2,0x6f,0xd2,0xde,0xbd,0xb9,0x0c),
	LL(0xf5,0xf5,0xf3,0xf5,0xfb,0x06,0xf7,0x0e),
	LL(0x79,0x79,0xf9,0x79,0xef,0x80,0xf2,0x96),
	LL(0x6f,0x6f,0xa1,0x6f,0x5f,0xce,0xde,0x30),
	LL(0x91,0x91,0x7e,0x91,0xfc,0xef,0x3f,0x6d),
	LL(0x52,0x52,0x55,0x52,0xaa,0x07,0xa4,0xf8),
	LL(0x60,0x60,0x9d,0x60,0x27,0xfd,0xc0,0x47),
	LL(0xbc,0xbc,0xca,0xbc,0x89,0x76,0x65,0x35),
	LL(0x9b,0x9b,0x56,0x9b,0xac,0xcd,0x2b,0x37),
	LL(0x8e,0x8e,0x02,0x8e,0x04,0x8c,0x01,0x8a),
	LL(0xa3,0xa3,0xb6,0xa3,0x71,0x15,0x5b,0xd2),
	LL(0x0c,0x0c,0x30,0x0c,0x60,0x3c,0x18,0x6c),
	LL(0x7b,0x7b,0xf1,0x7b,0xff,0x8a,0xf6,0x84),
	LL(0x35,0x35,0xd4,0x35,0xb5,0xe1,0x6a,0x80),
	LL(0x1d,0x1d,0x74,0x1d,0xe8,0x69,0x3a,0xf5),
	LL(0xe0,0xe0,0xa7,0xe0,0x53,0x47,0xdd,0xb3),
	LL(0xd7,0xd7,0x7b,0xd7,0xf6,0xac,0xb3,0x21),
	LL(0xc2,0xc2,0x2f,0xc2,0x5e,0xed,0x99,0x9c),
	LL(0x2e,0x2e,0xb8,0x2e,0x6d,0x96,0x5c,0x43),
	LL(0x4b,0x4b,0x31,0x4b,0x62,0x7a,0x96,0x29),
	LL(0xfe,0xfe,0xdf,0xfe,0xa3,0x21,0xe1,0x5d),
	LL(0x57,0x57,0x41,0x57,0x82,0x16,0xae,0xd5),
	LL(0x15,0x15,0x54,0x15,0xa8,0x41,0x2a,0xbd),
	LL(0x77,0x77,0xc1,0x77,0x9f,0xb6,0xee,0xe8),
	LL(0x37,0x37,0xdc,0x37,0xa5,0xeb,0x6e,0x92),
	LL(0xe5,0xe5,0xb3,0xe5,0x7b,0x56,0xd7,0x9e),
	LL(0x9f,0x9f,0x46,0x9f,0x8c,0xd9,0x23,0x13),
	LL(0xf0,0xf0,0xe7,0xf0,0xd3,0x17,0xfd,0x23),
	LL(0x4a,0x4a,0x35,0x4a,0x6a,0x7f,0x94,0x20),
	LL(0xda,0xda,0x4f,0xda,0x9e,0x95,0xa9,0x44),
	LL(0x58,0x58,0x7d,0x58,0xfa,0x25,0xb0,0xa2),
	LL(0xc9,0xc9,0x03,0xc9,0x06,0xca,0x8f,0xcf),
	LL(0x29,0x29,0xa4,0x29,0x55,0x8d,0x52,0x7c),
	LL(0x0a,0x0a,0x28,0x0a,0x50,0x22,0x14,0x5a),
	LL(0xb1,0xb1,0xfe,0xb1,0xe1,0x4f,0x7f,0x50),
	LL(0xa0,0xa0,0xba,0xa0,0x69,0x1a,0x5d,0xc9),
	LL(0x6b,0x6b,0xb1,0x6b,0x7f,0xda,0xd6,0x14),
	LL(0x85,0x85,0x2e,0x85,0x5c,0xab,0x17,0xd9),
	LL(0xbd,0xbd,0xce,0xbd,0x81,0x73,0x67,0x3c),
	LL(0x5d,0x5d,0x69,0x5d,0xd2,0x34,0xba,0x8f),
	LL(0x10,0x10,0x40,0x10,0x80,0x50,0x20,0x90),
	LL(0xf4,0xf4,0xf7,0xf4,0xf3,0x03,0xf5,0x07),
	LL(0xcb,0xcb,0x0b,0xcb,0x16,0xc0,0x8b,0xdd),
	LL(0x3e,0x3e,0xf8,0x3e,0xed,0xc6,0x7c,0xd3),
	LL(0x05,0x05,0x14,0x05,0x28,0x11,0x0a,0x2d),
	LL(0x67,0x67,0x81,0x67,0x1f,0xe6,0xce,0x78),
	LL(0xe4,0xe4,0xb7,0xe4,0x73,0x53,0xd5,0x97),
	LL(0x27,0x27,0x9c,0x27,0x25,0xbb,0x4e,0x02),
	LL(0x41,0x41,0x19,0x41,0x32,0x58,0x82,0x73),
	LL(0x8b,0x8b,0x16,0x8b,0x2c,0x9d,0x0b,0xa7),
	LL(0xa7,0xa7,0xa6,0xa7,0x51,0x01,0x53,0xf6),
	LL(0x7d,0x7d,0xe9,0x7d,0xcf,0x94,0xfa,0xb2),
	LL(0x95,0x95,0x6e,0x95,0xdc,0xfb,0x37,0x49),
	LL(0xd8,0xd8,0x47,0xd8,0x8e,0x9f,0xad,0x56),
	LL(0xfb,0xfb,0xcb,0xfb,0x8b,0x30,0xeb,0x70),
	LL(0xee,0xee,0x9f,0xee,0x23,0x71,0xc1,0xcd),
	LL(0x7c,0x7c,0xed,0x7c,0xc7,0x91,0xf8,0xbb),
	LL(0x66,0x66,0x85,0x66,0x17,0xe3,0xcc,0x71),
	LL(0xdd,0xdd,0x53,0xdd,0xa6,0x8e,0xa7,0x7b),
	LL(0x17,0x17,0x5c,0x17,0xb8,0x4b,0x2e,0xaf),
	LL(0x47,0x47,0x01,0x47,0x02,0x46,0x8e,0x45),
	LL(0x9e,0x9e,0x42,0x9e,0x84,0xdc,0x21,0x1a),
	LL(0xca,0xca,0x0f,0xca,0x1e,0xc5,0x89,0xd4),
	LL(0x2d,0x2d,0xb4,0x2d,0x75,0x99,0x5a,0x58),
	LL(0xbf,0xbf,0xc6,0xbf,0x91,0x79,0x63,0x2e),
	LL(0x07,0x07,0x1c,0x07,0x38,0x1b,0x0e,0x3f),
	LL(0xad,0xad,0x8e,0xad,0x01,0x23,0x47,0xac),
	LL(0x5a,0x5a,0x75,0x5a,0xea,0x2f,0xb4,0xb0),
	LL(0x83,0x83,0x36,0x83,0x6c,0xb5,0x1b,0xef),
	LL(0x33,0x33,0xcc,0x33,0x85,0xff,0x66,0xb6),
	LL(0x63,0x63,0x91,0x63,0x3f,0xf2,0xc6,0x5c),
	LL(0x02,0x02,0x08,0x02,0x10,0x0a,0x04,0x12),
	LL(0xaa,0xaa,0x92,0xaa,0x39,0x38,0x49,0x93),
	LL(0x71,0x71,0xd9,0x71,0xaf,0xa8,0xe2,0xde),
	LL(0xc8,0xc8,0x07,0xc8,0x0e,0xcf,0x8d,0xc6),
	LL(0x19,0x19,0x64,0x19,0xc8,0x7d,0x32,0xd1),
	LL(0x49,0x49,0x39,0x49,0x72,0x70,0x92,0x3b),
	LL(0xd9,0xd9,0x43,0xd9,0x86,0x9a,0xaf,0x5f),
	LL(0xf2,0xf2,0xef,0xf2,0xc3,0x1d,0xf9,0x31),
	LL(0xe3,0xe3,0xab,0xe3,0x4b,0x48,0xdb,0xa8),
	LL(0x5b,0x5b,0x71,0x5b,0xe2,0x2a,0xb6,0xb9),
	LL(0x88,0x88,0x1a,0x88,0x34,0x92,0x0d,0xbc),
	LL(0x9a,0x9a,0x52,0x9a,0xa4,0xc8,0x29,0x3e),
	LL(0x26,0x26,0x98,0x26,0x2d,0xbe,0x4c,0x0b),
	LL(0x32,0x32,0xc8,0x32,0x8d,0xfa,0x64,0xbf),
	LL(0xb0,0xb0,0xfa,0xb0,0xe9,0x4a,0x7d,0x59),
	LL(0xe9,0xe9,0x83,0xe9,0x1b,0x6a,0xcf,0xf2),
	LL(0x0f,0x0f,0x3c,0x0f,0x78,0x33,0x1e,0x77),
	LL(0xd5,0xd5,0x73,0xd5,0xe6,0xa6,0xb7,0x33),
	LL(0x80,0x80,0x3a,0x80,0x74,0xba,0x1d,0xf4),
	LL(0xbe,0xbe,0xc2,0xbe,0x99,0x7c,0x61,0x27),
	LL(0xcd,0xcd,0x13,0xcd,0x26,0xde,0x87,0xeb),
	LL(0x34,0x34,0xd0,0x34,0xbd,0xe4,0x68,0x89),
	LL(0x48,0x48,0x3d,0x48,0x7a,0x75,0x90,0x32),
	LL(0xff,0xff,0xdb,0xff,0xab,0x24,0xe3,0x54),
	LL(0x7a,0x7a,0xf5,0x7a,0xf7,0x8f,0xf4,0x8d),
	LL(0x90,0x90,0x7a,0x90,0xf4,0xea,0x3d,0x64),
	LL(0x5f,0x5f,0x61,0x5f,0xc2,0x3e,0xbe,0x9d),
	LL(0x20,0x20,0x80,0x20,0x1d,0xa0,0x40,0x3d),
	LL(0x68,0x68,0xbd,0x68,0x67,0xd5,0xd0,0x0f),
	LL(0x1a,0x1a,0x68,0x1a,0xd0,0x72,0x34,0xca),
	LL(0xae,0xae,0x82,0xae,0x19,0x2c,0x41,0xb7),
	LL(0xb4,0xb4,0xea,0xb4,0xc9,0x5e,0x75,0x7d),
	LL(0x54,0x54,0x4d,0x54,0x9a,0x19,0xa8,0xce),
	LL(0x93,0x93,0x76,0x93,0xec,0xe5,0x3b,0x7f),
	LL(0x22,0x22,0x88,0x22,0x0d,0xaa,0x44,0x2f),
	LL(0x64,0x64,0x8d,0x64,0x07,0xe9,0xc8,0x63),
	LL(0xf1,0xf1,0xe3,0xf1,0xdb,0x12,0xff,0x2a),
	LL(0x73,0x73,0xd1,0x73,0xbf,0xa2,0xe6,0xcc),
	LL(0x12,0x12,0x48,0x12,0x90,0x5a,0x24,0x82),
	LL(0x40,0x40,0x1d,0x40,0x3a,0x5d,0x80,0x7a),
	LL(0x08,0x08,0x20,0x08,0x40,0x28,0x10,0x48),
	LL(0xc3,0xc3,0x2b,0xc3,0x56,0xe8,0x9b,0x95),
	LL(0xec,0xec,0x97,0xec,0x33,0x7b,0xc5,0xdf),
	LL(0xdb,0xdb,0x4b,0xdb,0x96,0x90,0xab,0x4d),
	LL(0xa1,0xa1,0xbe,0xa1,0x61,0x1f,0x5f,0xc0),
	LL(0x8d,0x8d,0x0e,0x8d,0x1c,0x83,0x07,0x91),
	LL(0x3d,0x3d,0xf4,0x3d,0xf5,0xc9,0x7a,0xc8),
	LL(0x97,0x97,0x66,0x97,0xcc,0xf1,0x33,0x5b),
	LL(0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00),
	LL(0xcf,0xcf,0x1b,0xcf,0x36,0xd4,0x83,0xf9),
	LL(0x2b,0x2b,0xac,0x2b,0x45,0x87,0x56,0x6e),
	LL(0x76,0x76,0xc5,0x76,0x97,0xb3,0xec,0xe1),
	LL(0x82,0x82,0x32,0x82,0x64,0xb0,0x19,0xe6),
	LL(0xd6,0xd6,0x7f,0xd6,0xfe,0xa9,0xb1,0x28),
	LL(0x1b,0x1b,0x6c,0x1b,0xd8,0x77,0x36,0xc3),
	LL(0xb5,0xb5,0xee,0xb5,0xc1,0x5b,0x77,0x74),
	LL(0xaf,0xaf,0x86,0xaf,0x11,0x29,0x43,0xbe),
	LL(0x6a,0x6a,0xb5,0x6a,0x77,0xdf,0xd4,0x1d),
	LL(0x50,0x50,0x5d,0x50,0xba,0x0d,0xa0,0xea),
	LL(0x45,0x45,0x09,0x45,0x12,0x4c,0x8a,0x57),
	LL(0xf3,0xf3,0xeb,0xf3,0xcb,0x18,0xfb,0x38),
	LL(0x30,0x30,0xc0,0x30,0x9d,0xf0,0x60,0xad),
	LL(0xef,0xef,0x9b,0xef,0x2b,0x74,0xc3,0xc4),
	LL(0x3f,0x3f,0xfc,0x3f,0xe5,0xc3,0x7e,0xda),
	LL(0x55,0x55,0x49,0x55,0x92,0x1c,0xaa,0xc7),
	LL(0xa2,0xa2,0xb2,0xa2,0x79,0x10,0x59,0xdb),
	LL(0xea,0xea,0x8f,0xea,0x03,0x65,0xc9,0xe9),
	LL(0x65,0x65,0x89,0x65,0x0f,0xec,0xca,0x6a),
	LL(0xba,0xba,0xd2,0xba,0xb9,0x68,0x69,0x03),
	LL(0x2f,0x2f,0xbc,0x2f,0x65,0x93,0x5e,0x4a),
	LL(0xc0,0xc0,0x27,0xc0,0x4e,0xe7,0x9d,0x8e),
	LL(0xde,0xde,0x5f,0xde,0xbe,0x81,0xa1,0x60),
	LL(0x1c,0x1c,0x70,0x1c,0xe0,0x6c,0x38,0xfc),
	LL(0xfd,0xfd,0xd3,0xfd,0xbb,0x2e,0xe7,0x46),
	LL(0x4d,0x4d,0x29,0x4d,0x52,0x64,0x9a,0x1f),
	LL(0x92,0x92,0x72,0x92,0xe4,0xe0,0x39,0x76),
	LL(0x75,0x75,0xc9,0x75,0x8f,0xbc,0xea,0xfa),
	LL(0x06,0x06,0x18,0x06,0x30,0x1e,0x0c,0x36),
	LL(0x8a,0x8a,0x12,0x8a,0x24,0x98,0x09,0xae),
	LL(0xb2,0xb2,0xf2,0xb2,0xf9,0x40,0x79,0x4b),
	LL(0xe6,0xe6,0xbf,0xe6,0x63,0x59,0xd1,0x85),
	LL(0x0e,0x0e,0x38,0x0e,0x70,0x36,0x1c,0x7e),
	LL(0x1f,0x1f,0x7c,0x1f,0xf8,0x63,0x3e,0xe7),
	LL(0x62,0x62,0x95,0x62,0x37,0xf7,0xc4,0x55),
	LL(0xd4,0xd4,0x77,0xd4,0xee,0xa3,0xb5,0x3a),
	LL(0xa8,0xa8,0x9a,0xa8,0x29,0x32,0x4d,0x81),
	LL(0x96,0x96,0x62,0x96,0xc4,0xf4,0x31,0x52),
	LL(0xf9,0xf9,0xc3,0xf9,0x9b,0x3a,0xef,0x62),
	LL(0xc5,0xc5,0x33,0xc5,0x66,0xf6,0x97,0xa3),
	LL(0x25,0x25,0x94,0x25,0x35,0xb1,0x4a,0x10),
	LL(0x59,0x59,0x79,0x59,0xf2,0x20,0xb2,0xab),
	LL(0x84,0x84,0x2a,0x84,0x54,0xae,0x15,0xd0),
	LL(0x72,0x72,0xd5,0x72,0xb7,0xa7,0xe4,0xc5),
	LL(0x39,0x39,0xe4,0x39,0xd5,0xdd,0x72,0xec),
	LL(0x4c,0x4c,0x2d,0x4c,0x5a,0x61,0x98,0x16),
	LL(0x5e,0x5e,0x65,0x5e,0xca,0x3b,0xbc,0x94),
	LL(0x78,0x78,0xfd,0x78,0xe7,0x85,0xf0,0x9f),
	LL(0x38,0x38,0xe0,0x38,0xdd,0xd8,0x70,0xe5),
	LL(0x8c,0x8c,0x0a,0x8c,0x14,0x86,0x05,0x98),
	LL(0xd1,0xd1,0x63,0xd1,0xc6,0xb2,0xbf,0x17),
	LL(0xa5,0xa5,0xae,0xa5,0x41,0x0b,0x57,0xe4),
	LL(0xe2,0xe2,0xaf,0xe2,0x43,0x4d,0xd9,0xa1),
	LL(0x61,0x61,0x99,0x61,0x2f,0xf8,0xc2,0x4e),
	LL(0xb3,0xb3,0xf6,0xb3,0xf1,0x45,0x7b,0x42),
	LL(0x21,0x21,0x84,0x21,0x15,0xa5,0x42,0x34),
	LL(0x9c,0x9c,0x4a,0x9c,0x94,0xd6,0x25,0x08),
	LL(0x1e,0x1e,0x78,0x1e,0xf0,0x66,0x3c,0xee),
	LL(0x43,0x43,0x11,0x43,0x22,0x52,0x86,0x61),
	LL(0xc7,0xc7,0x3b,0xc7,0x76,0xfc,0x93,0xb1),
	LL(0xfc,0xfc,0xd7,0xfc,0xb3,0x2b,0xe5,0x4f),
	LL(0x04,0x04,0x10,0x04,0x20,0x14,0x08,0x24),
	LL(0x51,0x51,0x59,0x51,0xb2,0x08,0xa2,0xe3),
	LL(0x99,0x99,0x5e,0x99,0xbc,0xc7,0x2f,0x25),
	LL(0x6d,0x6d,0xa9,0x6d,0x4f,0xc4,0xda,0x22),
	LL(0x0d,0x0d,0x34,0x0d,0x68,0x39,0x1a,0x65),
	LL(0xfa,0xfa,0xcf,0xfa,0x83,0x35,0xe9,0x79),
	LL(0xdf,0xdf,0x5b,0xdf,0xb6,0x84,0xa3,0x69),
	LL(0x7e,0x7e,0xe5,0x7e,0xd7,0x9b,0xfc,0xa9),
	LL(0x24,0x24,0x90,0x24,0x3d,0xb4,0x48,0x19),
	LL(0x3b,0x3b,0xec,0x3b,0xc5,0xd7,0x76,0xfe),
	LL(0xab,0xab,0x96,0xab,0x31,0x3d,0x4b,0x9a),
	LL(0xce,0xce,0x1f,0xce,0x3e,0xd1,0x81,0xf0),
	LL(0x11,0x11,0x44,0x11,0x88,0x55,0x22,0x99),
	LL(0x8f,0x8f,0x06,0x8f,0x0c,0x89,0x03,0x83),
	LL(0x4e,0x4e,0x25,0x4e,0x4a,0x6b,0x9c,0x04),
	LL(0xb7,0xb7,0xe6,0xb7,0xd1,0x51,0x73,0x66),
	LL(0xeb,0xeb,0x8b,0xeb,0x0b,0x60,0xcb,0xe0),
	LL(0x3c,0x3c,0xf0,0x3c,0xfd,0xcc,0x78,0xc1),
	LL(0x81,0x81,0x3e,0x81,0x7c,0xbf,0x1f,0xfd),
	LL(0x94,0x94,0x6a,0x94,0xd4,0xfe,0x35,0x40),
	LL(0xf7,0xf7,0xfb,0xf7,0xeb,0x0c,0xf3,0x1c),
	LL(0xb9,0xb9,0xde,0xb9,0xa1,0x67,0x6f,0x18),
	LL(0x13,0x13,0x4c,0x13,0x98,0x5f,0x26,0x8b),
	LL(0x2c,0x2c,0xb0,0x2c,0x7d,0x9c,0x58,0x51),
	LL(0xd3,0xd3,0x6b,0xd3,0xd6,0xb8,0xbb,0x05),
	LL(0xe7,0xe7,0xbb,0xe7,0x6b,0x5c,0xd3,0x8c),
	LL(0x6e,0x6e,0xa5,0x6e,0x57,0xcb,0xdc,0x39),
	LL(0xc4,0xc4,0x37,0xc4,0x6e,0xf3,0x95,0xaa),
	LL(0x03,0x03,0x0c,0x03,0x18,0x0f,0x06,0x1b),
	LL(0x56,0x56,0x45,0x56,0x8a,0x13,0xac,0xdc),
	LL(0x44,0x44,0x0d,0x44,0x1a,0x49,0x88,0x5e),
	LL(0x7f,0x7f,0xe1,0x7f,0xdf,0x9e,0xfe,0xa0),
	LL(0xa9,0xa9,0x9e,0xa9,0x21,0x37,0x4f,0x88),
	LL(0x2a,0x2a,0xa8,0x2a,0x4d,0x82,0x54,0x67),
	LL(0xbb,0xbb,0xd6,0xbb,0xb1,0x6d,0x6b,0x0a),
	LL(0xc1,0xc1,0x23,0xc1,0x46,0xe2,0x9f,0x87),
	LL(0x53,0x53,0x51,0x53,0xa2,0x02,0xa6,0xf1),
	LL(0xdc,0xdc,0x57,0xdc,0xae,0x8b,0xa5,0x72),
	LL(0x0b,0x0b,0x2c,0x0b,0x58,0x27,0x16,0x53),
	LL(0x9d,0x9d,0x4e,0x9d,0x9c,0xd3,0x27,0x01),
	LL(0x6c,0x6c,0xad,0x6c,0x47,0xc1,0xd8,0x2b),
	LL(0x31,0x31,0xc4,0x31,0x95,0xf5,0x62,0xa4),
	LL(0x74,0x74,0xcd,0x74,0x87,0xb9,0xe8,0xf3),
	LL(0xf6,0xf6,0xff,0xf6,0xe3,0x09,0xf1,0x15),
	LL(0x46,0x46,0x05,0x46,0x0a,0x43,0x8c,0x4c),
	LL(0xac,0xac,0x8a,0xac,0x09,0x26,0x45,0xa5),
	LL(0x89,0x89,0x1e,0x89,0x3c,0x97,0x0f,0xb5),
	LL(0x14,0x14,0x50,0x14,0xa0,0x44,0x28,0xb4),
	LL(0xe1,0xe1,0xa3,0xe1,0x5b,0x42,0xdf,0xba),
	LL(0x16,0x16,0x58,0x16,0xb0,0x4e,0x2c,0xa6),
	LL(0x3a,0x3a,0xe8,0x3a,0xcd,0xd2,0x74,0xf7),
	LL(0x69,0x69,0xb9,0x69,0x6f,0xd0,0xd2,0x06),
	LL(0x09,0x09,0x24,0x09,0x48,0x2d,0x12,0x41),
	LL(0x70,0x70,0xdd,0x70,0xa7,0xad,0xe0,0xd7),
	LL(0xb6,0xb6,0xe2,0xb6,0xd9,0x54,0x71,0x6f),
	LL(0xd0,0xd0,0x67,0xd0,0xce,0xb7,0xbd,0x1e),
	LL(0xed,0xed,0x93,0xed,0x3b,0x7e,0xc7,0xd6),
	LL(0xcc,0xcc,0x17,0xcc,0x2e,0xdb,0x85,0xe2),
	LL(0x42,0x42,0x15,0x42,0x2a,0x57,0x84,0x68),
	LL(0x98,0x98,0x5a,0x98,0xb4,0xc2,0x2d,0x2c),
	LL(0xa4,0xa4,0xaa,0xa4,0x49,0x0e,0x55,0xed),
	LL(0x28,0x28,0xa0,0x28,0x5d,0x88,0x50,0x75),
	LL(0x5c,0x5c,0x6d,0x5c,0xda,0x31,0xb8,0x86),
	LL(0xf8,0xf8,0xc7,0xf8,0x93,0x3f,0xed,0x6b),
	LL(0x86,0x86,0x22,0x86,0x44,0xa4,0x11,0xc2),
#define RC	(&(Cx.q[256*N]))
	0x18,0x23,0xc6,0xe8,0x87,0xb8,0x01,0x4f,	/* rc[ROUNDS] */
	0x36,0xa6,0xd2,0xf5,0x79,0x6f,0x91,0x52,
	0x60,0xbc,0x9b,0x8e,0xa3,0x0c,0x7b,0x35,
	0x1d,0xe0,0xd7,0xc2,0x2e,0x4b,0xfe,0x57,
	0x15,0x77,0x37,0xe5,0x9f,0xf0,0x4a,0xda,
	0x58,0xc9,0x29,0x0a,0xb1,0xa0,0x6b,0x85,
	0xbd,0x5d,0x10,0xf4,0xcb,0x3e,0x05,0x67,
	0xe4,0x27,0x41,0x8b,0xa7,0x7d,0x95,0xd8,
	0xfb,0xee,0x7c,0x66,0xdd,0x17,0x47,0x9e,
	0xca,0x2d,0xbf,0x07,0xad,0x5a,0x83,0x33
	}
};

void whirlpool_block(WHIRLPOOL_CTX *ctx,const void *inp,size_t n)
	{
	int	r;
	const u8 *p=inp;
	union	{ u64 q[8]; u8 c[64]; } S,K,*H=(void *)ctx->H.q;

#ifdef GO_FOR_MMX
	GO_FOR_MMX(ctx,inp,n);
#endif
							do {
#ifdef OPENSSL_SMALL_FOOTPRINT
	u64	L[8];
	int	i;

	for (i=0;i<64;i++)	S.c[i] = (K.c[i] = H->c[i]) ^ p[i];
	for (r=0;r<ROUNDS;r++)
		{
		for (i=0;i<8;i++)
			{
			L[i]  = i ? 0 : RC[r];
			L[i] ^=	C0(K,i)       ^ C1(K,(i-1)&7) ^
				C2(K,(i-2)&7) ^ C3(K,(i-3)&7) ^
				C4(K,(i-4)&7) ^ C5(K,(i-5)&7) ^
				C6(K,(i-6)&7) ^ C7(K,(i-7)&7);
			}
		memcpy (K.q,L,64);
		for (i=0;i<8;i++)
			{
			L[i] ^= C0(S,i)       ^ C1(S,(i-1)&7) ^
				C2(S,(i-2)&7) ^ C3(S,(i-3)&7) ^
				C4(S,(i-4)&7) ^ C5(S,(i-5)&7) ^
				C6(S,(i-6)&7) ^ C7(S,(i-7)&7);
			}
		memcpy (S.q,L,64);
		}
	for (i=0;i<64;i++)	H->c[i] ^= S.c[i] ^ p[i];
#else
	u64	L0,L1,L2,L3,L4,L5,L6,L7;

#ifdef STRICT_ALIGNMENT
	if ((size_t)p & 7)
		{
		memcpy (S.c,p,64);
		S.q[0] ^= (K.q[0] = H->q[0]);
		S.q[1] ^= (K.q[1] = H->q[1]);
		S.q[2] ^= (K.q[2] = H->q[2]);
		S.q[3] ^= (K.q[3] = H->q[3]);
		S.q[4] ^= (K.q[4] = H->q[4]);
		S.q[5] ^= (K.q[5] = H->q[5]);
		S.q[6] ^= (K.q[6] = H->q[6]);
		S.q[7] ^= (K.q[7] = H->q[7]);
		}
	else
#endif
		{
		const u64 *pa = (const u64*)p;
		S.q[0] = (K.q[0] = H->q[0]) ^ pa[0];
		S.q[1] = (K.q[1] = H->q[1]) ^ pa[1];
		S.q[2] = (K.q[2] = H->q[2]) ^ pa[2];
		S.q[3] = (K.q[3] = H->q[3]) ^ pa[3];
		S.q[4] = (K.q[4] = H->q[4]) ^ pa[4];
		S.q[5] = (K.q[5] = H->q[5]) ^ pa[5];
		S.q[6] = (K.q[6] = H->q[6]) ^ pa[6];
		S.q[7] = (K.q[7] = H->q[7]) ^ pa[7];
		}

	for(r=0;r<ROUNDS;r++)
		{
#ifdef SMALL_REGISTER_BANK
		L0 =	C0(K,0) ^ C1(K,7) ^ C2(K,6) ^ C3(K,5) ^
			C4(K,4) ^ C5(K,3) ^ C6(K,2) ^ C7(K,1) ^ RC[r];
		L1 =	C0(K,1) ^ C1(K,0) ^ C2(K,7) ^ C3(K,6) ^
			C4(K,5) ^ C5(K,4) ^ C6(K,3) ^ C7(K,2);
		L2 =	C0(K,2) ^ C1(K,1) ^ C2(K,0) ^ C3(K,7) ^
			C4(K,6) ^ C5(K,5) ^ C6(K,4) ^ C7(K,3);
		L3 =	C0(K,3) ^ C1(K,2) ^ C2(K,1) ^ C3(K,0) ^
			C4(K,7) ^ C5(K,6) ^ C6(K,5) ^ C7(K,4);
		L4 =	C0(K,4) ^ C1(K,3) ^ C2(K,2) ^ C3(K,1) ^
			C4(K,0) ^ C5(K,7) ^ C6(K,6) ^ C7(K,5);
		L5 =	C0(K,5) ^ C1(K,4) ^ C2(K,3) ^ C3(K,2) ^
			C4(K,1) ^ C5(K,0) ^ C6(K,7) ^ C7(K,6);
		L6 =	C0(K,6) ^ C1(K,5) ^ C2(K,4) ^ C3(K,3) ^
			C4(K,2) ^ C5(K,1) ^ C6(K,0) ^ C7(K,7);
		L7 =	C0(K,7) ^ C1(K,6) ^ C2(K,5) ^ C3(K,4) ^
			C4(K,3) ^ C5(K,2) ^ C6(K,1) ^ C7(K,0);

		K.q[0] = L0; K.q[1] = L1; K.q[2] = L2; K.q[3] = L3;
		K.q[4] = L4; K.q[5] = L5; K.q[6] = L6; K.q[7] = L7;

		L0 ^=	C0(S,0) ^ C1(S,7) ^ C2(S,6) ^ C3(S,5) ^
			C4(S,4) ^ C5(S,3) ^ C6(S,2) ^ C7(S,1);
		L1 ^=	C0(S,1) ^ C1(S,0) ^ C2(S,7) ^ C3(S,6) ^
			C4(S,5) ^ C5(S,4) ^ C6(S,3) ^ C7(S,2);
		L2 ^=	C0(S,2) ^ C1(S,1) ^ C2(S,0) ^ C3(S,7) ^
			C4(S,6) ^ C5(S,5) ^ C6(S,4) ^ C7(S,3);
		L3 ^=	C0(S,3) ^ C1(S,2) ^ C2(S,1) ^ C3(S,0) ^
			C4(S,7) ^ C5(S,6) ^ C6(S,5) ^ C7(S,4);
		L4 ^=	C0(S,4) ^ C1(S,3) ^ C2(S,2) ^ C3(S,1) ^
			C4(S,0) ^ C5(S,7) ^ C6(S,6) ^ C7(S,5);
		L5 ^=	C0(S,5) ^ C1(S,4) ^ C2(S,3) ^ C3(S,2) ^
			C4(S,1) ^ C5(S,0) ^ C6(S,7) ^ C7(S,6);
		L6 ^=	C0(S,6) ^ C1(S,5) ^ C2(S,4) ^ C3(S,3) ^
			C4(S,2) ^ C5(S,1) ^ C6(S,0) ^ C7(S,7);
		L7 ^=	C0(S,7) ^ C1(S,6) ^ C2(S,5) ^ C3(S,4) ^
			C4(S,3) ^ C5(S,2) ^ C6(S,1) ^ C7(S,0);

		S.q[0] = L0; S.q[1] = L1; S.q[2] = L2; S.q[3] = L3;
		S.q[4] = L4; S.q[5] = L5; S.q[6] = L6; S.q[7] = L7;
#else
		L0  = C0(K,0); L1  = C1(K,0); L2  = C2(K,0); L3  = C3(K,0);
		L4  = C4(K,0); L5  = C5(K,0); L6  = C6(K,0); L7  = C7(K,0);
		L0 ^= RC[r];

		L1 ^= C0(K,1); L2 ^= C1(K,1); L3 ^= C2(K,1); L4 ^= C3(K,1);
		L5 ^= C4(K,1); L6 ^= C5(K,1); L7 ^= C6(K,1); L0 ^= C7(K,1);

		L2 ^= C0(K,2); L3 ^= C1(K,2); L4 ^= C2(K,2); L5 ^= C3(K,2);
		L6 ^= C4(K,2); L7 ^= C5(K,2); L0 ^= C6(K,2); L1 ^= C7(K,2);

		L3 ^= C0(K,3); L4 ^= C1(K,3); L5 ^= C2(K,3); L6 ^= C3(K,3);
		L7 ^= C4(K,3); L0 ^= C5(K,3); L1 ^= C6(K,3); L2 ^= C7(K,3);

		L4 ^= C0(K,4); L5 ^= C1(K,4); L6 ^= C2(K,4); L7 ^= C3(K,4);
		L0 ^= C4(K,4); L1 ^= C5(K,4); L2 ^= C6(K,4); L3 ^= C7(K,4);

		L5 ^= C0(K,5); L6 ^= C1(K,5); L7 ^= C2(K,5); L0 ^= C3(K,5);
		L1 ^= C4(K,5); L2 ^= C5(K,5); L3 ^= C6(K,5); L4 ^= C7(K,5);

		L6 ^= C0(K,6); L7 ^= C1(K,6); L0 ^= C2(K,6); L1 ^= C3(K,6);
		L2 ^= C4(K,6); L3 ^= C5(K,6); L4 ^= C6(K,6); L5 ^= C7(K,6);

		L7 ^= C0(K,7); L0 ^= C1(K,7); L1 ^= C2(K,7); L2 ^= C3(K,7);
		L3 ^= C4(K,7); L4 ^= C5(K,7); L5 ^= C6(K,7); L6 ^= C7(K,7);

		K.q[0] = L0; K.q[1] = L1; K.q[2] = L2; K.q[3] = L3;
		K.q[4] = L4; K.q[5] = L5; K.q[6] = L6; K.q[7] = L7;

		L0 ^= C0(S,0); L1 ^= C1(S,0); L2 ^= C2(S,0); L3 ^= C3(S,0);
		L4 ^= C4(S,0); L5 ^= C5(S,0); L6 ^= C6(S,0); L7 ^= C7(S,0);

		L1 ^= C0(S,1); L2 ^= C1(S,1); L3 ^= C2(S,1); L4 ^= C3(S,1);
		L5 ^= C4(S,1); L6 ^= C5(S,1); L7 ^= C6(S,1); L0 ^= C7(S,1);

		L2 ^= C0(S,2); L3 ^= C1(S,2); L4 ^= C2(S,2); L5 ^= C3(S,2);
		L6 ^= C4(S,2); L7 ^= C5(S,2); L0 ^= C6(S,2); L1 ^= C7(S,2);

		L3 ^= C0(S,3); L4 ^= C1(S,3); L5 ^= C2(S,3); L6 ^= C3(S,3);
		L7 ^= C4(S,3); L0 ^= C5(S,3); L1 ^= C6(S,3); L2 ^= C7(S,3);

		L4 ^= C0(S,4); L5 ^= C1(S,4); L6 ^= C2(S,4); L7 ^= C3(S,4);
		L0 ^= C4(S,4); L1 ^= C5(S,4); L2 ^= C6(S,4); L3 ^= C7(S,4);

		L5 ^= C0(S,5); L6 ^= C1(S,5); L7 ^= C2(S,5); L0 ^= C3(S,5);
		L1 ^= C4(S,5); L2 ^= C5(S,5); L3 ^= C6(S,5); L4 ^= C7(S,5);

		L6 ^= C0(S,6); L7 ^= C1(S,6); L0 ^= C2(S,6); L1 ^= C3(S,6);
		L2 ^= C4(S,6); L3 ^= C5(S,6); L4 ^= C6(S,6); L5 ^= C7(S,6);

		L7 ^= C0(S,7); L0 ^= C1(S,7); L1 ^= C2(S,7); L2 ^= C3(S,7);
		L3 ^= C4(S,7); L4 ^= C5(S,7); L5 ^= C6(S,7); L6 ^= C7(S,7);

		S.q[0] = L0; S.q[1] = L1; S.q[2] = L2; S.q[3] = L3;
		S.q[4] = L4; S.q[5] = L5; S.q[6] = L6; S.q[7] = L7;
#endif
		}

#ifdef STRICT_ALIGNMENT
	if ((size_t)p & 7)
		{
		int i;
		for(i=0;i<64;i++)	H->c[i] ^= S.c[i] ^ p[i];
		}
	else
#endif
		{
		const u64 *pa=(const u64 *)p;
		H->q[0] ^= S.q[0] ^ pa[0];
		H->q[1] ^= S.q[1] ^ pa[1];
		H->q[2] ^= S.q[2] ^ pa[2];
		H->q[3] ^= S.q[3] ^ pa[3];
		H->q[4] ^= S.q[4] ^ pa[4];
		H->q[5] ^= S.q[5] ^ pa[5];
		H->q[6] ^= S.q[6] ^ pa[6];
		H->q[7] ^= S.q[7] ^ pa[7];
		}
#endif
							p += 64;
							} while(--n);
	}
