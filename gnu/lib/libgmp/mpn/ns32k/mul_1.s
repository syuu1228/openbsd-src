# ns32000 __mpn_mul_1 -- Multiply a limb vector with a limb and store
# the result in a second limb vector.

# Copyright (C) 1992, 1994 Free Software Foundation, Inc.

# This file is part of the GNU MP Library.

# The GNU MP Library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.

# The GNU MP Library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
# License for more details.

# You should have received a copy of the GNU Library General Public License
# along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA.


	.align 1
.globl ___mpn_mul_1
___mpn_mul_1:
	save	[r3,r4,r5,r6,r7]
	negd	24(sp),r4
	movd	r4,r0
	lshd	2,r0
	movd	20(sp),r5
	subd	r0,r5			# r5 -> to end of S1
	movd	16(sp),r6
	subd	r0,r6			# r6 -> to end of RES
	subd	r0,r0			# r0 = 0, cy = 0
	movd	28(sp),r7		# r7 = s2_limb

Loop:	movd	r5[r4:d],r2
	meid	r7,r2			# r2 = low_prod, r3 = high_prod
	addcd	r0,r2			# r2 = low_prod + cy_limb
	movd	r3,r0			# r0 = new cy_limb
	movd	r2,r6[r4:d]
	acbd	1,r4,Loop

	addcd	0,r0
	restore	[r7,r6,r5,r4,r3]
	ret	0
