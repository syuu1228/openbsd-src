/*	$OpenBSD: src/sys/arch/hppa/dev/power.c,v 1.1 2003/08/20 20:53:31 mickey Exp $	*/

/*
 * Copyright (c) 2003 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/kthread.h>

#include <machine/reg.h>
#include <machine/autoconf.h>

#include <hppa/dev/cpudevs.h>

struct power_softc {
	struct device sc_dev;

	struct proc *sc_thread;
	void (*sc_kicker)(void *);

	int sc_dr_cnt;
};

int	powermatch(struct device *, void *, void *);
void	powerattach(struct device *, struct device *, void *);

struct cfattach power_ca = {
	sizeof(struct power_softc), powermatch, powerattach
};

struct cfdriver power_cd = {
	NULL, "power", DV_DULL
};

void power_thread_create(void *v);
void power_thread_dr(void *v);

int
powermatch(struct device *parent, void *cfdata, void *aux)
{
	struct cfdata *cf = cfdata;
	struct confargs *ca = aux;

	if (cf->cf_unit > 0 && !strcmp(ca->ca_name, "power"))
		return (0);

	return (1);
}

void
powerattach(struct device *parent, struct device *self, void *aux)
{
	struct power_softc *sc = (struct power_softc *)self;

	switch (cpu_hvers) {
	case HPPA_BOARD_HP712_60:
	case HPPA_BOARD_HP712_80:
	case HPPA_BOARD_HP712_100:
	case HPPA_BOARD_HP712_120:
		sc->sc_kicker = power_thread_dr;
		break;
	default:
		break;
	}

	printf("\n");

	if (sc->sc_kicker)
		kthread_create_deferred(power_thread_create, sc);
}

void
power_thread_create(void *v)
{
	struct power_softc *sc = v;

	if (kthread_create(sc->sc_kicker, sc, &sc->sc_thread,
	    "powerbutton", 0))
		printf("WARNING: failed to create kernel power thread\n");
}

void
power_thread_dr(void *v)
{
	struct power_softc *sc = v;
	u_int32_t r;

	for (;;) {
		mfcpu(DR0_PCXL_SHINT_EN, r);	/* XXX don't ask */
		if (r & 0x80000000)
			sc->sc_dr_cnt = 0;
		else
			sc->sc_dr_cnt++;

		/*
		 * the bit is undampened straight wire from the power
		 * switch and thus we have do dampen it ourselves.
		 */
		if (sc->sc_dr_cnt == hz / 10)
			boot(RB_POWERDOWN | RB_HALT);

		tsleep(v, PWAIT, "drpower", 10);
	}
}
