/* BFD support for the Motorola 68HC12 processor
   Copyright 1999, 2000, 2002, 2003 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"

static bfd_boolean scan_mach
  PARAMS ((const struct bfd_arch_info *, const char *));

static bfd_boolean
scan_mach (info, string)
     const struct bfd_arch_info *info;
     const char *string;
{
  if (strcasecmp (info->printable_name, string) == 0)
    return TRUE;
  return FALSE;
}

const bfd_arch_info_type bfd_m68hc12s_arch =
{
  16,	/* 16 bits in a word */
  16,	/* 16 bits in an address */
  8,	/* 8 bits in a byte */
  bfd_arch_m68hc12,
  bfd_mach_m6812s,
  "m68hcs12",
  "m68hcs12",
  4, /* section alignment power */
  FALSE,
  bfd_default_compatible,
  scan_mach,
  0,
};

const bfd_arch_info_type bfd_m68hc12_arch =
{
  16,	/* 16 bits in a word */
  16,	/* 16 bits in an address */
  8,	/* 8 bits in a byte */
  bfd_arch_m68hc12,
  0,
  "m68hc12",
  "m68hc12",
  4, /* section alignment power */
  TRUE,
  bfd_default_compatible,
  scan_mach,
  &bfd_m68hc12s_arch,
};
