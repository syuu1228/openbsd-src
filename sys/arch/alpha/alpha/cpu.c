/*	$OpenBSD: src/sys/arch/alpha/alpha/cpu.c,v 1.5 1996/10/30 22:38:01 niklas Exp $	*/
/*	$NetBSD: cpu.c,v 1.12 1996/10/13 02:59:26 christos Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

/* Definition of the driver for autoconfig. */
int	cpumatch __P((struct device *, void *, void *));
void	cpuattach __P((struct device *, struct device *, void *));

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

struct cfdriver cpu_cd = {
	NULL, "cpu", DV_DULL
};

int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	void *cfdata;
	void *aux;
{
	struct confargs *ca = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return (0);

	return (1);
}

void
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{
        struct pcs *p;
	char *cpu_major[] = {
		"UNKNOWN MAJOR TYPE (0)",
		"EV3",				/* PCS_PROC_EV3 */
		"21064 (EV4)",			/* PCS_PROC_EV4 */
		"Simulator",			/* PCS_PROC_SIMULATOR */
		"21066/21068 (LCA4)",		/* PCS_PROC_LCA4 */
		"21164 (EV5)",			/* PCS_PROC_EV5 */
		"21064A (EV45)",		/* PCS_PROC_EV45 */
	};
	int ncpu_major = sizeof(cpu_major) / sizeof(cpu_major[0]);
	char *dc21064_cpu_minor[] = {
		"Pass 2 or 2.1",
		"Pass 3",
	};
	int ndc21064_cpu_minor =
	    sizeof(dc21064_cpu_minor) / sizeof(dc21064_cpu_minor[0]);
	u_int32_t major, minor;
	int needcomma;

        p = (struct pcs*)((char *)hwrpb + hwrpb->rpb_pcs_off +
	    (dev->dv_unit * hwrpb->rpb_pcs_size));
	printf(": ");

	major = (p->pcs_proc_type & PCS_PROC_MAJOR) >> PCS_PROC_MAJORSHIFT;
	minor = (p->pcs_proc_type & PCS_PROC_MINOR) >> PCS_PROC_MINORSHIFT;

	if (major < ncpu_major)
		printf("%s", cpu_major[major]);
	else
		printf("UNKNOWN MAJOR TYPE (%d)", major);

	printf(", ");

	switch (major) {
	case PCS_PROC_EV4:
		if (minor < ndc21064_cpu_minor)
			printf("%s", dc21064_cpu_minor[minor]);
		else
			printf("UNKNOWN MINOR TYPE (%d)", minor);
		break;

	case PCS_PROC_EV45:
	case PCS_PROC_EV5:
		printf("Pass %d", minor + 1);
		break;

	default:
		printf("UNKNOWN MINOR TYPE (%d)", minor);
	}

	if (p->pcs_proc_revision[0] != 0) {		/* XXX bad test? */
		printf(", ");

		printf("Revision %c%c%c%c", p->pcs_proc_revision[0],
		    p->pcs_proc_revision[1], p->pcs_proc_revision[2],
		    p->pcs_proc_revision[3]);
	}

	printf("\n");

	if (p->pcs_proc_var != 0) {
		printf("cpu%d: ", dev->dv_unit);

		needcomma = 0;
		if (p->pcs_proc_var & PCS_VAR_VAXFP) {
			printf("VAX FP support");
			needcomma = 1;
		}
		if (p->pcs_proc_var & PCS_VAR_IEEEFP) {
			printf("%sIEEE FP support", needcomma ? ", " : "");
			needcomma = 1;
		}
		if (p->pcs_proc_var & PCS_VAR_PE) {
			printf("%sPrimary Eligible", needcomma ? ", " : "");
			needcomma = 1;
		}
		if (p->pcs_proc_var & PCS_VAR_RESERVED)
			printf("%sreserved bits: 0x%lx", needcomma ? ", " : "",
			    p->pcs_proc_var & PCS_VAR_RESERVED);
		printf("\n");
	}

	/*
	 * Though we could (should?) attach the LCA's PCI
	 * bus here there is no good reason to do so, and
	 * the bus attachment code is easier to understand
	 * and more compact if done the 'normal' way.
	 */
}
