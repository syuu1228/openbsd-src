/* vms-tir.c -- BFD back-end for VAX (openVMS/VAX) and
   EVAX (openVMS/Alpha) files.
   Copyright 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.

   TIR record handling functions
   ETIR record handling functions

   go and read the openVMS linker manual (esp. appendix B)
   if you don't know what's going on here :-)

   Written by Klaus K"ampf (kkaempf@rmi.de)

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

/* The following type abbreviations are used:

	cs	counted string (ascii string with length byte)
	by	byte (1 byte)
	sh	short (2 byte, 16 bit)
	lw	longword (4 byte, 32 bit)
	qw	quadword (8 byte, 64 bit)
	da	data stream  */

#include <ctype.h>

#include "bfd.h"
#include "sysdep.h"
#include "bfdlink.h"
#include "libbfd.h"

#include "vms.h"

static void image_set_ptr PARAMS ((bfd *abfd, int psect, uquad offset));
static void image_inc_ptr PARAMS ((bfd *abfd, uquad offset));
static void image_dump PARAMS ((bfd *abfd, unsigned char *ptr, int size, int offset));
static void image_write_b PARAMS ((bfd *abfd, unsigned int value));
static void image_write_w PARAMS ((bfd *abfd, unsigned int value));
static void image_write_l PARAMS ((bfd *abfd, unsigned long value));
static void image_write_q PARAMS ((bfd *abfd, uquad value));

/*-----------------------------------------------------------------------------*/

static int
check_section (abfd, size)
     bfd *abfd;
     int size;
{
  int offset;

  offset = PRIV(image_ptr) - PRIV(image_section)->contents;
  if ((bfd_size_type) (offset + size) > PRIV(image_section)->_raw_size)
    {
      PRIV(image_section)->contents = bfd_realloc (PRIV(image_section)->contents, offset + size);
      if (PRIV(image_section)->contents == 0)
	{
	  (*_bfd_error_handler) (_("No Mem !"));
	  return -1;
	}
      PRIV(image_section)->_raw_size = offset + size;
      PRIV(image_ptr) = PRIV(image_section)->contents + offset;
    }

  return 0;
}

/* routines to fill sections contents during tir/etir read */

/* Initialize image buffer pointer to be filled  */

static void
image_set_ptr (abfd, psect, offset)
     bfd *abfd;
     int psect;
     uquad offset;
{
#if VMS_DEBUG
  _bfd_vms_debug (4, "image_set_ptr (%d=%s, %d)\n",
		psect, PRIV(sections)[psect]->name, offset);
#endif

  PRIV(image_ptr) = PRIV(sections)[psect]->contents + offset;
  PRIV(image_section) = PRIV(sections)[psect];
  return;
}

/* Increment image buffer pointer by offset  */

static void
image_inc_ptr (abfd, offset)
     bfd *abfd;
     uquad offset;
{
#if VMS_DEBUG
  _bfd_vms_debug (4, "image_inc_ptr (%d)\n", offset);
#endif

  PRIV(image_ptr) += offset;

  return;
}

/* Dump multiple bytes to section image  */

static void
image_dump (abfd, ptr, size, offset)
    bfd *abfd;
    unsigned char *ptr;
    int size;
    int offset ATTRIBUTE_UNUSED;
{
#if VMS_DEBUG
  _bfd_vms_debug (8, "image_dump from (%p, %d) to (%p)\n", ptr, size, PRIV(image_ptr));
  _bfd_hexdump (9, ptr, size, offset);
#endif

  if (PRIV(is_vax) && check_section (abfd, size))
    return;

  while (size-- > 0)
    *PRIV(image_ptr)++ = *ptr++;
  return;
}

/* Write byte to section image  */

static void
image_write_b (abfd, value)
     bfd *abfd;
     unsigned int value;
{
#if VMS_DEBUG
  _bfd_vms_debug (6, "image_write_b(%02x)\n", (int)value);
#endif

  if (PRIV(is_vax) && check_section (abfd, 1))
    return;

  *PRIV(image_ptr)++ = (value & 0xff);
  return;
}

/* Write 2-byte word to image  */

static void
image_write_w (abfd, value)
     bfd *abfd;
     unsigned int value;
{
#if VMS_DEBUG
  _bfd_vms_debug (6, "image_write_w(%04x)\n", (int)value);
#endif

  if (PRIV(is_vax) && check_section (abfd, 2))
    return;

  bfd_putl16 (value, PRIV(image_ptr));
  PRIV(image_ptr) += 2;

  return;
}

/* Write 4-byte long to image  */

static void
image_write_l (abfd, value)
     bfd *abfd;
     unsigned long value;
{
#if VMS_DEBUG
  _bfd_vms_debug (6, "image_write_l (%08lx)\n", value);
#endif

  if (PRIV(is_vax) && check_section (abfd, 4))
    return;

  bfd_putl32 (value, PRIV(image_ptr));
  PRIV(image_ptr) += 4;

  return;
}

/* Write 8-byte quad to image  */

static void
image_write_q (abfd, value)
     bfd *abfd;
     uquad value;
{
#if VMS_DEBUG
  _bfd_vms_debug (6, "image_write_q (%016lx)\n", value);
#endif

  if (PRIV(is_vax) && check_section (abfd, 8))
    return;

  bfd_putl64 (value, PRIV(image_ptr));
  PRIV(image_ptr) += 8;

  return;
}


#define HIGHBIT(op) ((op & 0x80000000L) == 0x80000000L)

/* etir_sta

   vms stack commands

   handle sta_xxx commands in etir section
   ptr points to data area in record

   see table B-8 of the openVMS linker manual  */

static boolean
etir_sta (abfd, cmd, ptr)
     bfd *abfd;
     int cmd;
     unsigned char *ptr;
{

#if VMS_DEBUG
  _bfd_vms_debug (5, "etir_sta %d/%x\n", cmd, cmd);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  switch (cmd)
    {
      /* stack */

      /* stack global
	   arg: cs	symbol name

	   stack 32 bit value of symbol (high bits set to 0)  */

      case ETIR_S_C_STA_GBL:
	{
	  char *name;
	  vms_symbol_entry *entry;

	  name = _bfd_vms_save_counted_string (ptr);
	  entry = (vms_symbol_entry *)
		  bfd_hash_lookup (PRIV(vms_symbol_table), name, false, false);
	  if (entry == (vms_symbol_entry *)NULL)
	    {
#if VMS_DEBUG
	      _bfd_vms_debug (3, "ETIR_S_C_STA_GBL: no symbol \"%s\"\n", name);
#endif
	      _bfd_vms_push (abfd, (uquad)0, -1);
	    }
	  else
	    {
	      _bfd_vms_push (abfd, (uquad) (entry->symbol->value), -1);
	    }
	}
      break;

	/* stack longword
	   arg: lw	value

	   stack 32 bit value, sign extend to 64 bit  */

      case ETIR_S_C_STA_LW:
	_bfd_vms_push (abfd, (uquad)bfd_getl32 (ptr), -1);
      break;

	/* stack global
	   arg: qw	value

	   stack 64 bit value of symbol	 */

      case ETIR_S_C_STA_QW:
	_bfd_vms_push (abfd, (uquad)bfd_getl64(ptr), -1);
      break;

	/* stack psect base plus quadword offset
	   arg: lw	section index
	  	qw	signed quadword offset (low 32 bits)

	   stack qw argument and section index
	   (see ETIR_S_C_STO_OFF, ETIR_S_C_CTL_SETRB)  */

      case ETIR_S_C_STA_PQ:
  	{
	  uquad dummy;
	  unsigned int psect;

	  psect = bfd_getl32 (ptr);
	  if (psect >= PRIV(section_count))
	    {
	      (*_bfd_error_handler) (_("Bad section index in ETIR_S_C_STA_PQ"));
	      bfd_set_error (bfd_error_bad_value);
	      return false;
	    }
	  dummy = bfd_getl64 (ptr+4);
	  _bfd_vms_push (abfd, dummy, psect);
        }
      break;

	/* all not supported  */

      case ETIR_S_C_STA_LI:
      case ETIR_S_C_STA_MOD:
      case ETIR_S_C_STA_CKARG:

	(*_bfd_error_handler) (_("Unsupported STA cmd %d"), cmd);
	return false;
      break;

      default:
	(*_bfd_error_handler) (_("Reserved STA cmd %d"), cmd);
	return false;
      break;
    }
#if VMS_DEBUG
  _bfd_vms_debug (5, "etir_sta true\n");
#endif
  return true;
}

/*
   etir_sto

   vms store commands

   handle sto_xxx commands in etir section
   ptr points to data area in record

   see table B-9 of the openVMS linker manual  */

static boolean
etir_sto (abfd, cmd, ptr)
     bfd *abfd;
     int cmd;
     unsigned char *ptr;
{
  uquad dummy;
  int psect;

#if VMS_DEBUG
  _bfd_vms_debug (5, "etir_sto %d/%x\n", cmd, cmd);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  switch (cmd)
    {

      /* store byte: pop stack, write byte
	 arg: -  */

    case ETIR_S_C_STO_B:
      dummy = _bfd_vms_pop (abfd, &psect);
#if 0
      if (is_share)		/* FIXME */
	(*_bfd_error_handler) ("ETIR_S_C_STO_B: byte fixups not supported");
#endif
      image_write_b (abfd, dummy & 0xff);	/* FIXME: check top bits */
      break;

      /* store word: pop stack, write word
	 arg: -  */

    case ETIR_S_C_STO_W:
      dummy = _bfd_vms_pop (abfd, &psect);
#if 0
      if (is_share)		/* FIXME */
	(*_bfd_error_handler) ("ETIR_S_C_STO_B: word fixups not supported");
#endif
      image_write_w (abfd, dummy & 0xffff);	/* FIXME: check top bits */
      break;

      /* store longword: pop stack, write longword
	 arg: -  */

    case ETIR_S_C_STO_LW:
      dummy = _bfd_vms_pop (abfd, &psect);
      dummy += (PRIV(sections)[psect])->vma;
      image_write_l (abfd, dummy & 0xffffffff);/* FIXME: check top bits */
      break;

      /* store quadword: pop stack, write quadword
	 arg: -  */

    case ETIR_S_C_STO_QW:
      dummy = _bfd_vms_pop (abfd, &psect);
      dummy += (PRIV(sections)[psect])->vma;
      image_write_q (abfd, dummy);		/* FIXME: check top bits */
      break;

      /* store immediate repeated: pop stack for repeat count
	 arg: lw	byte count
	 da	data  */

    case ETIR_S_C_STO_IMMR:
      {
	unsigned long size;

	size = bfd_getl32 (ptr);
	dummy = (unsigned long)_bfd_vms_pop (abfd, NULL);
	while (dummy-- > 0L)
	  image_dump (abfd, ptr+4, size, 0);
      }
      break;

      /* store global: write symbol value
	 arg: cs	global symbol name  */

    case ETIR_S_C_STO_GBL:
      {
	vms_symbol_entry *entry;
	char *name;

	name = _bfd_vms_save_counted_string (ptr);
	entry = (vms_symbol_entry *)bfd_hash_lookup (PRIV(vms_symbol_table), name, false, false);
	if (entry == (vms_symbol_entry *)NULL)
	  {
	    (*_bfd_error_handler) (_("ETIR_S_C_STO_GBL: no symbol \"%s\""),
				   name);
	    return false;
	  }
	else
	  image_write_q (abfd, (uquad) (entry->symbol->value));	/* FIXME, reloc */
      }
      break;

      /* store code address: write address of entry point
	 arg: cs	global symbol name (procedure)  */

    case ETIR_S_C_STO_CA:
      {
	vms_symbol_entry *entry;
	char *name;

	name = _bfd_vms_save_counted_string (ptr);
	entry = (vms_symbol_entry *) bfd_hash_lookup (PRIV(vms_symbol_table), name, false, false);
	if (entry == (vms_symbol_entry *)NULL)
	  {
	    (*_bfd_error_handler) (_("ETIR_S_C_STO_CA: no symbol \"%s\""),
				   name);
	    return false;
	  }
	else
	  image_write_q (abfd, (uquad) (entry->symbol->value));	/* FIXME, reloc */
      }
      break;

      /* not supported  */

    case ETIR_S_C_STO_RB:
    case ETIR_S_C_STO_AB:
      (*_bfd_error_handler) (_("ETIR_S_C_STO_RB/AB: Not supported"));
      break;

    /* store offset to psect: pop stack, add low 32 bits to base of psect
       arg: -  */

    case ETIR_S_C_STO_OFF:
      {
	uquad q;
	int psect;

	q = _bfd_vms_pop (abfd, &psect);
	q += (PRIV(sections)[psect])->vma;
	image_write_q (abfd, q);
      }
      break;

      /* store immediate
	 arg: lw	count of bytes
	 da	data  */

    case ETIR_S_C_STO_IMM:
      {
	int size;

	size = bfd_getl32 (ptr);
	image_dump (abfd, ptr+4, size, 0);
      }
      break;

      /* this code is 'reserved to digital' according to the openVMS linker manual,
	 however it is generated by the DEC C compiler and defined in the include file.
	 FIXME, since the following is just a guess
	 store global longword: store 32bit value of symbol
	 arg: cs	symbol name  */

    case ETIR_S_C_STO_GBL_LW:
      {
	vms_symbol_entry *entry;
	char *name;

	name = _bfd_vms_save_counted_string (ptr);
	entry = (vms_symbol_entry *)bfd_hash_lookup (PRIV(vms_symbol_table), name, false, false);
	if (entry == (vms_symbol_entry *)NULL)
	  {
#if VMS_DEBUG
	    _bfd_vms_debug (3, "ETIR_S_C_STO_GBL_LW: no symbol \"%s\"\n", name);
#endif
	    image_write_l (abfd, (unsigned long)0);	/* FIXME, reloc */
	  }
	else
	  image_write_l (abfd, (unsigned long) (entry->symbol->value));	/* FIXME, reloc */
      }
      break;

      /* not supported  */

    case ETIR_S_C_STO_LP_PSB:
      (*_bfd_error_handler) (_("ETIR_S_C_STO_LP_PSB: Not supported"));
      break;

    /* */

    case ETIR_S_C_STO_HINT_GBL:
      (*_bfd_error_handler) (_("ETIR_S_C_STO_HINT_GBL: not implemented"));
      break;

    /* */

    case ETIR_S_C_STO_HINT_PS:
      (*_bfd_error_handler) (_("ETIR_S_C_STO_HINT_PS: not implemented"));
      break;

    default:
      (*_bfd_error_handler) (_("Reserved STO cmd %d"), cmd);
      break;
    }

  return true;
}

/* stack operator commands
   all 32 bit signed arithmetic
   all word just like a stack calculator
   arguments are popped from stack, results are pushed on stack

   see table B-10 of the openVMS linker manual  */

static boolean
etir_opr (abfd, cmd, ptr)
     bfd *abfd;
     int cmd;
     unsigned char *ptr ATTRIBUTE_UNUSED;
{
  long op1, op2;

#if VMS_DEBUG
  _bfd_vms_debug (5, "etir_opr %d/%x\n", cmd, cmd);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  switch (cmd)
    {
      /* operation */

      /* no-op  */

    case ETIR_S_C_OPR_NOP:
      break;

      /* add  */

    case ETIR_S_C_OPR_ADD:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op1 + op2), -1);
      break;

      /* subtract  */

    case ETIR_S_C_OPR_SUB:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op2 - op1), -1);
      break;

      /* multiply  */

    case ETIR_S_C_OPR_MUL:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op1 * op2), -1);
      break;

      /* divide  */

    case ETIR_S_C_OPR_DIV:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      if (op2 == 0)
	_bfd_vms_push (abfd, (uquad)0L, -1);
      else
	_bfd_vms_push (abfd, (uquad) (op2 / op1), -1);
      break;

      /* logical and  */

    case ETIR_S_C_OPR_AND:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op1 & op2), -1);
      break;

      /* logical inclusive or	 */

    case ETIR_S_C_OPR_IOR:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op1 | op2), -1);
      break;

      /* logical exclusive or  */

    case ETIR_S_C_OPR_EOR:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op1 ^ op2), -1);
      break;

      /* negate  */

    case ETIR_S_C_OPR_NEG:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (-op1), -1);
      break;

      /* complement  */

    case ETIR_S_C_OPR_COM:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      _bfd_vms_push (abfd, (uquad) (op1 ^ -1L), -1);
      break;

      /* insert field  */

    case ETIR_S_C_OPR_INSV:
      (void)_bfd_vms_pop (abfd, NULL);
      (*_bfd_error_handler) (_("ETIR_S_C_OPR_INSV: Not supported"));
      break;

    /* arithmetic shift  */

    case ETIR_S_C_OPR_ASH:
      op1 = (long)_bfd_vms_pop (abfd, NULL);
      op2 = (long)_bfd_vms_pop (abfd, NULL);
      if (op2 < 0)		/* shift right */
	op1 >>= -op2;
      else			/* shift left */
	op1 <<= op2;
      _bfd_vms_push (abfd, (uquad)op1, -1);
      break;

      /* unsigned shift  */

    case ETIR_S_C_OPR_USH:
      (*_bfd_error_handler) (_("ETIR_S_C_OPR_USH: Not supported"));
      break;

      /* rotate  */

    case ETIR_S_C_OPR_ROT:
      (*_bfd_error_handler) (_("ETIR_S_C_OPR_ROT: Not supported"));
      break;

      /* select  */

    case ETIR_S_C_OPR_SEL:
      if ((long)_bfd_vms_pop (abfd, NULL) & 0x01L)
	(void)_bfd_vms_pop (abfd, NULL);
      else
	{
	  op1 = (long)_bfd_vms_pop (abfd, NULL);
	  (void)_bfd_vms_pop (abfd, NULL);
	  _bfd_vms_push (abfd, (uquad)op1, -1);
	}
      break;

      /* redefine symbol to current location  */

    case ETIR_S_C_OPR_REDEF:
      (*_bfd_error_handler) (_("ETIR_S_C_OPR_REDEF: Not supported"));
      break;

      /* define a literal  */

    case ETIR_S_C_OPR_DFLIT:
      (*_bfd_error_handler) (_("ETIR_S_C_OPR_DFLIT: Not supported"));
      break;

    default:
      (*_bfd_error_handler) (_("Reserved OPR cmd %d"), cmd);
      break;
    }

  return true;
}

/* control commands

   see table B-11 of the openVMS linker manual  */

static boolean
etir_ctl (abfd, cmd, ptr)
     bfd *abfd;
     int cmd;
     unsigned char *ptr;
{
  uquad	 dummy;
  int psect;

#if VMS_DEBUG
  _bfd_vms_debug (5, "etir_ctl %d/%x\n", cmd, cmd);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  switch (cmd)
    {
      /* set relocation base: pop stack, set image location counter
	 arg: -  */

    case ETIR_S_C_CTL_SETRB:
      dummy = _bfd_vms_pop (abfd, &psect);
      image_set_ptr (abfd, psect, dummy);
      break;

      /* augment relocation base: increment image location counter by offset
	 arg: lw	offset value  */

    case ETIR_S_C_CTL_AUGRB:
      dummy = bfd_getl32 (ptr);
      image_inc_ptr (abfd, dummy);
      break;

      /* define location: pop index, save location counter under index
	 arg: -  */

    case ETIR_S_C_CTL_DFLOC:
      dummy = _bfd_vms_pop (abfd, NULL);
      /* FIXME */
      break;

      /* set location: pop index, restore location counter from index
	 arg: -  */

    case ETIR_S_C_CTL_STLOC:
      dummy = _bfd_vms_pop (abfd, &psect);
      /* FIXME */
      break;

      /* stack defined location: pop index, push location counter from index
	 arg: -  */

    case ETIR_S_C_CTL_STKDL:
      dummy = _bfd_vms_pop (abfd, &psect);
      /* FIXME */
      break;

    default:
      (*_bfd_error_handler) (_("Reserved CTL cmd %d"), cmd);
      break;
    }
  return true;
}

/* store conditional commands

   see table B-12 and B-13 of the openVMS linker manual  */

static boolean
etir_stc (abfd, cmd, ptr)
     bfd *abfd;
     int cmd;
     unsigned char *ptr ATTRIBUTE_UNUSED;
{

#if VMS_DEBUG
  _bfd_vms_debug (5, "etir_stc %d/%x\n", cmd, cmd);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  switch (cmd)
    {
      /* 200 Store-conditional Linkage Pair
	 arg:  */

    case ETIR_S_C_STC_LP:
      (*_bfd_error_handler) (_("ETIR_S_C_STC_LP: not supported"));
      break;

      /* 201 Store-conditional Linkage Pair with Procedure Signature
	 arg:	lw	linkage index
	 cs	procedure name
	 by	signature length
	 da	signature  */

    case ETIR_S_C_STC_LP_PSB:
      image_inc_ptr (abfd, 16);	/* skip entry,procval */
      break;

      /* 202 Store-conditional Address at global address
	 arg:	lw	linkage index
	 cs	global name  */

    case ETIR_S_C_STC_GBL:
      (*_bfd_error_handler) (_("ETIR_S_C_STC_GBL: not supported"));
      break;

      /* 203 Store-conditional Code Address at global address
	 arg:	lw	linkage index
	 cs	procedure name  */

    case ETIR_S_C_STC_GCA:
      (*_bfd_error_handler) (_("ETIR_S_C_STC_GCA: not supported"));
      break;

      /* 204 Store-conditional Address at psect + offset
	 arg:	lw	linkage index
	 lw	psect index
	 qw	offset  */

    case ETIR_S_C_STC_PS:
      (*_bfd_error_handler) (_("ETIR_S_C_STC_PS: not supported"));
      break;

      /* 205 Store-conditional NOP at address of global
	 arg:  */

    case ETIR_S_C_STC_NOP_GBL:

      /* 206 Store-conditional NOP at pect + offset
	 arg:  */

    case ETIR_S_C_STC_NOP_PS:

      /* 207 Store-conditional BSR at global address
	 arg:  */

    case ETIR_S_C_STC_BSR_GBL:

      /* 208 Store-conditional BSR at pect + offset
	 arg:  */

    case ETIR_S_C_STC_BSR_PS:

      /* 209 Store-conditional LDA at global address
	 arg:  */

    case ETIR_S_C_STC_LDA_GBL:

      /* 210 Store-conditional LDA at psect + offset
	 arg:  */

    case ETIR_S_C_STC_LDA_PS:

      /* 211 Store-conditional BSR or Hint at global address
	 arg:  */

    case ETIR_S_C_STC_BOH_GBL:

      /* 212 Store-conditional BSR or Hint at pect + offset
	 arg:  */

    case ETIR_S_C_STC_BOH_PS:

      /* 213 Store-conditional NOP,BSR or HINT at global address
	 arg:  */

    case ETIR_S_C_STC_NBH_GBL:

      /* 214 Store-conditional NOP,BSR or HINT at psect + offset
	 arg:  */

    case ETIR_S_C_STC_NBH_PS:
/* FIXME     (*_bfd_error_handler) ("ETIR_S_C_STC_xx: (%d) not supported", cmd); */
      break;

    default:
#if VMS_DEBUG
      _bfd_vms_debug (3,  "Reserved STC cmd %d", cmd);
#endif
      break;
    }
  return true;
}

static asection *
new_section (abfd, idx)
     bfd *abfd ATTRIBUTE_UNUSED;
     int idx;
{
  asection *section;
  char sname[16];
  char *name;

#if VMS_DEBUG
  _bfd_vms_debug (5,  "new_section %d\n", idx);
#endif
  sprintf (sname, SECTION_NAME_TEMPLATE, idx);

  name = bfd_malloc (strlen (sname) + 1);
  if (name == 0)
    return 0;
  strcpy (name, sname);

  section = bfd_malloc (sizeof (asection));
  if (section == 0)
    {
#if VMS_DEBUG
      _bfd_vms_debug (6,  "bfd_make_section (%s) failed", name);
#endif
      return 0;
    }

  section->_raw_size = 0;
  section->vma = 0;
  section->contents = 0;
  section->_cooked_size = 0;
  section->name = name;
  section->index = idx;

  return section;
}

static int
alloc_section (abfd, idx)
     bfd *abfd;
     unsigned int idx;
{
#if VMS_DEBUG
  _bfd_vms_debug (4,  "alloc_section %d\n", idx);
#endif

  PRIV(sections) = ((asection **)
		    bfd_realloc (PRIV(sections), (idx+1) * sizeof (asection *)));
  if (PRIV(sections) == 0)
    return -1;

  while (PRIV(section_count) <= idx)
    {
      PRIV(sections)[PRIV(section_count)] = new_section (abfd, PRIV(section_count));
      if (PRIV(sections)[PRIV(section_count)] == 0)
	return -1;
      PRIV(section_count)++;
    }

  return 0;
}

/*
 * tir_sta
 *
 * vax stack commands
 *
 * handle sta_xxx commands in tir section
 * ptr points to data area in record
 *
 * see table 7-3 of the VAX/VMS linker manual
 */

static unsigned char *
tir_sta (bfd *abfd, unsigned char *ptr)
{
  int cmd = *ptr++;

#if VMS_DEBUG
  _bfd_vms_debug (5, "tir_sta %d\n", cmd);
#endif

  switch (cmd)
    {
  /* stack */
      case TIR_S_C_STA_GBL:
	/*
	 * stack global
	 * arg: cs	symbol name
	 *
	 * stack 32 bit value of symbol (high bits set to 0)
	 */
	{
	  char *name;
	  vms_symbol_entry *entry;

	  name = _bfd_vms_save_counted_string (ptr);

          entry = _bfd_vms_enter_symbol (abfd, name);
	  if (entry == (vms_symbol_entry *)NULL)
	    return 0;

	  _bfd_vms_push (abfd, (unsigned long) (entry->symbol->value), -1);
	  ptr += *ptr + 1;
	}
      break;

      case TIR_S_C_STA_SB:
	/*
	 * stack signed byte
	 * arg: by	value
	 *
	 * stack byte value, sign extend to 32 bit
	 */
	_bfd_vms_push (abfd, (long)*ptr++, -1);
      break;

      case TIR_S_C_STA_SW:
	/*
	 * stack signed short word
	 * arg: sh	value
	 *
	 * stack 16 bit value, sign extend to 32 bit
	 */
	_bfd_vms_push (abfd, (long)bfd_getl16(ptr), -1);
	ptr += 2;
      break;

      case TIR_S_C_STA_LW:
	/*
	 * stack signed longword
	 * arg: lw	value
	 *
	 * stack 32 bit value
	 */
	_bfd_vms_push (abfd, (long)bfd_getl32 (ptr), -1);
	ptr += 4;
      break;

      case TIR_S_C_STA_PB:
      case TIR_S_C_STA_WPB:
	/*
	 * stack psect base plus byte offset (word index)
	 * arg: by	section index
	 *	(sh	section index)
	 *	by	signed byte offset
	 *
	 */
  	{
	  unsigned long dummy;
	  unsigned int psect;

	  if (cmd == TIR_S_C_STA_PB)
	    psect = *ptr++;
	  else
	    {
	      psect = bfd_getl16(ptr);
	      ptr += 2;
	    }

	  if (psect >= PRIV(section_count))
	    {
	      alloc_section (abfd, psect);
	    }

	  dummy = (long)*ptr++;
	  dummy += (PRIV(sections)[psect])->vma;
	  _bfd_vms_push (abfd, dummy, psect);
        }
      break;

      case TIR_S_C_STA_PW:
      case TIR_S_C_STA_WPW:
	/*
	 * stack psect base plus word offset (word index)
	 * arg: by	section index
	 *	(sh	section index)
	 *	sh	signed short offset
	 *
	 */
  	{
	  unsigned long dummy;
	  unsigned int psect;

	  if (cmd == TIR_S_C_STA_PW)
	    psect = *ptr++;
	  else
	    {
	      psect = bfd_getl16(ptr);
	      ptr += 2;
	    }

	  if (psect >= PRIV(section_count))
	    {
	      alloc_section (abfd, psect);
	    }

	  dummy = bfd_getl16(ptr); ptr+=2;
	  dummy += (PRIV(sections)[psect])->vma;
	  _bfd_vms_push (abfd, dummy, psect);
        }
      break;

      case TIR_S_C_STA_PL:
      case TIR_S_C_STA_WPL:
	/*
	 * stack psect base plus long offset (word index)
	 * arg: by	section index
	 *	(sh	section index)
	 *	lw	signed longword offset
	 *
	 */
  	{
	  unsigned long dummy;
	  unsigned int psect;

	  if (cmd == TIR_S_C_STA_PL)
	    psect = *ptr++;
	  else
	    {
	      psect = bfd_getl16(ptr);
	      ptr += 2;
	    }

	  if (psect >= PRIV(section_count))
	    {
	      alloc_section (abfd, psect);
	    }

	  dummy = bfd_getl32 (ptr); ptr += 4;
	  dummy += (PRIV(sections)[psect])->vma;
	  _bfd_vms_push (abfd, dummy, psect);
        }
      break;

      case TIR_S_C_STA_UB:
	/*
	 * stack unsigned byte
	 * arg: by	value
	 *
	 * stack byte value
	 */
	_bfd_vms_push (abfd, (unsigned long)*ptr++, -1);
      break;

      case TIR_S_C_STA_UW:
	/*
	 * stack unsigned short word
	 * arg: sh	value
	 *
	 * stack 16 bit value
	 */
	_bfd_vms_push (abfd, (unsigned long)bfd_getl16(ptr), -1);
	ptr += 2;
      break;

      case TIR_S_C_STA_BFI:
	/*
	 * stack byte from image
	 * arg: -
	 *
	 */
	/*FALLTHRU*/
      case TIR_S_C_STA_WFI:
	/*
	 * stack byte from image
	 * arg: -
	 *
	 */
	/*FALLTHRU*/
      case TIR_S_C_STA_LFI:
	/*
	 * stack byte from image
	 * arg: -
	 *
	 */
        (*_bfd_error_handler) (_("Stack-from-image not implemented"));
	return NULL;

      case TIR_S_C_STA_EPM:
	/*
	 * stack entry point mask
	 * arg: cs	symbol name
	 *
	 * stack (unsigned) entry point mask of symbol
	 * err if symbol is no entry point
	 */
	{
	  char *name;
	  vms_symbol_entry *entry;

	  name = _bfd_vms_save_counted_string (ptr);
	  entry = _bfd_vms_enter_symbol (abfd, name);
	  if (entry == (vms_symbol_entry *)NULL)
	    return 0;

          (*_bfd_error_handler) (_("Stack-entry-mask not fully implemented"));
	  _bfd_vms_push (abfd, 0L, -1);
	  ptr += *ptr + 1;
	}
      break;

      case TIR_S_C_STA_CKARG:
	/*
	 * compare procedure argument
	 * arg: cs	symbol name
	 *	by	argument index
	 *	da	argument descriptor
	 *
	 * compare argument descriptor with symbol argument (ARG$V_PASSMECH)
	 * and stack TRUE (args match) or FALSE (args dont match) value
	 */
        (*_bfd_error_handler) (_("PASSMECH not fully implemented"));
	_bfd_vms_push (abfd, 1L, -1);
	break;

      case TIR_S_C_STA_LSY:
	/*
	 * stack local symbol value
	 * arg:	sh	environment index
	 *	cs	symbol name
	 */
	{
	  int envidx;
	  char *name;
	  vms_symbol_entry *entry;

	  envidx = bfd_getl16(ptr); ptr += 2;
	  name = _bfd_vms_save_counted_string (ptr);
	  entry = _bfd_vms_enter_symbol (abfd, name);
	  if (entry == (vms_symbol_entry *)NULL)
	    return 0;
          (*_bfd_error_handler) (_("Stack-local-symbol not fully implemented"));
	  _bfd_vms_push (abfd, 0L, -1);
	  ptr += *ptr + 1;
	}
      break;

      case TIR_S_C_STA_LIT:
	/*
	 * stack literal
 	 * arg:	by	literal index
	 *
	 * stack literal
	 */
	ptr++;
	_bfd_vms_push (abfd, 0L, -1);
	(*_bfd_error_handler) (_("Stack-literal not fully implemented"));
	break;

      case TIR_S_C_STA_LEPM:
	/*
	 * stack local symbol entry point mask
	 * arg:	sh	environment index
	 *	cs	symbol name
	 *
	 * stack (unsigned) entry point mask of symbol
	 * err if symbol is no entry point
	 */
	{
	  int envidx;
	  char *name;
	  vms_symbol_entry *entry;

	  envidx = bfd_getl16(ptr); ptr += 2;
	  name = _bfd_vms_save_counted_string (ptr);
	  entry = _bfd_vms_enter_symbol (abfd, name);
	  if (entry == (vms_symbol_entry *)NULL)
	    return 0;
	  (*_bfd_error_handler) (_("Stack-local-symbol-entry-point-mask not fully implemented"));
	  _bfd_vms_push (abfd, 0L, -1);
	  ptr += *ptr + 1;
	}
      break;

      default:
	(*_bfd_error_handler) (_("Reserved STA cmd %d"), ptr[-1]);
	return NULL;
      break;
  }

  return ptr;
}

/*
 * tir_sto
 *
 * vax store commands
 *
 * handle sto_xxx commands in tir section
 * ptr points to data area in record
 *
 * see table 7-4 of the VAX/VMS linker manual
 */

static unsigned char *
tir_sto (bfd *abfd, unsigned char *ptr)
{
  unsigned long dummy;
  int size;
  int psect;

#if VMS_DEBUG
  _bfd_vms_debug (5, "tir_sto %d\n", *ptr);
#endif

  switch (*ptr++)
    {
      case TIR_S_C_STO_SB:
	/*
	 * store signed byte: pop stack, write byte
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	image_write_b (abfd, dummy & 0xff);	/* FIXME: check top bits */
      break;

      case TIR_S_C_STO_SW:
	/*
	 * store signed word: pop stack, write word
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	image_write_w (abfd, dummy & 0xffff);	/* FIXME: check top bits */
      break;

      case TIR_S_C_STO_LW:
	/*
	 * store longword: pop stack, write longword
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	image_write_l (abfd, dummy & 0xffffffff);/* FIXME: check top bits */
      break;

      case TIR_S_C_STO_BD:
	/*
	 * store byte displaced: pop stack, sub lc+1, write byte
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	dummy -= ((PRIV(sections)[psect])->vma + 1);
	image_write_b (abfd, dummy & 0xff);/* FIXME: check top bits */
      break;

      case TIR_S_C_STO_WD:
	/*
	 * store word displaced: pop stack, sub lc+2, write word
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	dummy -= ((PRIV(sections)[psect])->vma + 2);
	image_write_w (abfd, dummy & 0xffff);/* FIXME: check top bits */
      break;
      case TIR_S_C_STO_LD:
	/*
	 * store long displaced: pop stack, sub lc+4, write long
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	dummy -= ((PRIV(sections)[psect])->vma + 4);
	image_write_l (abfd, dummy & 0xffffffff);/* FIXME: check top bits */
      break;
      case TIR_S_C_STO_LI:
	/*
	 * store short literal: pop stack, write byte
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	image_write_b (abfd, dummy & 0xff);/* FIXME: check top bits */
      break;
      case TIR_S_C_STO_PIDR:
	/*
	 * store position independent data reference: pop stack, write longword
	 * arg: -
	 * FIXME: incomplete !
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	image_write_l (abfd, dummy & 0xffffffff);
      break;
      case TIR_S_C_STO_PICR:
	/*
	 * store position independent code reference: pop stack, write longword
	 * arg: -
	 * FIXME: incomplete !
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	image_write_b (abfd, 0x9f);
	image_write_l (abfd, dummy & 0xffffffff);
      break;
      case TIR_S_C_STO_RIVB:
	/*
	 * store repeated immediate variable bytes
	 * 1-byte count n field followed by n bytes of data
	 * pop stack, write n bytes <stack> times
	 */
	size = *ptr++;
	dummy = (unsigned long)_bfd_vms_pop (abfd, NULL);
	while (dummy-- > 0L)
	  image_dump (abfd, ptr, size, 0);
	ptr += size;
	break;
      case TIR_S_C_STO_B:
	/*
	 * store byte from top longword
	 */
	dummy = (unsigned long)_bfd_vms_pop (abfd, NULL);
	image_write_b (abfd, dummy & 0xff);
	break;
      case TIR_S_C_STO_W:
	/*
	 * store word from top longword
	 */
	dummy = (unsigned long)_bfd_vms_pop (abfd, NULL);
	image_write_w (abfd, dummy & 0xffff);
	break;
      case TIR_S_C_STO_RB:
	/*
	 * store repeated byte from top longword
	 */
	size = (unsigned long)_bfd_vms_pop (abfd, NULL);
	dummy = (unsigned long)_bfd_vms_pop (abfd, NULL);
	while (size-- > 0)
	  image_write_b (abfd, dummy & 0xff);
	break;
      case TIR_S_C_STO_RW:
	/*
	 * store repeated word from top longword
	 */
	size = (unsigned long)_bfd_vms_pop (abfd, NULL);
	dummy = (unsigned long)_bfd_vms_pop (abfd, NULL);
	while (size-- > 0)
	  image_write_w (abfd, dummy & 0xffff);
	break;

      case TIR_S_C_STO_RSB:
      case TIR_S_C_STO_RSW:
      case TIR_S_C_STO_RL:
      case TIR_S_C_STO_VPS:
      case TIR_S_C_STO_USB:
      case TIR_S_C_STO_USW:
      case TIR_S_C_STO_RUB:
      case TIR_S_C_STO_RUW:
      case TIR_S_C_STO_PIRR:
	(*_bfd_error_handler) (_("Unimplemented STO cmd %d"), ptr[-1]);
      break;

      default:
	(*_bfd_error_handler) (_("Reserved STO cmd %d"), ptr[-1]);
      break;
  }

  return ptr;
}

/*
 * stack operator commands
 * all 32 bit signed arithmetic
 * all word just like a stack calculator
 * arguments are popped from stack, results are pushed on stack
 *
 * see table 7-5 of the VAX/VMS linker manual
 */

static unsigned char *
tir_opr (bfd *abfd, unsigned char *ptr)
{
  long op1, op2;

#if VMS_DEBUG
  _bfd_vms_debug (5, "tir_opr %d\n", *ptr);
#endif

  switch (*ptr++)
    {
  /* operation */
      case TIR_S_C_OPR_NOP:
	/*
	 * no-op
	 */
      break;

      case TIR_S_C_OPR_ADD:
	/*
	 * add
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op1 + op2), -1);
      break;

      case TIR_S_C_OPR_SUB:
	/*
	 * subtract
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op2 - op1), -1);
      break;

      case TIR_S_C_OPR_MUL:
	/*
	 * multiply
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op1 * op2), -1);
      break;

      case TIR_S_C_OPR_DIV:
	/*
	 * divide
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	if (op2 == 0)
	  _bfd_vms_push (abfd, (unsigned long)0L, -1);
	else
	  _bfd_vms_push (abfd, (unsigned long) (op2 / op1), -1);
      break;

      case TIR_S_C_OPR_AND:
	/*
	 * logical and
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op1 & op2), -1);
      break;

      case TIR_S_C_OPR_IOR:
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	/*
	 * logical inclusive or
	 */
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op1 | op2), -1);
      break;

      case TIR_S_C_OPR_EOR:
	/*
	 * logical exclusive or
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op1 ^ op2), -1);
      break;

      case TIR_S_C_OPR_NEG:
	/*
	 * negate
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (-op1), -1);
      break;

      case TIR_S_C_OPR_COM:
	/*
	 * complement
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	_bfd_vms_push (abfd, (unsigned long) (op1 ^ -1L), -1);
      break;

      case TIR_S_C_OPR_INSV:
	/*
	 * insert field
	 */
	(void)_bfd_vms_pop (abfd, NULL);
	(*_bfd_error_handler)  ("TIR_S_C_OPR_INSV incomplete");
      break;

      case TIR_S_C_OPR_ASH:
	/*
	 * arithmetic shift
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	if (HIGHBIT(op1))		/* shift right */
	  op2 >>= op1;
	else			/* shift left */
	  op2 <<= op1;
	_bfd_vms_push (abfd, (unsigned long)op2, -1);
	(*_bfd_error_handler) (_("TIR_S_C_OPR_ASH incomplete"));
      break;

      case TIR_S_C_OPR_USH:
	/*
	 * unsigned shift
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	if (HIGHBIT(op1))		/* shift right */
	  op2 >>= op1;
	else			/* shift left */
	  op2 <<= op1;
	_bfd_vms_push (abfd, (unsigned long)op2, -1);
	(*_bfd_error_handler) (_("TIR_S_C_OPR_USH incomplete"));
      break;

      case TIR_S_C_OPR_ROT:
	/*
	 * rotate
	 */
	op1 = (long)_bfd_vms_pop (abfd, NULL);
	op2 = (long)_bfd_vms_pop (abfd, NULL);
	if (HIGHBIT(0))		/* shift right */
	  op2 >>= op1;
	else			/* shift left */
	  op2 <<= op1;
	_bfd_vms_push (abfd, (unsigned long)op2, -1);
	(*_bfd_error_handler) (_("TIR_S_C_OPR_ROT incomplete"));
      break;

      case TIR_S_C_OPR_SEL:
	/*
	 * select
	 */
	if ((long)_bfd_vms_pop (abfd, NULL) & 0x01L)
	  (void)_bfd_vms_pop (abfd, NULL);
	else
	  {
	    op1 = (long)_bfd_vms_pop (abfd, NULL);
	    (void)_bfd_vms_pop (abfd, NULL);
	    _bfd_vms_push (abfd, (unsigned long)op1, -1);
	  }
      break;

      case TIR_S_C_OPR_REDEF:
	/*
	 * redefine symbol to current location
	 */
	(*_bfd_error_handler) (_("TIR_S_C_OPR_REDEF not supported"));
      break;

      case TIR_S_C_OPR_DFLIT:
	/*
	 * define a literal
	 */
	(*_bfd_error_handler) (_("TIR_S_C_OPR_DFLIT not supported"));
      break;

      default:
	(*_bfd_error_handler) (_("Reserved OPR cmd %d"), ptr[-1]);
      break;
    }

  return ptr;
}

static unsigned char *
tir_ctl (bfd *abfd, unsigned char *ptr)
/*
 * control commands
 *
 * see table 7-6 of the VAX/VMS linker manual
 */
{
  unsigned long dummy;
  unsigned int psect;

#if VMS_DEBUG
  _bfd_vms_debug (5, "tir_ctl %d\n", *ptr);
#endif

  switch (*ptr++)
    {
      case TIR_S_C_CTL_SETRB:
	/*
	 * set relocation base: pop stack, set image location counter
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	if (psect >= PRIV(section_count))
	  {
	    alloc_section (abfd, psect);
	  }
	image_set_ptr (abfd, psect, dummy);
      break;
      case TIR_S_C_CTL_AUGRB:
	/*
	 * augment relocation base: increment image location counter by offset
	 * arg: lw	offset value
	 */
	dummy = bfd_getl32 (ptr);
	image_inc_ptr (abfd, dummy);
      break;
      case TIR_S_C_CTL_DFLOC:
	/*
	 * define location: pop index, save location counter under index
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, NULL);
	(*_bfd_error_handler) (_("TIR_S_C_CTL_DFLOC not fully implemented"));
      break;
      case TIR_S_C_CTL_STLOC:
	/*
	 * set location: pop index, restore location counter from index
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	(*_bfd_error_handler) (_("TIR_S_C_CTL_STLOC not fully implemented"));
      break;
    case TIR_S_C_CTL_STKDL:
	/*
	 * stack defined location: pop index, push location counter from index
	 * arg: -
	 */
	dummy = _bfd_vms_pop (abfd, &psect);
	(*_bfd_error_handler) (_("TIR_S_C_CTL_STKDL not fully implemented"));
      break;
    default:
        (*_bfd_error_handler) (_("Reserved CTL cmd %d"), ptr[-1]);
	break;
  }
  return ptr;
}

/*
 * handle command from TIR section
 */

static unsigned char *
tir_cmd (bfd *abfd, unsigned char *ptr)
{
  struct {
    int mincod;
    int maxcod;
    unsigned char * (*explain) (bfd *, unsigned char *);
  } tir_table[] = {
    { 0,		 TIR_S_C_MAXSTACOD, tir_sta }
   ,{ TIR_S_C_MINSTOCOD, TIR_S_C_MAXSTOCOD, tir_sto }
   ,{ TIR_S_C_MINOPRCOD, TIR_S_C_MAXOPRCOD, tir_opr }
   ,{ TIR_S_C_MINCTLCOD, TIR_S_C_MAXCTLCOD, tir_ctl }
   ,{ -1, -1, NULL }
  };
  int i = 0;

#if VMS_DEBUG
  _bfd_vms_debug (4, "tir_cmd %d/%x\n", *ptr, *ptr);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  if (*ptr & 0x80)				/* store immediate */
    {
      i = 128 - (*ptr++ & 0x7f);
      image_dump (abfd, ptr, i, 0);
      ptr += i;
    }
  else
    {
      while (tir_table[i].mincod >= 0)
	{
	  if ( (tir_table[i].mincod <= *ptr)
	    && (*ptr <= tir_table[i].maxcod))
	    {
	      ptr = tir_table[i].explain (abfd, ptr);
	      break;
	    }
	  i++;
	}
      if (tir_table[i].mincod < 0)
	{
	  (*_bfd_error_handler) (_("Obj code %d not found"), *ptr);
	  ptr = 0;
	}
    }

  return ptr;
}

/* handle command from ETIR section  */

static int
etir_cmd (abfd, cmd, ptr)
     bfd *abfd;
     int cmd;
     unsigned char *ptr;
{
  static struct {
    int mincod;
    int maxcod;
    boolean (*explain) PARAMS((bfd *, int, unsigned char *));
  } etir_table[] = {
    { ETIR_S_C_MINSTACOD, ETIR_S_C_MAXSTACOD, etir_sta },
    { ETIR_S_C_MINSTOCOD, ETIR_S_C_MAXSTOCOD, etir_sto },
    { ETIR_S_C_MINOPRCOD, ETIR_S_C_MAXOPRCOD, etir_opr },
    { ETIR_S_C_MINCTLCOD, ETIR_S_C_MAXCTLCOD, etir_ctl },
    { ETIR_S_C_MINSTCCOD, ETIR_S_C_MAXSTCCOD, etir_stc },
    { -1, -1, NULL }
  };

  int i = 0;

#if VMS_DEBUG
  _bfd_vms_debug (4, "etir_cmd %d/%x\n", cmd, cmd);
  _bfd_hexdump (8, ptr, 16, (int)ptr);
#endif

  while (etir_table[i].mincod >= 0)
    {
      if ( (etir_table[i].mincod <= cmd)
	&& (cmd <= etir_table[i].maxcod))
	{
	  if (!etir_table[i].explain (abfd, cmd, ptr))
	    return -1;
	  break;
	}
      i++;
    }

#if VMS_DEBUG
  _bfd_vms_debug (4, "etir_cmd: = 0\n");
#endif
  return 0;
}

/* Text Information and Relocation Records (OBJ$C_TIR)
   handle tir record  */

static int
analyze_tir (abfd, ptr, length)
     bfd *abfd;
     unsigned char *ptr;
     unsigned int length;
{
  unsigned char *maxptr;

#if VMS_DEBUG
  _bfd_vms_debug (3, "analyze_tir: %d bytes\n", length);
#endif

  maxptr = ptr + length;

  while (ptr < maxptr)
    {
      ptr = tir_cmd (abfd, ptr);
      if (ptr == 0)
	return -1;
    }

  return 0;
}

/* Text Information and Relocation Records (EOBJ$C_ETIR)
   handle etir record  */

static int
analyze_etir (abfd, ptr, length)
     bfd *abfd;
     unsigned char *ptr;
     unsigned int length;
{
  int cmd;
  unsigned char *maxptr;
  int result = 0;

#if VMS_DEBUG
  _bfd_vms_debug (3, "analyze_etir: %d bytes\n", length);
#endif

  maxptr = ptr + length;

  while (ptr < maxptr)
    {
      cmd = bfd_getl16 (ptr);
      length = bfd_getl16 (ptr + 2);
      result = etir_cmd (abfd, cmd, ptr+4);
      if (result != 0)
	break;
      ptr += length;
    }

#if VMS_DEBUG
  _bfd_vms_debug (3, "analyze_etir: = %d\n", result);
#endif

  return result;
}

/* process ETIR record

   return 0 on success, -1 on error  */

int
_bfd_vms_slurp_tir (abfd, objtype)
     bfd *abfd;
     int objtype;
{
  int result;

#if VMS_DEBUG
  _bfd_vms_debug (2, "TIR/ETIR\n");
#endif

  switch (objtype)
    {
      case EOBJ_S_C_ETIR:
	PRIV(vms_rec) += 4;	/* skip type, size */
	PRIV(rec_size) -= 4;
	result = analyze_etir (abfd, PRIV(vms_rec), PRIV(rec_size));
	break;
      case OBJ_S_C_TIR:
	PRIV(vms_rec) += 1;	/* skip type */
	PRIV(rec_size) -= 1;
	result = analyze_tir (abfd, PRIV(vms_rec), PRIV(rec_size));
	break;
      default:
	result = -1;
	break;
    }

  return result;
}

/* process EDBG record
   return 0 on success, -1 on error

   not implemented yet  */

int
_bfd_vms_slurp_dbg (abfd, objtype)
     bfd *abfd;
     int objtype ATTRIBUTE_UNUSED;
{
#if VMS_DEBUG
  _bfd_vms_debug (2, "DBG/EDBG\n");
#endif

  abfd->flags |= (HAS_DEBUG | HAS_LINENO);
  return 0;
}

/* process ETBT record
   return 0 on success, -1 on error

   not implemented yet  */

int
_bfd_vms_slurp_tbt (abfd, objtype)
     bfd *abfd ATTRIBUTE_UNUSED;
     int objtype ATTRIBUTE_UNUSED;
{
#if VMS_DEBUG
  _bfd_vms_debug (2, "TBT/ETBT\n");
#endif

  return 0;
}

/* process LNK record
   return 0 on success, -1 on error

   not implemented yet  */

int
_bfd_vms_slurp_lnk (abfd, objtype)
     bfd *abfd ATTRIBUTE_UNUSED;
     int objtype ATTRIBUTE_UNUSED;
{
#if VMS_DEBUG
  _bfd_vms_debug (2, "LNK\n");
#endif

  return 0;
}

/*----------------------------------------------------------------------*/
/*									*/
/*	WRITE ETIR SECTION						*/
/*									*/
/*	this is still under construction and therefore not documented	*/
/*									*/
/*----------------------------------------------------------------------*/

static void start_etir_record PARAMS ((bfd *abfd, int index, uquad offset, boolean justoffset));
static void sto_imm PARAMS ((bfd *abfd, vms_section *sptr, bfd_vma vaddr, int index));
static void end_etir_record PARAMS ((bfd *abfd));

static void
sto_imm (abfd, sptr, vaddr, index)
     bfd *abfd;
     vms_section *sptr;
     bfd_vma vaddr;
     int index;
{
  int size;
  int ssize;
  unsigned char *cptr;

#if VMS_DEBUG
  _bfd_vms_debug (8, "sto_imm %d bytes\n", sptr->size);
  _bfd_hexdump (9, sptr->contents, (int)sptr->size, (int)vaddr);
#endif

  ssize = sptr->size;
  cptr = sptr->contents;

  while (ssize > 0)
    {

      size = ssize;				/* try all the rest */

      if (_bfd_vms_output_check (abfd, size) < 0)
	{					/* doesn't fit, split ! */
	  end_etir_record (abfd);
	  start_etir_record (abfd, index, vaddr, false);
	  size = _bfd_vms_output_check (abfd, 0);	/* get max size */
	  if (size > ssize)			/* more than what's left ? */
	    size = ssize;
	}

      _bfd_vms_output_begin (abfd, ETIR_S_C_STO_IMM, -1);
      _bfd_vms_output_long (abfd, (unsigned long) (size));
      _bfd_vms_output_dump (abfd, cptr, size);
      _bfd_vms_output_flush (abfd);

#if VMS_DEBUG
      _bfd_vms_debug (10, "dumped %d bytes\n", size);
      _bfd_hexdump (10, cptr, (int)size, (int)vaddr);
#endif

      vaddr += size;
      ssize -= size;
      cptr += size;
    }

  return;
}

/*-------------------------------------------------------------------*/

/* start ETIR record for section #index at virtual addr offset.  */

static void
start_etir_record (abfd, index, offset, justoffset)
    bfd *abfd;
    int index;
    uquad offset;
    boolean justoffset;
{
  if (!justoffset)
    {
      _bfd_vms_output_begin (abfd, EOBJ_S_C_ETIR, -1);	/* one ETIR per section */
      _bfd_vms_output_push (abfd);
    }

  _bfd_vms_output_begin (abfd, ETIR_S_C_STA_PQ, -1);	/* push start offset */
  _bfd_vms_output_long (abfd, (unsigned long)index);
  _bfd_vms_output_quad (abfd, (uquad)offset);
  _bfd_vms_output_flush (abfd);

  _bfd_vms_output_begin (abfd, ETIR_S_C_CTL_SETRB, -1);	/* start = pop () */
  _bfd_vms_output_flush (abfd);

  return;
}

/* end etir record  */
static void
end_etir_record (abfd)
    bfd *abfd;
{
  _bfd_vms_output_pop (abfd);
  _bfd_vms_output_end (abfd);
}

/* write section contents for bfd abfd  */

int
_bfd_vms_write_tir (abfd, objtype)
     bfd *abfd;
     int objtype ATTRIBUTE_UNUSED;
{
  asection *section;
  vms_section *sptr;
  int nextoffset;

#if VMS_DEBUG
  _bfd_vms_debug (2, "vms_write_tir (%p, %d)\n", abfd, objtype);
#endif

  _bfd_vms_output_alignment (abfd, 4);

  nextoffset = 0;
  PRIV(vms_linkage_index) = 1;

  /* dump all other sections  */

  section = abfd->sections;

  while (section != NULL)
    {

#if VMS_DEBUG
      _bfd_vms_debug (4, "writing %d. section '%s' (%d bytes)\n", section->index, section->name, (int) (section->_raw_size));
#endif

      if (section->flags & SEC_RELOC)
	{
	  int i;

	  if ((i = section->reloc_count) <= 0)
	    {
	      (*_bfd_error_handler) (_("SEC_RELOC with no relocs in section %s"),
				     section->name);
	    }
#if VMS_DEBUG
	  else
	    {
	      arelent **rptr;
	      _bfd_vms_debug (4, "%d relocations:\n", i);
	      rptr = section->orelocation;
	      while (i-- > 0)
		{
		  _bfd_vms_debug (4, "sym %s in sec %s, value %08lx, addr %08lx, off %08lx, len %d: %s\n",
			      (*(*rptr)->sym_ptr_ptr)->name,
			      (*(*rptr)->sym_ptr_ptr)->section->name,
			      (long) (*(*rptr)->sym_ptr_ptr)->value,
			      (*rptr)->address, (*rptr)->addend,
			      bfd_get_reloc_size((*rptr)->howto),
			      (*rptr)->howto->name);
		  rptr++;
		}
	    }
#endif
	}

      if ((section->flags & SEC_HAS_CONTENTS)
	&& (! bfd_is_com_section (section)))
	{
	  bfd_vma vaddr;		/* virtual addr in section */

	  sptr = _bfd_get_vms_section (abfd, section->index);
	  if (sptr == NULL)
	    {
	      bfd_set_error (bfd_error_no_contents);
	      return -1;
	    }

	  vaddr = (bfd_vma) (sptr->offset);

	  start_etir_record (abfd, section->index, (uquad) sptr->offset,
			     false);

	  while (sptr != NULL)				/* one STA_PQ, CTL_SETRB per vms_section */
	    {

	      if (section->flags & SEC_RELOC)			/* check for relocs */
		{
		  arelent **rptr = section->orelocation;
		  int i = section->reloc_count;
		  for (;;)
		    {
		      bfd_size_type addr = (*rptr)->address;
		      bfd_size_type len = bfd_get_reloc_size ((*rptr)->howto);
		      if (sptr->offset < addr)		/* sptr starts before reloc */
			{
			  bfd_size_type before = addr - sptr->offset;
			  if (sptr->size <= before)		/* complete before */
			    {
			      sto_imm (abfd, sptr, vaddr, section->index);
			      vaddr += sptr->size;
			      break;
			    }
			  else				/* partly before */
			    {
			      int after = sptr->size - before;
			      sptr->size = before;
			      sto_imm (abfd, sptr, vaddr, section->index);
			      vaddr += sptr->size;
			      sptr->contents += before;
			      sptr->offset += before;
			      sptr->size = after;
			    }
			}
		      else if (sptr->offset == addr)	/* sptr starts at reloc */
			{
			  asymbol *sym = *(*rptr)->sym_ptr_ptr;
			  asection *sec = sym->section;

			  switch ((*rptr)->howto->type)
			    {
			    case ALPHA_R_IGNORE:
			      break;

			    case ALPHA_R_REFLONG:
			      {
				if (bfd_is_und_section (sym->section))
				  {
				    if (_bfd_vms_output_check (abfd,
								strlen((char *)sym->name))
					< 0)
				      {
					end_etir_record (abfd);
					start_etir_record (abfd,
							   section->index,
							   vaddr, false);
				      }
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STO_GBL_LW,
							    -1);
				    _bfd_vms_output_counted (abfd,
							      _bfd_vms_length_hash_symbol (abfd, sym->name, EOBJ_S_C_SYMSIZ));
				    _bfd_vms_output_flush (abfd);
				  }
				else if (bfd_is_abs_section (sym->section))
				  {
				    if (_bfd_vms_output_check (abfd, 16) < 0)
				      {
					end_etir_record (abfd);
					start_etir_record (abfd,
							   section->index,
							   vaddr, false);
				      }
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STA_LW,
							    -1);
				    _bfd_vms_output_quad (abfd,
							   (uquad)sym->value);
				    _bfd_vms_output_flush (abfd);
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STO_LW,
							    -1);
				    _bfd_vms_output_flush (abfd);
				  }
				else
				  {
				    if (_bfd_vms_output_check (abfd, 32) < 0)
				      {
					end_etir_record (abfd);
					start_etir_record (abfd,
							   section->index,
							   vaddr, false);
				      }
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STA_PQ,
							    -1);
				    _bfd_vms_output_long (abfd,
							   (unsigned long) (sec->index));
				    _bfd_vms_output_quad (abfd,
							   ((uquad) (*rptr)->addend
							    + (uquad)sym->value));
				    _bfd_vms_output_flush (abfd);
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STO_LW,
							    -1);
				    _bfd_vms_output_flush (abfd);
				  }
			      }
			      break;

			    case ALPHA_R_REFQUAD:
			      {
				if (bfd_is_und_section (sym->section))
				  {
				    if (_bfd_vms_output_check (abfd,
								strlen((char *)sym->name))
					< 0)
				      {
					end_etir_record (abfd);
					start_etir_record (abfd,
							   section->index,
							   vaddr, false);
				      }
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STO_GBL,
							    -1);
				    _bfd_vms_output_counted (abfd,
							      _bfd_vms_length_hash_symbol (abfd, sym->name, EOBJ_S_C_SYMSIZ));
				    _bfd_vms_output_flush (abfd);
				  }
				else if (bfd_is_abs_section (sym->section))
				  {
				    if (_bfd_vms_output_check (abfd, 16) < 0)
				      {
					end_etir_record (abfd);
					start_etir_record (abfd,
							   section->index,
							   vaddr, false);
				      }
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STA_QW,
							    -1);
				    _bfd_vms_output_quad (abfd,
							   (uquad)sym->value);
				    _bfd_vms_output_flush (abfd);
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STO_QW,
							    -1);
				    _bfd_vms_output_flush (abfd);
				  }
				else
				  {
				    if (_bfd_vms_output_check (abfd, 32) < 0)
				      {
					end_etir_record (abfd);
					start_etir_record (abfd,
							   section->index,
							   vaddr, false);
				      }
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STA_PQ,
							    -1);
				    _bfd_vms_output_long (abfd,
							   (unsigned long) (sec->index));
				    _bfd_vms_output_quad (abfd,
							   ((uquad) (*rptr)->addend
							    + (uquad)sym->value));
				    _bfd_vms_output_flush (abfd);
				    _bfd_vms_output_begin (abfd,
							    ETIR_S_C_STO_OFF,
							    -1);
				    _bfd_vms_output_flush (abfd);
				  }
			      }
			      break;

			    case ALPHA_R_HINT:
			      {
				int hint_size;

				hint_size = sptr->size;
				sptr->size = len;
				sto_imm (abfd, sptr, vaddr, section->index);
				sptr->size = hint_size;
#if 0
				vms_output_begin(abfd, ETIR_S_C_STO_HINT_GBL, -1);
				vms_output_long(abfd, (unsigned long) (sec->index));
				vms_output_quad(abfd, (uquad)addr);

				vms_output_counted(abfd, _bfd_vms_length_hash_symbol (abfd, sym->name, EOBJ_S_C_SYMSIZ));
				vms_output_flush(abfd);
#endif
			      }
			      break;
			    case ALPHA_R_LINKAGE:
			      {
				if (_bfd_vms_output_check (abfd, 64) < 0)
				  {
				    end_etir_record (abfd);
				    start_etir_record (abfd, section->index,
						       vaddr, false);
				  }
				_bfd_vms_output_begin (abfd,
							ETIR_S_C_STC_LP_PSB,
							-1);
				_bfd_vms_output_long (abfd,
						       (unsigned long)PRIV(vms_linkage_index));
				PRIV(vms_linkage_index) += 2;
				_bfd_vms_output_counted (abfd,
							  _bfd_vms_length_hash_symbol (abfd, sym->name, EOBJ_S_C_SYMSIZ));
				_bfd_vms_output_byte (abfd, 0);
				_bfd_vms_output_flush (abfd);
			      }
			      break;

			    case ALPHA_R_CODEADDR:
			      {
				if (_bfd_vms_output_check (abfd,
							    strlen((char *)sym->name))
				    < 0)
				  {
				    end_etir_record (abfd);
				    start_etir_record (abfd,
						       section->index,
						       vaddr, false);
				  }
				_bfd_vms_output_begin (abfd,
							ETIR_S_C_STO_CA,
							-1);
				_bfd_vms_output_counted (abfd,
							  _bfd_vms_length_hash_symbol (abfd, sym->name, EOBJ_S_C_SYMSIZ));
				_bfd_vms_output_flush (abfd);
			      }
			      break;

			    default:
			      (*_bfd_error_handler) (_("Unhandled relocation %s"),
						     (*rptr)->howto->name);
			      break;
			    }

			  vaddr += len;

			  if (len == sptr->size)
			    {
			      break;
			    }
			  else
			    {
			      sptr->contents += len;
			      sptr->offset += len;
			      sptr->size -= len;
			      i--;
			      rptr++;
			    }
			}
		      else					/* sptr starts after reloc */
			{
			  i--;				/* check next reloc */
			  rptr++;
			}

		      if (i==0)				/* all reloc checked */
			{
			  if (sptr->size > 0)
			    {
			      sto_imm (abfd, sptr, vaddr, section->index);	/* dump rest */
			      vaddr += sptr->size;
			    }
			  break;
			}
		    } /* for (;;) */
		} /* if SEC_RELOC */
	      else						/* no relocs, just dump */
		{
		  sto_imm (abfd, sptr, vaddr, section->index);
		  vaddr += sptr->size;
		}

	      sptr = sptr->next;

	    } /* while (sptr != 0) */

	  end_etir_record (abfd);

	} /* has_contents */

      section = section->next;
    }

  _bfd_vms_output_alignment(abfd, 2);
  return 0;
}

/* write traceback data for bfd abfd  */

int
_bfd_vms_write_tbt (abfd, objtype)
     bfd *abfd ATTRIBUTE_UNUSED;
     int objtype ATTRIBUTE_UNUSED;
{
#if VMS_DEBUG
  _bfd_vms_debug (2, "vms_write_tbt (%p, %d)\n", abfd, objtype);
#endif

  return 0;
}

/* write debug info for bfd abfd  */

int
_bfd_vms_write_dbg (abfd, objtype)
     bfd *abfd ATTRIBUTE_UNUSED;
     int objtype ATTRIBUTE_UNUSED;
{
#if VMS_DEBUG
  _bfd_vms_debug (2, "vms_write_dbg (%p, objtype)\n", abfd, objtype);
#endif

  return 0;
}
