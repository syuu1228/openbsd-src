/*	$OpenBSD: src/sys/arch/zaurus/dev/zaurus_ssp.c,v 1.1 2005/01/26 06:34:54 uwe Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <zaurus/dev/zaurus_sspvar.h>

#define GPIO_TG_CS_C3000	53
#define GPIO_MAX1111_CS_C3000	20
#define GPIO_ADS7846_CS_C3000	14	/* SSP SFRM */

#define	SSCR0_LZ9JG18		0x01ab
#define SSCR0_MAX1111		0x0387

struct zssp_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

int	zssp_match(struct device *, void *, void *);
void	zssp_attach(struct device *, struct device *, void *);

struct cfattach zssp_ca = {
	sizeof (struct zssp_softc), zssp_match, zssp_attach
};

struct cfdriver zssp_cd = {
	NULL, "zssp", DV_DULL
};

int
zssp_match(struct device *parent, void *match, void *aux)
{
	return 1;
}

void
zssp_attach(struct device *parent, struct device *self, void *aux)
{
	struct zssp_softc *sc = (struct zssp_softc *)self;

	sc->sc_iot = &pxa2x0_bs_tag;
	if (bus_space_map(sc->sc_iot, PXA2X0_SSP1_BASE, PXA2X0_SSP_SIZE,
	    0, &sc->sc_ioh))
		panic("can't map %s", sc->sc_dev.dv_xname);

	pxa2x0_clkman_config(CKEN_SSP, 1);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, SSCR0_LZ9JG18);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR1, 0);

	pxa2x0_gpio_set_function(GPIO_TG_CS_C3000, GPIO_OUT|GPIO_SET);
	pxa2x0_gpio_set_function(GPIO_MAX1111_CS_C3000, GPIO_OUT|GPIO_SET);
	pxa2x0_gpio_set_function(GPIO_ADS7846_CS_C3000, GPIO_OUT|GPIO_SET);

	printf("\n");
}

int
zssp_read_max1111(u_int32_t cmd)
{
	struct	zssp_softc *sc;
	int	voltage[2];
	int	i;

	if (zssp_cd.cd_ndevs < 1 || zssp_cd.cd_devs[0] == NULL) {
		printf("zssp_read_max1111: not configured\n");
		return 0;
	}
	sc = (struct zssp_softc *)zssp_cd.cd_devs[0];

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, SSCR0_MAX1111);

	pxa2x0_gpio_set_bit(GPIO_TG_CS_C3000);
	pxa2x0_gpio_set_bit(GPIO_ADS7846_CS_C3000);
	pxa2x0_gpio_clear_bit(GPIO_MAX1111_CS_C3000);

	delay(1);

	/* Send the command word and read a dummy word back. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR, cmd);
	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
	    & SSSR_TNF) != SSSR_TNF)
		/* poll */;
	/* XXX is this delay necessary? */
	delay(1);
	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
	    & SSSR_RNE) != SSSR_RNE)
		/* poll */;
	i = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR);

	for (i = 0; i < 2; i++) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR, 0);
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_TNF) != SSSR_TNF)
			/* poll */;
		/* XXX again, is this delay necessary? */
		delay(1);
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_RNE) != SSSR_RNE)
			/* poll */;
		voltage[i] = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    SSP_SSDR);         
	}

	pxa2x0_gpio_set_bit(GPIO_TG_CS_C3000);
	pxa2x0_gpio_set_bit(GPIO_ADS7846_CS_C3000);
	pxa2x0_gpio_set_bit(GPIO_MAX1111_CS_C3000);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);

	/* XXX no idea what this means, but it's what Linux would do. */
	if ((voltage[0] & 0xc0) != 0 || (voltage[1] & 0x3f) != 0)
		voltage[0] = -1;
	else
		voltage[0] = ((voltage[0] << 2) & 0xfc) |
		    ((voltage[1] >> 6) & 0x03);

	return voltage[0];
}
