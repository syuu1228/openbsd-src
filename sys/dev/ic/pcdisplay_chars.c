/* $OpenBSD: src/sys/dev/ic/pcdisplay_chars.c,v 1.4 2004/04/02 04:39:50 deraadt Exp $ */
/* $NetBSD: pcdisplay_chars.c,v 1.5 2000/06/08 07:01:19 cgd Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>

#include <dev/wscons/unicode.h>

#define CONTROL 1 /* XXX smiley */
#define NOTPRINTABLE 4 /* diamond XXX watch out - not in ISO part! */

static u_char isomappings[128] = {
	CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL,
	CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL,
	CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL,
	CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL, CONTROL,
	0xff, /* 0x00a0 NO-BREAK SPACE */
	0xad, /* 0x00a1 INVERTED EXCLAMATION MARK */
	0x9b, /* 0x00a2 CENT SIGN */
	0x9c, /* 0x00a3 POUND SIGN */
	NOTPRINTABLE, /* 0x00a4 CURRENCY SIGN */
	0x9d, /* 0x00a5 YEN SIGN */
	0x7c, /* 0x00a6 BROKEN BAR */
	0x15, /* 0x00a7 SECTION SIGN */
	NOTPRINTABLE, /* 0x00a8 DIAERESIS */
	NOTPRINTABLE, /* 0x00a9 COPYRIGHT SIGN */
	0xa6, /* 0x00aa FEMININE ORDINAL INDICATOR */
	0xae, /* 0x00ab LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
	0xaa, /* 0x00ac NOT SIGN */
	0xc4, /* 0x00ad SOFT HYPHEN */
	NOTPRINTABLE, /* 0x00ae REGISTERED SIGN */
	NOTPRINTABLE, /* 0x00af MACRON */
	0xf8, /* 0x00b0 DEGREE SIGN */
	0xf1, /* 0x00b1 PLUS-MINUS SIGN */
	0xfd, /* 0x00b2 SUPERSCRIPT TWO */
	NOTPRINTABLE, /* 0x00b3 SUPERSCRIPT THREE */
	0x27, /* 0x00b4 ACUTE ACCENT */
	0xe6, /* 0x00b5 MICRO SIGN */
	0x14, /* 0x00b6 PILCROW SIGN */
	0xfa, /* 0x00b7 MIDDLE DOT */
	NOTPRINTABLE, /* 0x00b8 CEDILLA */
	NOTPRINTABLE, /* 0x00b9 SUPERSCRIPT ONE */
	0xa7, /* 0x00ba MASCULINE ORDINAL INDICATOR */
	0xaf, /* 0x00bb RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
	0xac, /* 0x00bc VULGAR FRACTION ONE QUARTER */
	0xab, /* 0x00bd VULGAR FRACTION ONE HALF */
	NOTPRINTABLE, /* 0x00be VULGAR FRACTION THREE QUARTERS */
	0xa8, /* 0x00bf INVERTED QUESTION MARK */
	NOTPRINTABLE, /* 0x00c0 LATIN CAPITAL LETTER A WITH GRAVE */
	NOTPRINTABLE, /* 0x00c1 LATIN CAPITAL LETTER A WITH ACUTE */
	NOTPRINTABLE, /* 0x00c2 LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
	NOTPRINTABLE, /* 0x00c3 LATIN CAPITAL LETTER A WITH TILDE */
	0x8e, /* 0x00c4 LATIN CAPITAL LETTER A WITH DIAERESIS */
	0x8f, /* 0x00c5 LATIN CAPITAL LETTER A WITH RING ABOVE */
	0x92, /* 0x00c6 LATIN CAPITAL LIGATURE AE */
	0x80, /* 0x00c7 LATIN CAPITAL LETTER C WITH CEDILLA */
	NOTPRINTABLE, /* 0x00c8 LATIN CAPITAL LETTER E WITH GRAVE */
	0x90, /* 0x00c9 LATIN CAPITAL LETTER E WITH ACUTE */
	NOTPRINTABLE, /* 0x00ca LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
	NOTPRINTABLE, /* 0x00cb LATIN CAPITAL LETTER E WITH DIAERESIS */
	NOTPRINTABLE, /* 0x00cc LATIN CAPITAL LETTER I WITH GRAVE */
	NOTPRINTABLE, /* 0x00cd LATIN CAPITAL LETTER I WITH ACUTE */
	NOTPRINTABLE, /* 0x00ce LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
	NOTPRINTABLE, /* 0x00cf LATIN CAPITAL LETTER I WITH DIAERESIS */
	NOTPRINTABLE, /* 0x00d0 LATIN CAPITAL LETTER ETH */
	0xa5, /* 0x00d1 LATIN CAPITAL LETTER N WITH TILDE */
	NOTPRINTABLE, /* 0x00d2 LATIN CAPITAL LETTER O WITH GRAVE */
	NOTPRINTABLE, /* 0x00d3 LATIN CAPITAL LETTER O WITH ACUTE */
	NOTPRINTABLE, /* 0x00d4 LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
	NOTPRINTABLE, /* 0x00d5 LATIN CAPITAL LETTER O WITH TILDE */
	0x99, /* 0x00d6 LATIN CAPITAL LETTER O WITH DIAERESIS */
	NOTPRINTABLE, /* 0x00d7 MULTIPLICATION SIGN */
	NOTPRINTABLE, /* 0x00d8 LATIN CAPITAL LETTER O WITH STROKE */
	NOTPRINTABLE, /* 0x00d9 LATIN CAPITAL LETTER U WITH GRAVE */
	NOTPRINTABLE, /* 0x00da LATIN CAPITAL LETTER U WITH ACUTE */
	NOTPRINTABLE, /* 0x00db LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
	0x9a, /* 0x00dc LATIN CAPITAL LETTER U WITH DIAERESIS */
	NOTPRINTABLE, /* 0x00dd LATIN CAPITAL LETTER Y WITH ACUTE */
	NOTPRINTABLE, /* 0x00de LATIN CAPITAL LETTER THORN */
	0xe1, /* 0x00df LATIN SMALL LETTER SHARP S */
	0x85, /* 0x00e0 LATIN SMALL LETTER A WITH GRAVE */
	0xa0, /* 0x00e1 LATIN SMALL LETTER A WITH ACUTE */
	0x83, /* 0x00e2 LATIN SMALL LETTER A WITH CIRCUMFLEX */
	NOTPRINTABLE, /* 0x00e3 LATIN SMALL LETTER A WITH TILDE */
	0x84, /* 0x00e4 LATIN SMALL LETTER A WITH DIAERESIS */
	0x86, /* 0x00e5 LATIN SMALL LETTER A WITH RING ABOVE */
	0x91, /* 0x00e6 LATIN SMALL LIGATURE AE */
	0x87, /* 0x00e7 LATIN SMALL LETTER C WITH CEDILLA */
	0x8a, /* 0x00e8 LATIN SMALL LETTER E WITH GRAVE */
	0x82, /* 0x00e9 LATIN SMALL LETTER E WITH ACUTE */
	0x88, /* 0x00ea LATIN SMALL LETTER E WITH CIRCUMFLEX */
	0x89, /* 0x00eb LATIN SMALL LETTER E WITH DIAERESIS */
	0x8d, /* 0x00ec LATIN SMALL LETTER I WITH GRAVE */
	0xa1, /* 0x00ed LATIN SMALL LETTER I WITH ACUTE */
	0x8c, /* 0x00ee LATIN SMALL LETTER I WITH CIRCUMFLEX */
	0x8b, /* 0x00ef LATIN SMALL LETTER I WITH DIAERESIS */
	NOTPRINTABLE, /* 0x00f0 LATIN SMALL LETTER ETH */
	0xa4, /* 0x00f1 LATIN SMALL LETTER N WITH TILDE */
	0x95, /* 0x00f2 LATIN SMALL LETTER O WITH GRAVE */
	0xa2, /* 0x00f3 LATIN SMALL LETTER O WITH ACUTE */
	0x93, /* 0x00f4 LATIN SMALL LETTER O WITH CIRCUMFLEX */
	NOTPRINTABLE, /* 0x00f5 LATIN SMALL LETTER O WITH TILDE */
	0x94, /* 0x00f6 LATIN SMALL LETTER O WITH DIAERESIS */
	0xf6, /* 0x00f7 DIVISION SIGN */
	NOTPRINTABLE, /* 0x00f8 LATIN SMALL LETTER O WITH STROKE */
	0x97, /* 0x00f9 LATIN SMALL LETTER U WITH GRAVE */
	0xa3, /* 0x00fa LATIN SMALL LETTER U WITH ACUTE */
	0x96, /* 0x00fb LATIN SMALL LETTER U WITH CIRCUMFLEX */
	0x81, /* 0x00fc LATIN SMALL LETTER U WITH DIAERESIS */
	NOTPRINTABLE, /* 0x00fd LATIN SMALL LETTER Y WITH ACUTE */
	NOTPRINTABLE, /* 0x00fe LATIN SMALL LETTER THORN */
	0x98, /* 0x00ff LATIN SMALL LETTER Y WITH DIAERESIS */
};

static struct {
	u_int16_t uni;
	u_char ibm;
} unimappings[] = {
	{0x0192, 0x9f}, /* LATIN SMALL LETTER F WITH HOOK */
	{0x0393, 0xe2}, /* GREEK CAPITAL LETTER GAMMA */
	{0x0398, 0xe9}, /* GREEK CAPITAL LETTER THETA */
	{0x03a3, 0xe4}, /* GREEK CAPITAL LETTER SIGMA */
	{0x03a6, 0xe8}, /* GREEK CAPITAL LETTER PHI */
	{0x03a9, 0xea}, /* GREEK CAPITAL LETTER OMEGA */
	{0x03b1, 0xe0}, /* GREEK SMALL LETTER ALPHA */
	{0x03b2, 0xe1}, /* GREEK SMALL LETTER BETA */
	{0x03b4, 0xeb}, /* GREEK SMALL LETTER DELTA */
	{0x03b5, 0xee}, /* GREEK SMALL LETTER EPSILON */
	{0x03c0, 0xe3}, /* GREEK SMALL LETTER PI */
	{0x03c3, 0xe5}, /* GREEK SMALL LETTER SIGMA */
	{0x03c4, 0xe7}, /* GREEK SMALL LETTER TAU */
	{0x03c6, 0xed}, /* GREEK SMALL LETTER PHI */
	{0x2022, 0x07}, /* BULLET */
	{0x203c, 0x13}, /* DOUBLE EXCLAMATION MARK */
	{0x207f, 0xfc}, /* SUPERSCRIPT LATIN SMALL LETTER N */
	{0x20a7, 0x9e}, /* PESETA SIGN */
	{0x2190, 0x1b}, /* LEFTWARDS ARROW */
	{0x2191, 0x18}, /* UPWARDS ARROW */
	{0x2192, 0x1a}, /* RIGHTWARDS ARROW */
	{0x2193, 0x19}, /* DOWNWARDS ARROW */
	{0x2194, 0x1d}, /* LEFT RIGHT ARROW */
	{0x2195, 0x12}, /* UP DOWN ARROW */
	{0x21a8, 0x17}, /* UP DOWN ARROW WITH BASE */
	{0x2212, 0x2d}, /* MINUS SIGN XXX move to more general place */
	{0x2215, 0x2f}, /* DIVISION SLASH XXX move to more general place */
	{0x2219, 0xf9}, /* BULLET OPERATOR */
	{0x221a, 0xfb}, /* SQUARE ROOT */
	{0x221e, 0xec}, /* INFINITY */
	{0x2229, 0xef}, /* INTERSECTION */
	{0x2248, 0xf7}, /* ALMOST EQUAL TO */
	{0x2261, 0xf0}, /* IDENTICAL TO */
	{0x2264, 0xf3}, /* LESS-THAN OR EQUAL TO */
	{0x2265, 0xf2}, /* GREATER-THAN OR EQUAL TO */
	{0x2302, 0x7f}, /* HOUSE */
	{0x2310, 0xa9}, /* REVERSED NOT SIGN */
	{0x2320, 0xf4}, /* TOP HALF INTEGRAL */
	{0x2321, 0xf5}, /* BOTTOM HALF INTEGRAL */
	{0x2500, 0xc4}, /* BOX DRAWINGS LIGHT HORIZONTAL */
	{0x2502, 0xb3}, /* BOX DRAWINGS LIGHT VERTICAL */
	{0x250c, 0xda}, /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
	{0x2510, 0xbf}, /* BOX DRAWINGS LIGHT DOWN AND LEFT */
	{0x2514, 0xc0}, /* BOX DRAWINGS LIGHT UP AND RIGHT */
	{0x2518, 0xd9}, /* BOX DRAWINGS LIGHT UP AND LEFT */
	{0x251c, 0xc3}, /* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
	{0x2524, 0xb4}, /* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
	{0x252c, 0xc2}, /* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
	{0x2534, 0xc1}, /* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
	{0x253c, 0xc5}, /* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
	{0x2550, 0xcd}, /* BOX DRAWINGS DOUBLE HORIZONTAL */
	{0x2551, 0xba}, /* BOX DRAWINGS DOUBLE VERTICAL */
	{0x2552, 0xd5}, /* BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE */
	{0x2553, 0xd6}, /* BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE */
	{0x2554, 0xc9}, /* BOX DRAWINGS DOUBLE DOWN AND RIGHT */
	{0x2555, 0xb8}, /* BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE */
	{0x2556, 0xb7}, /* BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE */
	{0x2557, 0xbb}, /* BOW DRAWINGS DOUBLE DOWN AND LEFT */
	{0x2558, 0xd4}, /* BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE */
	{0x2559, 0xd3}, /* BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE */
	{0x255a, 0xc8}, /* BOX DRAWINGS DOUBLE UP AND RIGHT */
	{0x255b, 0xbe}, /* BOX DRAWINGS UP SINGLE AND LEFT DOUBLE */
	{0x255c, 0xbd}, /* BOX DRAWINGS UP DOUBLE AND LEFT SINGLE */
	{0x255d, 0xbc}, /* BOX DRAWINGS DOUBLE UP AND LEFT */
	{0x255e, 0xc6}, /* BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE */
	{0x255f, 0xc7}, /* BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE */
	{0x2560, 0xcc}, /* BOX DRAWINGS DOUBLE VERTICAL AND RIGHT */
	{0x2561, 0xb4}, /* BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE */
	{0x2562, 0xb5}, /* BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE */
	{0x2563, 0xb9}, /* BOX DRAWINGS DOUBLE VERTICAL AND LEFT */
	{0x2564, 0xd1}, /* BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE */
	{0x2565, 0xd2}, /* BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE */
	{0x2566, 0xcb}, /* BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL */
	{0x2567, 0xcf}, /* BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE */
	{0x2568, 0xd0}, /* BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE */
	{0x2569, 0xca}, /* BOX DRAWINGS DOUBLE UP AND HORIZONTAL */
	{0x256a, 0xd8}, /* BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE */
	{0x256b, 0xd7}, /* BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE */
	{0x256c, 0xce}, /* BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL */
	{0x2580, 0xdf}, /* UPPER HALF BLOCK */
	{0x2584, 0xdc}, /* LOWER HALF BLOCK */
	{0x2588, 0xdb}, /* FULL BLOCK */
	{0x258c, 0xdd}, /* LEFT HALF BLOCK */
	{0x2590, 0xde}, /* RIGHT HALF BLOCK */
	{0x2591, 0xb0}, /* LIGHT SHADE */
	{0x2592, 0xb1}, /* MEDIUM SHADE */
	{0x2593, 0xb2}, /* DARK SHADE */
	{0x25a0, 0xfe}, /* BLACK SQUARE */
	{0x25ac, 0x16}, /* BLACK RECTANGLE */
	{0x25b2, 0x1e}, /* BLACK UP-POINTING TRIANGLE */
	{0x25ba, 0x10}, /* BLACK RIGHT-POINTING POINTER */
	{0x25bc, 0x1f}, /* BLACK DOWN-POINTING TRIANGLE */
	{0x25c4, 0x11}, /* BLACK LEFT-POINTING POINTER */
	{0x25c6, 0x04}, /* BLACK DIAMOND */
	{0x25cb, 0x09}, /* WHITE CIRCLE */
	{0x25d8, 0x08}, /* INVERSE BULLET */
	{0x25d9, 0x0a}, /* INVERSE WHITE CIRCLE */
	{0x263a, 0x01}, /* WHITE SMILING FACE */
	{0x263b, 0x02}, /* BLACK SMILING FACE */
	{0x263c, 0x0f}, /* WHITE SUN WITH RAYS */
	{0x2640, 0x0c}, /* FEMALE SIGN */
	{0x2642, 0x0b}, /* MALE SIGN */
	{0x2660, 0x06}, /* BLACK SPADE SUIT */
	{0x2663, 0x05}, /* BLACK CLUB SUIT */
	{0x2665, 0x03}, /* BLACK HEART SUIT */
	{0x2666, 0x04}, /* BLACK DIAMOND SUIT */
	{0x266a, 0x0d}, /* EIGHTH NOTE */
	{0x266b, 0x0e}, /* BEAMED EIGHTH NOTES */
};

static struct {
	u_int16_t uni;
	u_char ibm;
	int quality;
} replacements[] = {
	{0x00af, 0x2d, 3}, /* MACRON -> - */
	{0x221f, 0xc0, 3}, /* RIGHT ANGLE -> light up and right */
	{0x222a, 0x55, 3}, /* UNION -> U */
	{0x223c, 0x7e, 3}, /* TILDE OPERATOR -> ~ */
	{0x2308, 0xda, 3}, /* LEFT CEILING -> light down and right */
	{0x2309, 0xbf, 3}, /* RIGHT CEILING -> light down and left */
	{0x230a, 0xc0, 3}, /* LEFT FLOOR -> light up and right */
	{0x230b, 0xd9, 3}, /* RIGHT FLOOR -> light up and left */
	{0x2329, 0x3c, 3}, /* LEFT-POINTING ANGLE BRACKET -> < */
	{0x232a, 0x3e, 3}, /* RIGHT-POINTING ANGLE BRACKET -> > */
	{_e003U, 0x2d, 3}, /* scan 5 -> - */
	{_e005U, 0x5f, 3}, /* scan 9 -> _ */
	{_e00bU, 0x7b, 3}, /* braceleftmid -> { */
	{_e00cU, 0x7d, 3}, /* bracerightmid -> } */
	{_e00fU, 0xd9, 3}, /* mirrored not sign? -> light up and left */
	{0x00d7, 0x78, 2}, /* MULTIPLICATION SIGN -> x */
	{0x00d8, 0xe9, 2}, /* LATIN CAPITAL LETTER O WITH STROKE -> Theta */
	{0x00f8, 0xed, 2}, /* LATIN SMALL LETTER O WITH STROKE -> phi */
	{0x03a0, 0xe3, 2}, /* GREEK CAPITAL LETTER PI -> pi */
	{0x03a5, 0x59, 2}, /* GREEK CAPITAL LETTER UPSILON -> Y */
	{0x03b3, 0x59, 2}, /* GREEK SMALL LETTER GAMMA -> Y */
	{0x03b8, 0xe9, 2}, /* GREEK SMALL LETTER THETA -> Theta */
	{0x03bd, 0x76, 2}, /* GREEK SMALL LETTER NU -> v */
	{0x03c9, 0x77, 2}, /* GREEK SMALL LETTER OMEGA -> w */
	{0x20ac, 0x45, 2}, /* EURO SIGN -> E */
	{_e002U, 0x2d, 2}, /* scan 3 -> - */
	{_e004U, 0x2d, 2}, /* scan 7 -> - */
	{_e007U, 0xda, 2}, /* bracelefttp -> light down and right */
	{_e008U, 0xc0, 2}, /* braceleftbt -> light up and right */
	{_e009U, 0xbf, 2}, /* bracerighttp -> light down and left */
	{_e00aU, 0xd9, 2}, /* bracerighrbt -> light up and left */
	{_e00dU, 0x3c, 2}, /* inverted angle? -> < */
	{_e00eU, 0x3c, 2}, /* angle? -> < */
	{_e00fU, 0xd9, 2}, /* mirrored not sign? -> light up and left */
	{0x00a9, 0x63, 1}, /* COPYRIGHT SIGN -> c */
	{0x00ae, 0x72, 1}, /* REGISTERED SIGN -> r */
	{0x00b3, 0x33, 1}, /* SUPERSCRIPT THREE -> 3 */
	{0x00b9, 0x39, 1}, /* SUPERSCRIPT ONE -> 1 */
	{0x00c0, 0x41, 1}, /* LATIN CAPITAL LETTER A WITH GRAVE -> A */
	{0x00c1, 0x41, 1}, /* LATIN CAPITAL LETTER A WITH ACUTE -> A */
	{0x00c2, 0x41, 1}, /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX -> A */
	{0x00c3, 0x41, 1}, /* LATIN CAPITAL LETTER A WITH TILDE -> A */
	{0x00c8, 0x45, 1}, /* LATIN CAPITAL LETTER E WITH GRAVE -> E */
	{0x00ca, 0x45, 1}, /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX -> E */
	{0x00cb, 0x45, 1}, /* LATIN CAPITAL LETTER E WITH DIAERESIS -> E */
	{0x00cc, 0x49, 1}, /* LATIN CAPITAL LETTER I WITH GRAVE -> I */
	{0x00cd, 0x49, 1}, /* LATIN CAPITAL LETTER I WITH ACUTE -> I */
	{0x00ce, 0x49, 1}, /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX -> I */
	{0x00cf, 0x49, 1}, /* LATIN CAPITAL LETTER I WITH DIAERESIS -> I */
	{0x00d0, 0x44, 1}, /* LATIN CAPITAL LETTER ETH -> D */
	{0x00d2, 0x4f, 1}, /* LATIN CAPITAL LETTER O WITH GRAVE -> O */
	{0x00d3, 0x4f, 1}, /* LATIN CAPITAL LETTER O WITH ACUTE -> O */
	{0x00d4, 0x4f, 1}, /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX -> O */
	{0x00d5, 0x4f, 1}, /* LATIN CAPITAL LETTER O WITH TILDE -> O */
	{0x00d9, 0x55, 1}, /* LATIN CAPITAL LETTER U WITH GRAVE -> U */
	{0x00da, 0x55, 1}, /* LATIN CAPITAL LETTER U WITH ACUTE -> U */
	{0x00db, 0x55, 1}, /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX -> U */
	{0x00dd, 0x59, 1}, /* LATIN CAPITAL LETTER Y WITH ACUTE -> Y */
	{0x00e3, 0x61, 1}, /* LATIN SMALL LETTER A WITH TILDE -> a */
	{0x00f5, 0x6f, 1}, /* LATIN SMALL LETTER O WITH TILDE -> o */
	{0x00fd, 0x79, 1}, /* LATIN SMALL LETTER Y WITH ACUTE -> y */
};

int
pcdisplay_mapchar(id, uni, index)
	void *id;
	int uni;
	unsigned int *index;
{
	u_int i;

	if (uni < 128) {
		*index = uni;
		return (5);
	} else if ((uni < 256) && (isomappings[uni - 128] != NOTPRINTABLE)) {
		*index = isomappings[uni - 128];
		return (5);
	}

	for (i = 0; i < sizeof(unimappings) / sizeof(unimappings[0]); i++)
		if (uni == unimappings[i].uni) {
			*index = unimappings[i].ibm;
			return (5);
		}

	for (i = 0; i < sizeof(replacements) / sizeof(replacements[0]); i++)
		if (uni == replacements[i].uni) {
			*index = replacements[i].ibm;
			return (replacements[i].quality);
		}

	*index = NOTPRINTABLE;
	return (0);
}
