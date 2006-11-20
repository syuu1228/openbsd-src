/*	$OpenBSD: src/usr.sbin/ntpd/sensors.c,v 1.26 2006/11/20 20:58:47 henning Exp $ */

/*
 * Copyright (c) 2006 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/sensors.h>
#include <sys/sysctl.h>
#include <sys/device.h>
#include <sys/hotplug.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ntpd.h"

#define SENSORS_MAX		255
#define	_PATH_DEV_HOTPLUG	"/dev/hotplug"

void	sensor_probe(int);
void	sensor_add(struct sensor *);
void	sensor_remove(struct ntp_sensor *);
void	sensor_update(struct ntp_sensor *);

struct ntpd_conf *conf;

void
sensor_init(struct ntpd_conf *c)
{
	conf = c;
	TAILQ_INIT(&conf->ntp_sensors);
}

void
sensor_scan(void)
{
	int		i;

	for (i = 0; i < SENSORS_MAX; i++)
		sensor_probe(i);
}

void
sensor_probe(int id)
{
	int		mib[3];
	size_t		len;
	struct sensor	sensor;

	mib[0] = CTL_HW;
	mib[1] = HW_SENSORS;
	mib[2] = id;

	len = sizeof(sensor);
	if (sysctl(mib, 3, &sensor, &len, NULL, 0) == -1) {
		if (errno != ENOENT)
			log_warn("sensor_probe sysctl");
		return;
	}

	if (sensor.type == SENSOR_TIMEDELTA)
		sensor_add(&sensor);
}

void
sensor_add(struct sensor *sensor)
{
	struct ntp_sensor	*s;
	struct ntp_conf_sensor	*cs;

	/* check wether it is already there */
	TAILQ_FOREACH(s, &conf->ntp_sensors, entry)
		if (!strcmp(s->device, sensor->device))
			return;

	/* check wether it is requested in the config file */
	for (cs = TAILQ_FIRST(&conf->ntp_conf_sensors); cs != NULL &&
	    strcmp(cs->device, sensor->device) && strcmp(cs->device, "*");
	    cs = TAILQ_NEXT(cs, entry))
		; /* nothing */
	if (cs == NULL)
		return;

	if ((s = calloc(1, sizeof(*s))) == NULL)
		fatal("sensor_add calloc");

	s->next = getmonotime();
	s->weight = cs->weight;
	if ((s->device = strdup(sensor->device)) == NULL)
		fatal("sensor_add strdup");
	s->sensorid = sensor->num;

	TAILQ_INSERT_TAIL(&conf->ntp_sensors, s, entry);

	log_debug("sensor %s added", s->device);
}

void
sensor_remove(struct ntp_sensor *s)
{
	TAILQ_REMOVE(&conf->ntp_sensors, s, entry);
	free(s->device);
	free(s);
}

void
sensor_query(struct ntp_sensor *s)
{
	struct sensor	 sensor;
	u_int32_t	 refid;
	int		 mib[3];
	size_t		 len;

	s->next = getmonotime() + SENSOR_QUERY_INTERVAL;

	/* rcvd is walltime here, monotime in client.c. not used elsewhere */
	if (s->update.rcvd < time(NULL) - SENSOR_DATA_MAXAGE)
		s->update.good = 0;

	mib[0] = CTL_HW;
	mib[1] = HW_SENSORS;
	mib[2] = s->sensorid;
	len = sizeof(sensor);
	if (sysctl(mib, 3, &sensor, &len, NULL, 0) == -1) {
		if (errno == ENOENT)
			sensor_remove(s);
		else
			log_warn("sensor_query sysctl");
		return;
	}

	if (sensor.flags & SENSOR_FINVALID ||
	    sensor.status != SENSOR_S_OK)
		return;

	if (sensor.type != SENSOR_TIMEDELTA ||
	    strcmp(sensor.device, s->device)) {
		sensor_remove(s);
		return;
	}

	if (sensor.tv.tv_sec == s->last)	/* already seen */
		return;

	s->last = sensor.tv.tv_sec;
	memcpy(&refid, "HARD", sizeof(refid));
	s->offsets[s->shift].offset = (0 - (float)sensor.value / 1000000000.0) -
	    getoffset();
	s->offsets[s->shift].rcvd = sensor.tv.tv_sec;
	s->offsets[s->shift].good = 1;

	s->offsets[s->shift].status.refid = htonl(refid);
	s->offsets[s->shift].status.stratum = 0;	/* increased when sent out */
	s->offsets[s->shift].status.rootdelay = 0;
	s->offsets[s->shift].status.rootdispersion = 0;
	s->offsets[s->shift].status.reftime = sensor.tv.tv_sec;
	s->offsets[s->shift].status.synced = 1;

	log_debug("sensor %s: offset %f", s->device,
	    s->offsets[s->shift].offset);

	if (++s->shift >= SENSOR_OFFSETS) {
		s->shift = 0;
		sensor_update(s);
	}

}

void
sensor_update(struct ntp_sensor *s)
{
	struct ntp_offset	**offsets;
	int			  i;

	if ((offsets = calloc(SENSOR_OFFSETS, sizeof(struct ntp_offset *))) ==
	    NULL)
		fatal("calloc sensor_update");

	for (i = 0; i < SENSOR_OFFSETS; i++)
		offsets[i] = &s->offsets[i];

	qsort(offsets, SENSOR_OFFSETS, sizeof(struct ntp_offset *),
	    offset_compare);

	i = SENSOR_OFFSETS / 2;
	memcpy(&s->update, offsets[i], sizeof(s->update));
	if (SENSOR_OFFSETS % 2 == 0) {
		s->update.offset =
		    (offsets[i - 1]->offset + offsets[i]->offset) / 2;
	}
	free(offsets);

	log_debug("sensor update %s: offset %f", s->device, s->update.offset);
	priv_adjtime();
}

int
sensor_hotplugfd(void)
{
#ifdef notyet
	int	fd, flags;

	if ((fd = open(_PATH_DEV_HOTPLUG, O_RDONLY, 0)) == -1) {
		log_warn("open %s", _PATH_DEV_HOTPLUG);
		return (-1);
	}

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		fatal("fnctl F_GETFL");
	flags |= O_NONBLOCK;
	if ((flags = fcntl(fd, F_SETFL, flags)) == -1)
		fatal("fnctl F_SETFL");

	return (fd);
#else
	return (-1);
#endif
}

void
sensor_hotplugevent(int fd)
{
	struct hotplug_event	he;
	ssize_t			n;

	do {
		if ((n = read(fd, &he, sizeof(he))) == -1 &&
		    errno != EINTR && errno != EAGAIN)
			fatal("sensor_hotplugevent read");

		if (n == sizeof(he))
			switch (he.he_type) {
			case HOTPLUG_DEVAT:
				if (he.he_devclass == DV_DULL &&
				    !strcmp(he.he_devname, "sensor"))
					sensor_scan();
				break;
			default:		/* ignore */
				break;
			}
		else if (n > 0)
			fatal("sensor_hotplugevent: short read");
	} while (n > 0 || (n == -1 && errno == EINTR));
}
