/*	$OpenBSD: src/sys/ddb/db_run.h,v 1.5 1997/07/19 22:28:57 niklas Exp $	*/
/*	$NetBSD: db_run.h,v 1.3 1996/02/05 01:57:14 christos Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 * 	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#ifndef	_DDB_DB_RUN_
#define	_DDB_DB_RUN_

/*
 * Commands to run process.
 */
int		db_inst_count;
int		db_load_count;
int		db_store_count;

boolean_t db_stop_at_pc __P((db_regs_t *, boolean_t *));
void db_restart_at_pc __P((db_regs_t *, boolean_t));
void db_single_step __P((db_regs_t *));
#ifndef db_set_single_step
void db_set_single_step __P((db_regs_t *));
#endif
#ifndef db_clear_single_step
void db_clear_single_step __P((db_regs_t *));
#endif
void db_single_step_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_trace_until_call_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_trace_until_matching_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_continue_cmd __P((db_expr_t, int, db_expr_t, char *));

#ifdef SOFTWARE_SSTEP
/*
 * I've seen this defined to (0) when it is not needed and then the proto will
 * not be correct, so skip it then.
 */
#ifndef getreg_val
extern register_t getreg_val __P((db_regs_t *, int));
#endif
#endif /* SOFTWARE_SSTEP */

#endif	_DDB_DB_RUN_
