/*	$OpenBSD: src/sys/dev/i2c/adm1031.c,v 1.5 2006/04/10 00:57:23 deraadt Exp $	*/

/*
 * Copyright (c) 2005 Theo de Raadt
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
#include <sys/sensors.h>

#include <dev/i2c/i2cvar.h>

/* adm 1031 registers */
#define ADM1031_INT_TEMP	0x0a
#define ADM1031_EXT_TEMP	0x0b
#define ADM1031_EXT2_TEMP	0x0c
#define ADM1031_FAN		0x08
#define ADM1031_FANC		0x20
#define ADM1031_FAN2		0x09
#define ADM1031_FAN2C		0x21
#define  ADM1024_FANC_VAL(x)	(x >> 6)

/* Sensors */
#define ADMTT_INT		0
#define ADMTT_EXT		1
#define ADMTT_EXT2		2
#define ADMTT_FAN		3
#define ADMTT_FAN2		4
#define ADMTT_NUM_SENSORS	5

struct admtt_softc {
	struct device	sc_dev;
	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;
	int		sc_fanmul;

	struct sensor	sc_sensor[ADMTT_NUM_SENSORS];
};

int	admtt_match(struct device *, void *, void *);
void	admtt_attach(struct device *, struct device *, void *);
void	admtt_refresh(void *);

struct cfattach admtt_ca = {
	sizeof(struct admtt_softc), admtt_match, admtt_attach
};

struct cfdriver admtt_cd = {
	NULL, "admtt", DV_DULL
};

int
admtt_match(struct device *parent, void *match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "adm1031") == 0)
		return (1);
	return (0);
}

void
admtt_attach(struct device *parent, struct device *self, void *aux)
{
	struct admtt_softc *sc = (struct admtt_softc *)self;
	struct i2c_attach_args *ia = aux;
	u_int8_t cmd, data;
	int i;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	cmd = ADM1031_FANC;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof cmd, &data, sizeof data, 0)) {
		printf(", unable to read fan setting\n");
		return;
	}
	sc->sc_fanmul = 11250/8 * (1 << ADM1024_FANC_VAL(data)) * 60;

	/* Initialize sensor data. */
	for (i = 0; i < ADMTT_NUM_SENSORS; i++)
		strlcpy(sc->sc_sensor[i].device, sc->sc_dev.dv_xname,
		    sizeof(sc->sc_sensor[i].device));

	sc->sc_sensor[ADMTT_INT].type = SENSOR_TEMP;
	strlcpy(sc->sc_sensor[ADMTT_INT].desc, "Internal Temp",
	    sizeof(sc->sc_sensor[ADMTT_INT].desc));

	sc->sc_sensor[ADMTT_EXT].type = SENSOR_TEMP;
	strlcpy(sc->sc_sensor[ADMTT_EXT].desc, "External1 Temp",
	    sizeof(sc->sc_sensor[ADMTT_EXT].desc));

	sc->sc_sensor[ADMTT_EXT2].type = SENSOR_TEMP;
	strlcpy(sc->sc_sensor[ADMTT_EXT2].desc, "External2 Temp",
	    sizeof(sc->sc_sensor[ADMTT_EXT2].desc));

	sc->sc_sensor[ADMTT_FAN].type = SENSOR_FANRPM;
	strlcpy(sc->sc_sensor[ADMTT_FAN].desc, "Fan1",
	    sizeof(sc->sc_sensor[ADMTT_FAN].desc));

	sc->sc_sensor[ADMTT_FAN2].type = SENSOR_FANRPM;
	strlcpy(sc->sc_sensor[ADMTT_FAN2].desc, "Fan2",
	    sizeof(sc->sc_sensor[ADMTT_FAN2].desc));

	if (sensor_task_register(sc, admtt_refresh, 5)) {
		printf(", unable to register update task\n");
		return;
	}

	for (i = 0; i < ADMTT_NUM_SENSORS; i++)
		sensor_add(&sc->sc_sensor[i]);

	printf("\n");
}

void
admtt_refresh(void *arg)
{
	struct admtt_softc *sc = arg;
	u_int8_t cmd, data;

	iic_acquire_bus(sc->sc_tag, 0);

	cmd = ADM1031_INT_TEMP;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof cmd, &data, sizeof data, 0) == 0)
		sc->sc_sensor[ADMTT_INT].value = 273150000 + 1000000 * data;

	cmd = ADM1031_EXT_TEMP;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof cmd, &data, sizeof data, 0) == 0)
		sc->sc_sensor[ADMTT_EXT].value = 273150000 + 1000000 * data;

	cmd = ADM1031_EXT2_TEMP;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof cmd, &data, sizeof data, 0) == 0)
		sc->sc_sensor[ADMTT_EXT2].value = 273150000 + 1000000 * data;

	cmd = ADM1031_FAN;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof cmd, &data, sizeof data, 0) == 0) {
		if (data == 0)
			sc->sc_sensor[ADMTT_FAN].flags |= SENSOR_FINVALID;
		else {
			sc->sc_sensor[ADMTT_FAN].value =
			    sc->sc_fanmul / (2 * (int)data);
			sc->sc_sensor[ADMTT_FAN].flags &= ~SENSOR_FINVALID;
		}
	}

	cmd = ADM1031_FAN2;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, sizeof cmd, &data, sizeof data, 0) == 0) {
		if (data == 0)
			sc->sc_sensor[ADMTT_FAN2].flags |= SENSOR_FINVALID;
		else {
			sc->sc_sensor[ADMTT_FAN2].value =
			    sc->sc_fanmul / (2 * (int)data);
			sc->sc_sensor[ADMTT_FAN2].flags &= ~SENSOR_FINVALID;
		}
	}

	iic_release_bus(sc->sc_tag, 0);
}
