/*	$OpenBSD: src/sys/arch/sparc64/dev/lpt_ebus.c,v 1.6 2002/07/18 02:20:17 jason Exp $	*/
/*	$NetBSD: lpt_ebus.c,v 1.8 2002/03/01 11:51:00 martin Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * NS Super I/O PC87332VLJ "lpt" to ebus attachment
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>

#include <sparc64/dev/ebusreg.h>
#include <sparc64/dev/ebusvar.h>

#include <dev/ic/lptvar.h>

struct lpt_ebus_softc {
	struct lpt_softc sc_lpt;
	bus_space_handle_t sc_ctrl;
};

int	lpt_ebus_match(struct device *, void *, void *);
void	lpt_ebus_attach(struct device *, struct device *, void *);

struct cfattach lpt_ebus_ca = {
	sizeof(struct lpt_ebus_softc), lpt_ebus_match, lpt_ebus_attach
};

int
lpt_ebus_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct ebus_attach_args *ea = aux;

	if (!strcmp(ea->ea_name, "ecpp") ||
	    !strcmp(ea->ea_name, "parallel"))
		return (1);
	return (0);
}

void
lpt_ebus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct lpt_ebus_softc *sc = (void *)self;
	struct ebus_attach_args *ea = aux;

	sc->sc_lpt.sc_iot = ea->ea_bustag;

	if (ebus_bus_map(sc->sc_lpt.sc_iot, 0,
	    EBUS_PADDR_FROM_REG(&ea->ea_regs[0]), ea->ea_regs[0].size,
	    BUS_SPACE_MAP_LINEAR, 0, &sc->sc_lpt.sc_ioh) != 0) {
		printf(": can't map register space\n");
                return;
	}

	if (ebus_bus_map(sc->sc_lpt.sc_iot, 0,
	    EBUS_PADDR_FROM_REG(&ea->ea_regs[1]), ea->ea_regs[1].size,
	    BUS_SPACE_MAP_LINEAR, 0, &sc->sc_ctrl) != 0) {
		printf(": can't map control space\n");
		bus_space_unmap(sc->sc_lpt.sc_iot, sc->sc_lpt.sc_ioh,
		    ea->ea_regs[0].size);
		return;
	}

	sc->sc_lpt.sc_flags |= LPT_POLLED;
	printf(": polled");

	lpt_attach_common(&sc->sc_lpt);
}
