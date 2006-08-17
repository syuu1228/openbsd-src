/*	$OpenBSD: src/sys/dev/microcode/rum/microcode.h,v 1.1 2006/08/17 08:32:30 damien Exp $	*/

/*-
 * Copyright (c) 2005-2006, Ralink Technology, Corp.
 *	Paul Lin <paul_lin@ralinktech.com.tw>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file contains the loadable 8051 microcode for the Ralink RT2573
 * chipset.
 */

static const uint8_t rt2573[] = {
	0x02, 0x13, 0x25, 0x12, 0x10, 0xd9, 0x02, 0x12, 0x58, 0x02, 0x13,
	0x58, 0x02, 0x13, 0x5a, 0xc0, 0xd0, 0x75, 0xd0, 0x18, 0x12, 0x13,
	0x5c, 0xd0, 0xd0, 0x22, 0x02, 0x14, 0x5c, 0x02, 0x14, 0xe7, 0xed,
	0x4c, 0x70, 0x44, 0x90, 0x01, 0xa8, 0x74, 0x80, 0xf0, 0xef, 0x30,
	0xe5, 0x07, 0xe4, 0x90, 0x00, 0x0f, 0xf0, 0x80, 0x2c, 0xe5, 0x40,
	0x24, 0xc0, 0x60, 0x13, 0x24, 0xc0, 0x60, 0x16, 0x24, 0xc0, 0x60,
	0x19, 0x24, 0xc0, 0x70, 0x1a, 0xe4, 0x90, 0x00, 0x0b, 0xf0, 0x80,
	0x13, 0xe4, 0x90, 0x00, 0x13, 0xf0, 0x80, 0x0c, 0xe4, 0x90, 0x00,
	0x1b, 0xf0, 0x80, 0x05, 0xe4, 0x90, 0x00, 0x23, 0xf0, 0xe4, 0x90,
	0x01, 0xa8, 0xf0, 0xd3, 0x22, 0x90, 0x02, 0x02, 0xed, 0xf0, 0x90,
	0x02, 0x01, 0xef, 0xf0, 0xd3, 0x22, 0xef, 0x24, 0xc0, 0x60, 0x1f,
	0x24, 0xc0, 0x60, 0x2e, 0x24, 0xc0, 0x60, 0x3d, 0x24, 0xc0, 0x70,
	0x53, 0x90, 0x00, 0x0b, 0xe0, 0x30, 0xe1, 0x02, 0xc3, 0x22, 0x90,
	0x00, 0x09, 0xe0, 0xfe, 0x90, 0x00, 0x08, 0x80, 0x37, 0x90, 0x00,
	0x13, 0xe0, 0x30, 0xe1, 0x02, 0xc3, 0x22, 0x90, 0x00, 0x11, 0xe0,
	0xfe, 0x90, 0x00, 0x10, 0x80, 0x24, 0x90, 0x00, 0x1b, 0xe0, 0x30,
	0xe1, 0x02, 0xc3, 0x22, 0x90, 0x00, 0x19, 0xe0, 0xfe, 0x90, 0x00,
	0x18, 0x80, 0x11, 0x90, 0x00, 0x23, 0xe0, 0x30, 0xe1, 0x02, 0xc3,
	0x22, 0x90, 0x00, 0x21, 0xe0, 0xfe, 0x90, 0x00, 0x20, 0xe0, 0xfd,
	0xee, 0xf5, 0x37, 0xed, 0xf5, 0x38, 0xd3, 0x22, 0x30, 0x09, 0x20,
	0x20, 0x04, 0x0b, 0x90, 0x02, 0x08, 0xe0, 0x54, 0x0f, 0x70, 0x03,
	0x02, 0x12, 0x57, 0xc2, 0x09, 0x90, 0x02, 0x00, 0xe0, 0x44, 0x04,
	0xf0, 0x74, 0x04, 0x12, 0x0c, 0x3a, 0xc2, 0x04, 0xc2, 0x07, 0x90,
	0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80, 0xf6, 0x90, 0x03,
	0x26, 0xe0, 0x20, 0xe2, 0x03, 0x02, 0x12, 0x57, 0x90, 0x02, 0x08,
	0xe0, 0x70, 0x1b, 0x20, 0x07, 0x03, 0x02, 0x12, 0x57, 0x90, 0x03,
	0x12, 0xe0, 0x64, 0x22, 0x60, 0x03, 0x02, 0x12, 0x57, 0xd2, 0x09,
	0xc2, 0x07, 0x74, 0x02, 0x12, 0x0c, 0x3a, 0x22, 0x90, 0x02, 0x03,
	0xe0, 0x30, 0xe4, 0x47, 0x20, 0x06, 0x44, 0xe5, 0x3c, 0x60, 0x34,
	0xe5, 0x40, 0x24, 0xc0, 0x60, 0x14, 0x24, 0xc0, 0x60, 0x18, 0x24,
	0xc0, 0x60, 0x1c, 0x24, 0xc0, 0x70, 0x22, 0x90, 0x00, 0x0b, 0xe0,
	0x30, 0xe1, 0x1b, 0x22, 0x90, 0x00, 0x13, 0xe0, 0x30, 0xe1, 0x13,
	0x22, 0x90, 0x00, 0x1b, 0xe0, 0x30, 0xe1, 0x0b, 0x22, 0x90, 0x00,
	0x23, 0xe0, 0x30, 0xe1, 0x03, 0x02, 0x12, 0x57, 0x90, 0x02, 0x03,
	0x74, 0x01, 0xf0, 0x00, 0xe0, 0x54, 0xc0, 0xf5, 0x40, 0xe5, 0x40,
	0x24, 0xc0, 0x60, 0x20, 0x24, 0xc0, 0x60, 0x30, 0x24, 0xc0, 0x60,
	0x40, 0x24, 0xc0, 0x70, 0x56, 0x90, 0x00, 0x0b, 0xe0, 0x30, 0xe1,
	0x03, 0x02, 0x12, 0x57, 0x90, 0x00, 0x09, 0xe0, 0xfe, 0x90, 0x00,
	0x08, 0x80, 0x3a, 0x90, 0x00, 0x13, 0xe0, 0x30, 0xe1, 0x03, 0x02,
	0x12, 0x57, 0x90, 0x00, 0x11, 0xe0, 0xfe, 0x90, 0x00, 0x10, 0x80,
	0x26, 0x90, 0x00, 0x1b, 0xe0, 0x30, 0xe1, 0x03, 0x02, 0x12, 0x57,
	0x90, 0x00, 0x19, 0xe0, 0xfe, 0x90, 0x00, 0x18, 0x80, 0x12, 0x90,
	0x00, 0x23, 0xe0, 0x30, 0xe1, 0x03, 0x02, 0x12, 0x57, 0x90, 0x00,
	0x21, 0xe0, 0xfe, 0x90, 0x00, 0x20, 0xe0, 0xfd, 0xee, 0xf5, 0x37,
	0xed, 0xf5, 0x38, 0x90, 0x03, 0x27, 0x74, 0x82, 0xf0, 0x90, 0x02,
	0x01, 0xe5, 0x40, 0xf0, 0x90, 0x02, 0x06, 0xe0, 0xf5, 0x3c, 0xc3,
	0xe5, 0x38, 0x95, 0x3a, 0xe5, 0x37, 0x95, 0x39, 0x50, 0x21, 0xe5,
	0x40, 0x44, 0x05, 0xff, 0xe5, 0x37, 0xa2, 0xe7, 0x13, 0xfc, 0xe5,
	0x38, 0x13, 0xfd, 0x12, 0x10, 0x20, 0xe5, 0x3c, 0x30, 0xe2, 0x04,
	0xd2, 0x06, 0x80, 0x02, 0xc2, 0x06, 0x53, 0x3c, 0x01, 0x22, 0x30,
	0x0b, 0x07, 0xe4, 0x90, 0x02, 0x02, 0xf0, 0x80, 0x06, 0x90, 0x02,
	0x02, 0x74, 0x20, 0xf0, 0xe5, 0x40, 0x44, 0x01, 0x90, 0x02, 0x01,
	0xf0, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80, 0xf6,
	0x90, 0x03, 0x27, 0x74, 0x02, 0xf0, 0xaf, 0x40, 0x12, 0x10, 0x74,
	0x40, 0xa5, 0x00, 0x80, 0xf6, 0x22, 0x90, 0x7f, 0xf8, 0xe0, 0xb4,
	0x02, 0x03, 0x12, 0x16, 0x38, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0,
	0x03, 0x00, 0x80, 0xf6, 0x90, 0x03, 0x26, 0xe0, 0x20, 0xe1, 0x07,
	0xe5, 0x3b, 0x70, 0x03, 0x02, 0x13, 0x24, 0xe5, 0x3b, 0x70, 0x15,
	0x90, 0x03, 0x24, 0xe0, 0x75, 0xf0, 0x40, 0xa4, 0xf5, 0x36, 0x85,
	0xf0, 0x35, 0x75, 0x24, 0x83, 0x75, 0x3b, 0x01, 0x80, 0x03, 0x75,
	0x24, 0x03, 0xd3, 0xe5, 0x36, 0x95, 0x3a, 0xe5, 0x35, 0x95, 0x39,
	0x40, 0x36, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80,
	0xf6, 0x90, 0x03, 0x27, 0xe5, 0x24, 0xf0, 0x90, 0x00, 0x0f, 0xe0,
	0x30, 0xe1, 0x04, 0x30, 0x0e, 0xf6, 0x22, 0x30, 0x0b, 0x07, 0xe4,
	0x90, 0x02, 0x02, 0xf0, 0x80, 0x06, 0x90, 0x02, 0x02, 0x74, 0x20,
	0xf0, 0x90, 0x02, 0x01, 0x74, 0x21, 0xf0, 0x75, 0x24, 0x03, 0x80,
	0x3d, 0xe5, 0x35, 0xa2, 0xe7, 0x13, 0xfe, 0xe5, 0x36, 0x13, 0xfd,
	0xac, 0x06, 0x90, 0x02, 0x01, 0xe0, 0x30, 0xe0, 0x03, 0x00, 0x80,
	0xf6, 0x90, 0x03, 0x27, 0xe5, 0x24, 0xf0, 0x90, 0x00, 0x0f, 0xe0,
	0x30, 0xe1, 0x04, 0x30, 0x0e, 0xf6, 0x22, 0x7f, 0x25, 0x12, 0x10,
	0x20, 0xe5, 0x36, 0xb5, 0x3a, 0x08, 0xe5, 0x35, 0xb5, 0x39, 0x03,
	0x00, 0x80, 0x04, 0xe4, 0xf5, 0x3b, 0x22, 0xc3, 0xe5, 0x36, 0x95,
	0x3a, 0xf5, 0x36, 0xe5, 0x35, 0x95, 0x39, 0xf5, 0x35, 0x02, 0x12,
	0x96, 0x22, 0x75, 0xa8, 0x0f, 0x90, 0x03, 0x06, 0x74, 0x01, 0xf0,
	0x90, 0x03, 0x07, 0xf0, 0x90, 0x03, 0x08, 0x04, 0xf0, 0x90, 0x03,
	0x09, 0x74, 0x6c, 0xf0, 0x90, 0x03, 0x0a, 0x74, 0xff, 0xf0, 0x90,
	0x03, 0x02, 0x74, 0x1f, 0xf0, 0x90, 0x03, 0x00, 0x74, 0x04, 0xf0,
	0x90, 0x03, 0x25, 0x74, 0x31, 0xf0, 0xd2, 0xaf, 0x22, 0x00, 0x22,
	0x00, 0x22, 0x90, 0x03, 0x05, 0xe0, 0x30, 0xe0, 0x0b, 0xe0, 0x44,
	0x01, 0xf0, 0x30, 0x09, 0x02, 0xd2, 0x04, 0xc2, 0x07, 0x22, 0x8d,
	0x24, 0xa9, 0x07, 0x90, 0x7f, 0xfc, 0xe0, 0x75, 0x25, 0x00, 0xf5,
	0x26, 0xa3, 0xe0, 0x75, 0x27, 0x00, 0xf5, 0x28, 0xa3, 0xe0, 0xff,
	0xa3, 0xe0, 0xfd, 0xe9, 0x30, 0xe5, 0x14, 0x54, 0xc0, 0x60, 0x05,
	0x43, 0x05, 0x03, 0x80, 0x03, 0x53, 0x05, 0xfc, 0xef, 0x54, 0x3f,
	0x44, 0x40, 0xff, 0x80, 0x06, 0x53, 0x07, 0x3f, 0x53, 0x05, 0xf0,
	0xe5, 0x24, 0x30, 0xe0, 0x05, 0x43, 0x05, 0x10, 0x80, 0x03, 0x53,
	0x05, 0xef, 0x90, 0x7f, 0xfc, 0xe5, 0x26, 0xf0, 0xa3, 0xe5, 0x28,
	0xf0, 0xa3, 0xef, 0xf0, 0xa3, 0xed, 0xf0, 0x22, 0x8f, 0x24, 0xa9,
	0x05, 0x90, 0x7f, 0xfc, 0xe0, 0x75, 0x25, 0x00, 0xf5, 0x26, 0xa3,
	0xe0, 0x75, 0x27, 0x00, 0xf5, 0x28, 0xa3, 0xe0, 0xff, 0xa3, 0xe0,
	0xfd, 0xe5, 0x24, 0x30, 0xe5, 0x0b, 0x43, 0x05, 0x0f, 0xef, 0x54,
	0x3f, 0x44, 0x40, 0xff, 0x80, 0x06, 0x53, 0x05, 0xf0, 0x53, 0x07,
	0x3f, 0xe9, 0x30, 0xe0, 0x05, 0x43, 0x05, 0x10, 0x80, 0x03, 0x53,
	0x05, 0xef, 0x90, 0x7f, 0xfc, 0xe5, 0x26, 0xf0, 0xa3, 0xe5, 0x28,
	0xf0, 0xa3, 0xef, 0xf0, 0xa3, 0xed, 0xf0, 0x22, 0x90, 0x7f, 0xfc,
	0xe0, 0xf9, 0xa3, 0xe0, 0xfe, 0xa3, 0xe0, 0xfc, 0xa3, 0xe0, 0xfb,
	0xef, 0x30, 0xe5, 0x0b, 0x43, 0x03, 0x0f, 0xec, 0x54, 0x3f, 0x44,
	0x40, 0xfc, 0x80, 0x06, 0x53, 0x03, 0xf0, 0x53, 0x04, 0x3f, 0xed,
	0x30, 0xe0, 0x07, 0xef, 0x54, 0xc0, 0x60, 0x07, 0x80, 0x0a, 0xef,
	0x54, 0xc0, 0x60, 0x05, 0x43, 0x03, 0x10, 0x80, 0x03, 0x53, 0x03,
	0xef, 0x90, 0x7f, 0xfc, 0xe9, 0xf0, 0xa3, 0xee, 0xf0, 0xa3, 0xec,
	0xf0, 0xa3, 0xeb, 0xf0, 0x22, 0xe5, 0x4b, 0xfd, 0x54, 0x1f, 0x90,
	0x7f, 0xf8, 0xf0, 0xe5, 0x4a, 0xf5, 0x09, 0x90, 0x30, 0x38, 0xe0,
	0x90, 0x7f, 0xfc, 0xf0, 0x90, 0x30, 0x39, 0xe0, 0x90, 0x7f, 0xfd,
	0xf0, 0x90, 0x30, 0x3a, 0xe0, 0x90, 0x7f, 0xfe, 0xf0, 0x90, 0x30,
	0x3b, 0xe0, 0x90, 0x7f, 0xff, 0xf0, 0xed, 0x30, 0xe5, 0x0c, 0x54,
	0xc0, 0x60, 0x0d, 0x90, 0x7f, 0xf0, 0xe5, 0x47, 0xf0, 0x80, 0x05,
	0xe4, 0x90, 0x7f, 0xf0, 0xf0, 0x90, 0x7f, 0xf8, 0xe0, 0x14, 0x60,
	0x08, 0x24, 0xfe, 0x60, 0x0d, 0x24, 0x03, 0x80, 0x12, 0xaf, 0x05,
	0xad, 0x09, 0x12, 0x13, 0xc5, 0x80, 0x10, 0xaf, 0x05, 0xad, 0x09,
	0x12, 0x14, 0x12, 0x80, 0x07, 0xaf, 0x05, 0xad, 0x09, 0x12, 0x13,
	0x6f, 0x90, 0x7f, 0xfc, 0xe0, 0x90, 0x30, 0x38, 0xf0, 0x90, 0x7f,
	0xfd, 0xe0, 0x90, 0x30, 0x39, 0xf0, 0x90, 0x7f, 0xfe, 0xe0, 0x90,
	0x30, 0x3a, 0xf0, 0x90, 0x7f, 0xff, 0xe0, 0x90, 0x30, 0x3b, 0xf0,
	0x22, 0xe5, 0x4b, 0x64, 0x01, 0x60, 0x03, 0x02, 0x15, 0x71, 0xf5,
	0x4b, 0xe5, 0x44, 0x45, 0x43, 0x70, 0x03, 0x02, 0x15, 0xa0, 0x12,
	0x0c, 0x14, 0x12, 0x0b, 0x86, 0x50, 0xfb, 0x90, 0x00, 0x00, 0xe0,
	0xf5, 0x25, 0x12, 0x15, 0xb4, 0xc2, 0x92, 0xe4, 0xf5, 0x24, 0xe5,
	0x24, 0xc3, 0x95, 0x25, 0x50, 0x49, 0x7e, 0x00, 0x7f, 0x4c, 0x74,
	0x40, 0x25, 0x24, 0xf5, 0x82, 0xe4, 0x34, 0x01, 0xad, 0x82, 0xfc,
	0x75, 0x2b, 0x02, 0x7b, 0x10, 0x12, 0x07, 0x1e, 0xc2, 0x93, 0x12,
	0x15, 0xa1, 0x7d, 0xa0, 0x12, 0x15, 0xd0, 0xe5, 0x24, 0x54, 0x0f,
	0x24, 0x4c, 0xf8, 0xe6, 0xfd, 0xaf, 0x4b, 0xae, 0x4a, 0x12, 0x15,
	0xd8, 0x05, 0x4b, 0xe5, 0x4b, 0x70, 0x02, 0x05, 0x4a, 0x12, 0x0a,
	0x5f, 0x05, 0x24, 0xe5, 0x24, 0x54, 0x0f, 0x70, 0xd5, 0xd2, 0x93,
	0x80, 0xb0, 0xc3, 0xe5, 0x44, 0x95, 0x25, 0xf5, 0x44, 0xe5, 0x43,
	0x94, 0x00, 0xf5, 0x43, 0x02, 0x14, 0xf2, 0x12, 0x15, 0xb4, 0xc2,
	0x93, 0xc2, 0x92, 0x12, 0x15, 0xa1, 0x7d, 0x80, 0x12, 0x15, 0xd0,
	0x7d, 0xaa, 0x74, 0x55, 0xff, 0xfe, 0x12, 0x15, 0xd8, 0x7d, 0x55,
	0x7f, 0xaa, 0x7e, 0x2a, 0x12, 0x15, 0xd8, 0x7d, 0x30, 0xaf, 0x4b,
	0xae, 0x4a, 0x12, 0x15, 0xd8, 0x12, 0x0a, 0x5f, 0xd2, 0x93, 0x22,
	0x7d, 0xaa, 0x74, 0x55, 0xff, 0xfe, 0x12, 0x15, 0xd8, 0x7d, 0x55,
	0x7f, 0xaa, 0x7e, 0x2a, 0x12, 0x15, 0xd8, 0x22, 0xad, 0x47, 0x7f,
	0x34, 0x7e, 0x30, 0x12, 0x15, 0xd8, 0x7d, 0xff, 0x7f, 0x35, 0x7e,
	0x30, 0x12, 0x15, 0xd8, 0xe4, 0xfd, 0x7f, 0x37, 0x7e, 0x30, 0x12,
	0x15, 0xd8, 0x22, 0x74, 0x55, 0xff, 0xfe, 0x12, 0x15, 0xd8, 0x22,
	0x8f, 0x82, 0x8e, 0x83, 0xed, 0xf0, 0x22, 0xe4, 0xfc, 0x90, 0x7f,
	0xf0, 0xe0, 0xaf, 0x09, 0x14, 0x60, 0x14, 0x14, 0x60, 0x15, 0x14,
	0x60, 0x16, 0x14, 0x60, 0x17, 0x14, 0x60, 0x18, 0x24, 0x05, 0x70,
	0x16, 0xe4, 0xfc, 0x80, 0x12, 0x7c, 0x01, 0x80, 0x0e, 0x7c, 0x03,
	0x80, 0x0a, 0x7c, 0x07, 0x80, 0x06, 0x7c, 0x0f, 0x80, 0x02, 0x7c,
	0x1f, 0xec, 0x6f, 0xf4, 0x54, 0x1f, 0xfc, 0x90, 0x30, 0x34, 0xe0,
	0x54, 0xe0, 0x4c, 0xfd, 0xa3, 0xe0, 0xfc, 0x43, 0x04, 0x1f, 0x7f,
	0x34, 0x7e, 0x30, 0x12, 0x15, 0xd8, 0xad, 0x04, 0x0f, 0x12, 0x15,
	0xd8, 0xe4, 0xfd, 0x7f, 0x37, 0x02, 0x15, 0xd8, 0x02, 0x15, 0xdf,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07,
	0x29, 0xe9 
};
