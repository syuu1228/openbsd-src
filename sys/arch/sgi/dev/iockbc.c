/*	$OpenBSD: src/sys/arch/sgi/dev/iockbc.c,v 1.1 2009/11/10 15:50:09 jsing Exp $	*/
/*
 * Copyright (c) 2006, 2007, 2009 Joel Sing <jsing@openbsd.org>
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

/*
 * Derived from sys/dev/ic/pckbc.c under the following terms:
 * $NetBSD: pckbc.c,v 1.5 2000/06/09 04:58:35 soda Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for IOC3 PS/2 Controllers (iockbc)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/timeout.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <mips64/archtype.h>

#include <sgi/localbus/crimebus.h>
#include <sgi/localbus/macebus.h>
#include <sgi/pci/iocreg.h>
#include <sgi/pci/iocvar.h>

#include <dev/ic/pckbcvar.h>
#include <dev/pckbc/pckbdvar.h>

const char *iockbc_slot_names[] = { "kbd", "mouse" };

#define KBC_DEVCMD_ACK 0xfa
#define KBC_DEVCMD_RESEND 0xfe

/* #define IOCKBC_DEBUG */
#ifdef IOCKBC_DEBUG
#define DPRINTF(x...)           do { printf(x); } while(0)
#else
#define DPRINTF(x...)
#endif

struct iockbc_softc {
	struct pckbc_softc sc_pckbc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t rx[PCKBC_NSLOTS];
	bus_addr_t tx[PCKBC_NSLOTS];
};

int	iockbc_match(struct device *, void *, void *);
void	iockbc_attach(struct device *, struct device *, void *);

struct cfattach iockbc_ca = {
	sizeof(struct iockbc_softc), iockbc_match, iockbc_attach
};

struct cfdriver iockbc_cd = {
	NULL, "iockbc", DV_DULL
};

/* Descriptor for one device command. */
struct pckbc_devcmd {
	TAILQ_ENTRY(pckbc_devcmd) next;
	int flags;
#define KBC_CMDFLAG_SYNC 1 /* Give descriptor back to caller. */
#define KBC_CMDFLAG_SLOW 2
	u_char cmd[4];
	int cmdlen, cmdidx, retries;
	u_char response[4];
	int status, responselen, responseidx;
};

/* Data per slave device. */
struct pckbc_slotdata {
	int polling; /* Don't read data port in interrupt handler. */
	TAILQ_HEAD(, pckbc_devcmd) cmdqueue; /* Active commands. */
	TAILQ_HEAD(, pckbc_devcmd) freequeue; /* Free commands. */
#define NCMD 5
	struct pckbc_devcmd cmds[NCMD];
	int rx_queue[3];
	int rx_index;
};

#define CMD_IN_QUEUE(q) (TAILQ_FIRST(&(q)->cmdqueue) != NULL)

static int iockbc_console;

void	iockbc_start(struct pckbc_internal *, pckbc_slot_t);
int	iockbc_attach_slot(struct iockbc_softc *, pckbc_slot_t);
void	iockbc_init_slotdata(struct pckbc_slotdata *);
int	iockbc_submatch(struct device *, void *, void *);
int	iockbcprint(void *, const char *);
int	iockbcintr(void *);
int	iockbcintr_internal(struct pckbc_internal *, struct pckbc_softc *);
void	iockbc_cleanqueue(struct pckbc_slotdata *);
void	iockbc_cleanup(void *);
void	iockbc_poll(void *);
int	iockbc_cmdresponse(struct pckbc_internal *, pckbc_slot_t, u_char);
int	iockbc_poll_read(struct pckbc_internal *, pckbc_slot_t);
int	iockbc_poll_write(struct pckbc_internal *, pckbc_slot_t, int);
void	iockbc_process_input(struct pckbc_softc *, struct pckbc_internal *,
	    int, uint);

int
iockbc_match(struct device *parent, void *cf, void *aux)
{
	/*
	 * We expect ioc NOT to attach us on if there are no PS/2 ports.
	 */
	return 1;
}

void
iockbc_init_slotdata(struct pckbc_slotdata *q)
{
	int i;
	TAILQ_INIT(&q->cmdqueue);
	TAILQ_INIT(&q->freequeue);

	for (i = 0; i < NCMD; i++)
		TAILQ_INSERT_TAIL(&q->freequeue, &(q->cmds[i]), next);

	q->polling = 0;
	q->rx_index = -1;
}

int
iockbcprint(void *aux, const char *pnp)
{
	struct pckbc_attach_args *pa = aux;

	if (!pnp)
		printf(" (%s slot)", iockbc_slot_names[pa->pa_slot]);

	return (QUIET);
}

int
iockbc_submatch(struct device *parent, void *match, void *aux)
{
	struct cfdata *cf = match;
	struct pckbc_attach_args *pa = aux;

	if (cf->cf_loc[PCKBCCF_SLOT] != PCKBCCF_SLOT_DEFAULT &&
	    cf->cf_loc[PCKBCCF_SLOT] != pa->pa_slot)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
iockbc_attach_slot(struct iockbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_softc *isc = &sc->sc_pckbc;
	struct pckbc_internal *t = isc->id;
	struct pckbc_attach_args pa;
	int found;

	if (!t->t_slotdata[slot]) {
		t->t_slotdata[slot] = malloc(sizeof(struct pckbc_slotdata),
		    M_DEVBUF, M_NOWAIT);

		if (t->t_slotdata[slot] == NULL) {
			printf("Failed to allocate slot data!\n");
			return 0;
		}
		iockbc_init_slotdata(t->t_slotdata[slot]);
	}

	pa.pa_tag = t;
	pa.pa_slot = slot;
	found = (config_found_sm((struct device *)sc, &pa,
	    iockbcprint, iockbc_submatch) != NULL);

	return (found);
}

void
iockbc_attach(struct device *parent, struct device *self, void *aux)
{
	struct iockbc_softc *isc = (void*)self;
	struct ioc_attach_args *iaa = aux;
	struct pckbc_softc *sc = &isc->sc_pckbc;
	struct pckbc_internal *t;

	/*
	 * For some reason keyboard and mouse ports are inverted on
	 * Fuel (and probably Origin 300 as well).
	 */

	if (sys_config.system_type == SGI_IP35) {
		isc->rx[PCKBC_KBD_SLOT] = IOC3_KBC_AUX_RX;
		isc->rx[PCKBC_AUX_SLOT] = IOC3_KBC_KBD_RX;
		isc->tx[PCKBC_KBD_SLOT] = IOC3_KBC_AUX_TX;
		isc->tx[PCKBC_AUX_SLOT] = IOC3_KBC_KBD_TX;
	} else {
		isc->rx[PCKBC_KBD_SLOT] = IOC3_KBC_KBD_RX;
		isc->rx[PCKBC_AUX_SLOT] = IOC3_KBC_AUX_RX;
		isc->tx[PCKBC_KBD_SLOT] = IOC3_KBC_KBD_TX;
		isc->tx[PCKBC_AUX_SLOT] = IOC3_KBC_AUX_TX;
	}

	/* Setup bus space mapping. */
	isc->iot = iaa->iaa_memt;
	isc->ioh = iaa->iaa_memh;

	/* Setup pckbc_internal structure. */
	t = malloc(sizeof(struct pckbc_internal), M_DEVBUF,
	    M_WAITOK | M_ZERO);
	t->t_iot = isc->iot;
	t->t_ioh_d = isc->ioh;
	t->t_ioh_c = isc->ioh;
	t->t_addr = iaa->iaa_base;
	t->t_sc = (struct pckbc_softc *)sc;
	sc->id = t;

	timeout_set(&t->t_cleanup, iockbc_cleanup, t);
	timeout_set(&t->t_poll, iockbc_poll, t);

	/* Establish interrupt handler. */
	if (ioc_intr_establish(parent, iaa->iaa_dev, IPL_TTY, iockbcintr,
	    (void *)isc, sc->sc_dv.dv_xname))
		printf("\n");
	else
		printf(": unable to establish interrupt\n");

	/*
	 * Attach "slots". 
	 */
	iockbc_attach_slot(isc, PCKBC_KBD_SLOT);
	iockbc_attach_slot(isc, PCKBC_AUX_SLOT);
}

void
iockbc_poll(void *self)
{
	struct pckbc_internal *t = self;
        int s;

	s = spltty();
	(void)iockbcintr_internal(t, t->t_sc);
	timeout_add_sec(&t->t_poll, 1);
	splx(s);
}

int
iockbcintr(void *vsc)
{
	struct iockbc_softc *isc = (struct iockbc_softc *)vsc;
	struct pckbc_softc *sc = &isc->sc_pckbc;
	struct pckbc_internal *t = sc->id;

	return iockbcintr_internal(t, sc);
}

int
iockbcintr_internal(struct pckbc_internal *t, struct pckbc_softc *sc)
{
	struct iockbc_softc *isc = (void *)sc;
	pckbc_slot_t slot;
	struct pckbc_slotdata *q;
	int served = 0;
	uint32_t data;
	uint32_t val;

	/* Reschedule timeout further into the idle times. */
	if (timeout_pending(&t->t_poll))
		timeout_add_sec(&t->t_poll, 1);

	/*
	 * Need to check both "slots" since interrupt could be from
	 * either controller.
	 */ 
	for (slot = 0; slot < PCKBC_NSLOTS; slot++) {
		q = t->t_slotdata[slot];

		for (;;) {
			if (!q) {
				DPRINTF("iockbcintr: no slot%d data!\n", slot);
				break;
			}

			if (q->polling) {
				served = 1;
				break; /* pckbc_poll_data() will get it */
			}

			val = bus_space_read_4(t->t_iot, t->t_ioh_d,
			    isc->rx[slot]);
			if ((val & IOC3_KBC_DATA_VALID) == 0)
        	        	break;

			served = 1;

			/* Process received data. */
			if (val & IOC3_KBC_DATA_0_VALID) {
				data = (val & IOC3_KBC_DATA_0_MASK) >>
				    IOC3_KBC_DATA_0_SHIFT;
				iockbc_process_input(sc, t, slot, data);
			}

			if (val & IOC3_KBC_DATA_1_VALID) {
				data = (val & IOC3_KBC_DATA_1_MASK) >>
				    IOC3_KBC_DATA_1_SHIFT;
				iockbc_process_input(sc, t, slot, data);
			}

			if (val & IOC3_KBC_DATA_2_VALID) {
				data = (val & IOC3_KBC_DATA_2_MASK) >>
				    IOC3_KBC_DATA_2_SHIFT;
				iockbc_process_input(sc, t, slot, data);
			}
		}
	}
	
	return (served);
}

void
iockbc_process_input(struct pckbc_softc *sc, struct pckbc_internal *t,
    int slot, uint data)
{
	struct pckbc_slotdata *q;

	q = t->t_slotdata[slot];
	if (CMD_IN_QUEUE(q) && iockbc_cmdresponse(t, slot, data))
		return;

	if (sc->inputhandler[slot])
		(*sc->inputhandler[slot])(sc->inputarg[slot], data);
	else
		DPRINTF("iockbcintr: slot %d lost %d\n", slot, data);
}

int
iockbc_poll_write(struct pckbc_internal *t, pckbc_slot_t slot, int val)
{
	struct iockbc_softc *isc = (void *)t->t_sc;
	bus_space_tag_t iot = t->t_iot;
	bus_space_handle_t ioh = t->t_ioh_d;
	u_int64_t stat, offset, busy;
	int timeout = 10000;

	offset = isc->tx[slot];
	busy = slot == PCKBC_AUX_SLOT ?  IOC3_KBC_STATUS_AUX_WRITE_PENDING :
	    IOC3_KBC_STATUS_KBD_WRITE_PENDING;

	/* Attempt to write a value to the controller. */
	while (timeout--) {
		stat = bus_space_read_4(iot, ioh, IOC3_KBC_CTRL_STATUS);
		if ((stat & busy) == 0) {
			bus_space_write_4(iot, ioh, offset, val & 0xff);
			return 0;
		}
		delay(50);
	}

	return -1;
}

int
iockbc_poll_read(struct pckbc_internal *t, pckbc_slot_t slot)
{
	struct iockbc_softc *isc = (void *)t->t_sc;
	struct pckbc_slotdata *q = t->t_slotdata[slot];
	int timeout = 10000;
	u_int32_t val;

	/* See if we already have bytes queued. */
	if (q->rx_index >= 0)
		return q->rx_queue[q->rx_index--];

        /* Poll input from controller. */
        while (timeout--) {
		val = bus_space_read_4(t->t_iot, t->t_ioh_d, isc->rx[slot]);
		if (val & IOC3_KBC_DATA_VALID)
                	break;
                delay(50);
        }

	/* Process received data. */
	if (val & IOC3_KBC_DATA_2_VALID)
		q->rx_queue[++q->rx_index] =
		    (val & IOC3_KBC_DATA_2_MASK) >> IOC3_KBC_DATA_2_SHIFT;

	if (val & IOC3_KBC_DATA_1_VALID)
		q->rx_queue[++q->rx_index] =
		    (val & IOC3_KBC_DATA_1_MASK) >> IOC3_KBC_DATA_1_SHIFT;

	if (val & IOC3_KBC_DATA_0_VALID)
		q->rx_queue[++q->rx_index] =
		    (val & IOC3_KBC_DATA_0_MASK) >> IOC3_KBC_DATA_0_SHIFT;

	if (q->rx_index >= 0)
		return q->rx_queue[q->rx_index--];
	else 
		return -1;
}

/*
 * Pass command to device, poll for ACK and data.
 * to be called at spltty()
 */
static void
iockbc_poll_cmd(struct pckbc_internal *t, pckbc_slot_t slot, 
    struct pckbc_devcmd *cmd)
{
	int i, c = 0;

	while (cmd->cmdidx < cmd->cmdlen) {
		if (iockbc_poll_write(t, slot, cmd->cmd[cmd->cmdidx]) == -1) {
			DPRINTF("iockbc_poll_cmd: send error\n");
			cmd->status = EIO;
			return;
		}
		for (i = 10; i; i--) { /* 1s ??? */
			c = iockbc_poll_read(t, slot);
			if (c != -1)
				break;
		}
		if (c == KBC_DEVCMD_ACK) {
			cmd->cmdidx++;
			continue;
		}
		if (c == KBC_DEVCMD_RESEND) {
			DPRINTF("iockbc_cmd: RESEND\n");
			if (cmd->retries++ < 5)
				continue;
			else {
				DPRINTF("iockbc: cmd failed\n");
				cmd->status = EIO;
				return;
			}
		}
		if (c == -1) {
			DPRINTF("iockbc_cmd: timeout\n");
			cmd->status = EIO;
			return;
		}
		DPRINTF("iockbc_cmd: lost 0x%x\n", c);
	}

	while (cmd->responseidx < cmd->responselen) {
		if (cmd->flags & KBC_CMDFLAG_SLOW)
			i = 100; /* 10s ??? */
		else
			i = 10; /* 1s ??? */
		while (i--) {
			c = iockbc_poll_read(t, slot);
			if (c != -1)
				break;
		}
		if (c == -1) {
			DPRINTF("iockbc_poll_cmd: no data\n");
			cmd->status = ETIMEDOUT;
			return;
		} else
			cmd->response[cmd->responseidx++] = c;
	}
}

/*
 * Clean up a command queue, throw away everything.
 */
void
iockbc_cleanqueue(struct pckbc_slotdata *q)
{
	struct pckbc_devcmd *cmd;
#ifdef IOCKBC_DEBUG
	int i;
#endif

	while ((cmd = TAILQ_FIRST(&q->cmdqueue))) {
		TAILQ_REMOVE(&q->cmdqueue, cmd, next);
#ifdef IOCKBC_DEBUG
		printf("iockbc_cleanqueue: removing");
		for (i = 0; i < cmd->cmdlen; i++)
			printf(" %02x", cmd->cmd[i]);
		printf("\n");
#endif
		TAILQ_INSERT_TAIL(&q->freequeue, cmd, next);
	}
}

/*
 * Timeout error handler: clean queues and data port.
 * XXX could be less invasive.
 */
void
iockbc_cleanup(void *self)
{
	struct pckbc_internal *t = self;
	int s;

	printf("iockbc: command timeout\n");

	s = spltty();

	if (t->t_slotdata[PCKBC_KBD_SLOT])
		iockbc_cleanqueue(t->t_slotdata[PCKBC_KBD_SLOT]);
	if (t->t_slotdata[PCKBC_AUX_SLOT])
		iockbc_cleanqueue(t->t_slotdata[PCKBC_AUX_SLOT]);

	while (iockbc_poll_read(t, PCKBC_KBD_SLOT) 
	       != -1) ;
	while (iockbc_poll_read(t, PCKBC_AUX_SLOT) 
	       != -1) ;

	/* Reset KBC? */

	splx(s);
}

/*
 * Pass command to device during normal operation.
 * to be called at spltty()
 */
void
iockbc_start(struct pckbc_internal *t, pckbc_slot_t slot)
{
	struct pckbc_slotdata *q = t->t_slotdata[slot];
	struct pckbc_devcmd *cmd = TAILQ_FIRST(&q->cmdqueue);

	if (q->polling) {
		do {
			iockbc_poll_cmd(t, slot, cmd);
			if (cmd->status)
				printf("iockbc_start: command error\n");

			TAILQ_REMOVE(&q->cmdqueue, cmd, next);
			if (cmd->flags & KBC_CMDFLAG_SYNC)
				wakeup(cmd);
			else {
				timeout_del(&t->t_cleanup);
				TAILQ_INSERT_TAIL(&q->freequeue, cmd, next);
			}
			cmd = TAILQ_FIRST(&q->cmdqueue);
		} while (cmd);
		return;
	}

	if (iockbc_poll_write(t, slot, cmd->cmd[cmd->cmdidx])) {
		printf("iockbc_start: send error\n");
		/* XXX what now? */
		return;
	}
}

/*
 * Handle command responses coming in asynchronously,
 * return nonzero if valid response.
 * to be called at spltty()
 */
int
iockbc_cmdresponse(struct pckbc_internal *t, pckbc_slot_t slot, u_char data)
{
	struct pckbc_slotdata *q = t->t_slotdata[slot];
	struct pckbc_devcmd *cmd = TAILQ_FIRST(&q->cmdqueue);
#ifdef DIAGNOSTIC
	if (!cmd)
		panic("iockbc_cmdresponse: no active command");
#endif
	if (cmd->cmdidx < cmd->cmdlen) {
		if (data != KBC_DEVCMD_ACK && data != KBC_DEVCMD_RESEND)
			return (0);

		if (data == KBC_DEVCMD_RESEND) {
			if (cmd->retries++ < 5) {
				/* try again last command */
				goto restart;
			} else {
				DPRINTF("iockbc: cmd failed\n");
				cmd->status = EIO;
				/* dequeue */
			}
		} else {
			if (++cmd->cmdidx < cmd->cmdlen)
				goto restart;
			if (cmd->responselen)
				return (1);
			/* else dequeue */
		}
	} else if (cmd->responseidx < cmd->responselen) {
		cmd->response[cmd->responseidx++] = data;
		if (cmd->responseidx < cmd->responselen)
			return (1);
		/* else dequeue */
	} else
		return (0);

	/* dequeue: */
	TAILQ_REMOVE(&q->cmdqueue, cmd, next);
	if (cmd->flags & KBC_CMDFLAG_SYNC)
		wakeup(cmd);
	else {
		timeout_del(&t->t_cleanup);
		TAILQ_INSERT_TAIL(&q->freequeue, cmd, next);
	}
	if (!CMD_IN_QUEUE(q))
		return (1);
restart:
	iockbc_start(t, slot);
	return (1);
}

/*
 * Interfaces to act like pckbc(4).
 */

int
pckbc_xt_translation(pckbc_tag_t self, pckbc_slot_t slot, int on)
{
	/* Translation isn't supported... */
	return 0;
}

/* For use in autoconfiguration. */
int
pckbc_poll_cmd(pckbc_tag_t self, pckbc_slot_t slot, u_char *cmd, int len, 
    int responselen, u_char *respbuf, int slow)
{
	struct pckbc_devcmd nc;
	int s;

	if ((len > 4) || (responselen > 4))
		return (EINVAL);

	bzero(&nc, sizeof(nc));
	bcopy(cmd, nc.cmd, len);
	nc.cmdlen = len;
	nc.responselen = responselen;
	nc.flags = (slow ? KBC_CMDFLAG_SLOW : 0);

	s = spltty();
	iockbc_poll_cmd(self, slot, &nc);
	splx(s);

	if (nc.status == 0 && respbuf)
		bcopy(nc.response, respbuf, responselen);

	return (nc.status);
}

void
pckbc_flush(pckbc_tag_t self, pckbc_slot_t slot)
{
	/* Read any data and discard. */
	struct pckbc_internal *t = self;
	(void) iockbc_poll_read(t, slot);
}

/*
 * Put command into the device's command queue, return zero or errno.
 */
int
pckbc_enqueue_cmd(pckbc_tag_t self, pckbc_slot_t slot, u_char *cmd, int len, 
    int responselen, int sync, u_char *respbuf)
{
	struct pckbc_internal *t = self;
	struct pckbc_slotdata *q = t->t_slotdata[slot];
	struct pckbc_devcmd *nc;
	int s, isactive, res = 0;

	if ((len > 4) || (responselen > 4))
		return (EINVAL);
	s = spltty();
	nc = TAILQ_FIRST(&q->freequeue);
	if (nc)
		TAILQ_REMOVE(&q->freequeue, nc, next);
	splx(s);
	if (!nc)
		return (ENOMEM);

	bzero(nc, sizeof(*nc));
	bcopy(cmd, nc->cmd, len);
	nc->cmdlen = len;
	nc->responselen = responselen;
	nc->flags = (sync ? KBC_CMDFLAG_SYNC : 0);

	s = spltty();

	if (q->polling && sync) {
		/*
		 * XXX We should poll until the queue is empty.
		 * But we don't come here normally, so make
		 * it simple and throw away everything.
		 */
		iockbc_cleanqueue(q);
	}

	isactive = CMD_IN_QUEUE(q);
	TAILQ_INSERT_TAIL(&q->cmdqueue, nc, next);
	if (!isactive)
		iockbc_start(t, slot);

	if (q->polling)
		res = (sync ? nc->status : 0);
	else if (sync) {
		if ((res = tsleep(nc, 0, "kbccmd", 1*hz))) {
			TAILQ_REMOVE(&q->cmdqueue, nc, next);
			iockbc_cleanup(t);
		} else
			res = nc->status;
	} else
		timeout_add_sec(&t->t_cleanup, 1);

	if (sync) {
		if (respbuf)
			bcopy(nc->response, respbuf, responselen);
		TAILQ_INSERT_TAIL(&q->freequeue, nc, next);
	}

	splx(s);

	return (res);
}

int
pckbc_poll_data(pckbc_tag_t self, pckbc_slot_t slot)
{
	struct pckbc_internal *t = self;
	struct pckbc_slotdata *q = t->t_slotdata[slot];
	int c;

	c = iockbc_poll_read(t, slot);
	if (c != -1 && q && CMD_IN_QUEUE(q)) {
		/* We jumped into a running command - try to deliver the 
		   response. */
		if (iockbc_cmdresponse(t, slot, c))
			return (-1);
	}
	return (c);
}

void
pckbc_set_inputhandler(pckbc_tag_t self, pckbc_slot_t slot, pckbc_inputfcn func,
    void *arg, char *name)
{
	struct pckbc_internal *t = (struct pckbc_internal *)self;
	struct pckbc_softc *sc = t->t_sc;

	if (slot >= PCKBC_NSLOTS)
		panic("iockbc_set_inputhandler: bad slot %d", slot);

	sc->inputhandler[slot] = func;
	sc->inputarg[slot] = arg;
	sc->subname[slot] = name;

	if (iockbc_console && slot == PCKBC_KBD_SLOT)
		timeout_add_sec(&t->t_poll, 1);
}

void
pckbc_slot_enable(pckbc_tag_t self, pckbc_slot_t slot, int on)
{
	struct pckbc_internal *t = (struct pckbc_internal *)self;

	if (slot == PCKBC_KBD_SLOT) {
		if (on)
			timeout_add_sec(&t->t_poll, 1);
		else
			timeout_del(&t->t_poll);
	}
}

void
pckbc_set_poll(pckbc_tag_t self, pckbc_slot_t slot, int on)
{
	struct pckbc_internal *t = (struct pckbc_internal *)self;

	t->t_slotdata[slot]->polling = on;

	if (!on) {
		int s;

		/*
		 * If disabling polling on a device that's been configured,
		 * make sure there are no bytes left in the FIFO, holding up
		 * the interrupt line.  Otherwise we won't get any further
		 * interrupts.
		 */
		if (t->t_sc) {
			s = spltty();
			iockbcintr(t->t_sc);
			splx(s);
		}
	}
}
