/*	$OpenBSD: src/usr.sbin/relayd/pfe.c,v 1.68 2011/05/09 12:08:47 reyk Exp $	*/

/*
 * Copyright (c) 2006 Pierre-Yves Ritschard <pyr@openbsd.org>
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
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <net/if.h>

#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include <openssl/ssl.h>

#include "relayd.h"

void	 pfe_init(struct privsep *, struct privsep_proc *p, void *);
void	 pfe_shutdown(void);
void	 pfe_setup_events(void);
void	 pfe_disable_events(void);
void	 pfe_sync(void);
void	 pfe_statistics(int, short, void *);

int	 pfe_dispatch_parent(int, struct privsep_proc *, struct imsg *);
int	 pfe_dispatch_hce(int, struct privsep_proc *, struct imsg *);
int	 pfe_dispatch_relay(int, struct privsep_proc *, struct imsg *);

static struct relayd	*env = NULL;

static struct privsep_proc procs[] = {
	{ "parent",	PROC_PARENT,	pfe_dispatch_parent },
	{ "relay",	PROC_RELAY,	pfe_dispatch_relay },
	{ "hce",	PROC_HCE,	pfe_dispatch_hce }
};

pid_t
pfe(struct privsep *ps, struct privsep_proc *p)
{
	env = ps->ps_env;

	init_filter(env);
	init_tables(env);

	return (proc_run(ps, p, procs, nitems(procs), pfe_init, NULL));
}

void
pfe_init(struct privsep *ps, struct privsep_proc *p, void *arg)
{
	p->p_shutdown = pfe_shutdown;
	purge_config(env, PURGE_PROTOS);
	pfe_setup_events();
	pfe_sync();
}

void
pfe_shutdown(void)
{
	flush_rulesets(env);
}

void
pfe_setup_events(void)
{
	struct timeval	 tv;

	/* Schedule statistics timer */
	evtimer_set(&env->sc_statev, pfe_statistics, NULL);
	bcopy(&env->sc_statinterval, &tv, sizeof(tv));
	evtimer_add(&env->sc_statev, &tv);
}

void
pfe_disable_events(void)
{
	event_del(&env->sc_statev);
}

int
pfe_dispatch_hce(int fd, struct privsep_proc *p, struct imsg *imsg)
{
	struct host		*host;
	struct table		*table;
	struct ctl_status	 st;

	control_imsg_forward(imsg);

	switch (imsg->hdr.type) {
	case IMSG_HOST_STATUS:
		IMSG_SIZE_CHECK(imsg, &st);
		memcpy(&st, imsg->data, sizeof(st));
		if ((host = host_find(env, st.id)) == NULL)
			fatalx("pfe_dispatch_imsg: invalid host id");
		host->he = st.he;
		if (host->flags & F_DISABLE)
			break;
		host->retry_cnt = st.retry_cnt;
		if (st.up != HOST_UNKNOWN) {
			host->check_cnt++;
			if (st.up == HOST_UP)
				host->up_cnt++;
		}
		if (host->check_cnt != st.check_cnt) {
			log_debug("%s: host %d => %d", __func__,
			    host->conf.id, host->up);
			fatalx("pfe_dispatch_imsg: desynchronized");
		}

		if (host->up == st.up)
			break;

		/* Forward to relay engine(s) */
		proc_compose_imsg(env->sc_ps, PROC_RELAY, -1,
		    IMSG_HOST_STATUS, -1, &st, sizeof(st));

		if ((table = table_find(env, host->conf.tableid))
		    == NULL)
			fatalx("pfe_dispatch_imsg: invalid table id");

		log_debug("%s: state %d for host %u %s", __func__,
		    st.up, host->conf.id, host->conf.name);

		/*
		 * Do not change the table state when the host
		 * state switches between UNKNOWN and DOWN.
		 */
		if (HOST_ISUP(st.up)) {
			table->conf.flags |= F_CHANGED;
			table->up++;
			host->flags |= F_ADD;
			host->flags &= ~(F_DEL);
		} else if (HOST_ISUP(host->up)) {
			table->up--;
			table->conf.flags |= F_CHANGED;
			host->flags |= F_DEL;
			host->flags &= ~(F_ADD);
		}

		host->up = st.up;
		break;
	case IMSG_SYNC:
		pfe_sync();
		break;
	default:
		return (-1);
	}

	return (0);
}

int
pfe_dispatch_parent(int fd, struct privsep_proc *p, struct imsg *imsg)
{
	static struct rdr	*rdr = NULL;
	static struct table	*table = NULL;
	struct host		*host, *parent;
	struct address		*virt;

	switch (imsg->hdr.type) {
	case IMSG_RECONF:
		IMSG_SIZE_CHECK(imsg, env);

		log_debug("%s: reloading configuration", __func__);

		pfe_disable_events();
		purge_config(env, PURGE_RDRS|PURGE_TABLES);
		merge_config(env, (struct relayd *)imsg->data);

		/*
		 * no relays when reconfiguring yet.
		 */
		env->sc_relays = NULL;
		env->sc_protos = NULL;

		env->sc_tables = calloc(1, sizeof(*env->sc_tables));
		env->sc_rdrs = calloc(1, sizeof(*env->sc_rdrs));
		if (env->sc_tables == NULL || env->sc_rdrs == NULL)
			fatal(NULL);

		TAILQ_INIT(env->sc_tables);
		TAILQ_INIT(env->sc_rdrs);
		break;
	case IMSG_RECONF_TABLE:
		if ((table = calloc(1, sizeof(*table))) == NULL)
			fatal(NULL);
		memcpy(&table->conf, imsg->data, sizeof(table->conf));
		TAILQ_INIT(&table->hosts);
		TAILQ_INSERT_TAIL(env->sc_tables, table, entry);
		break;
	case IMSG_RECONF_HOST:
		if ((host = calloc(1, sizeof(*host))) == NULL)
			fatal(NULL);
		memcpy(&host->conf, imsg->data, sizeof(host->conf));
		host->tablename = table->conf.name;
		TAILQ_INSERT_TAIL(&table->hosts, host, entry);
		if (host->conf.parentid) {
			parent = host_find(env, host->conf.parentid);
			SLIST_INSERT_HEAD(&parent->children,
			    host, child);
		}
		break;
	case IMSG_RECONF_RDR:
		if ((rdr = calloc(1, sizeof(*rdr))) == NULL)
			fatal(NULL);
		memcpy(&rdr->conf, imsg->data,
		    sizeof(rdr->conf));
		rdr->table = table_find(env,
		     rdr->conf.table_id);
		if (rdr->conf.backup_id == EMPTY_TABLE)
			rdr->backup = &env->sc_empty_table;
		else
			rdr->backup = table_find(env,
			    rdr->conf.backup_id);
		if (rdr->table == NULL || rdr->backup == NULL)
			fatal("pfe_dispatch_parent:"
			    " corrupted configuration");
		log_debug("%s: redirect table %s, backup %s",
		    __func__,
		    rdr->table->conf.name,
		    rdr->backup->conf.name);
		TAILQ_INIT(&rdr->virts);
		TAILQ_INSERT_TAIL(env->sc_rdrs, rdr, entry);
		break;
	case IMSG_RECONF_VIRT:
		if ((virt = calloc(1, sizeof(*virt))) == NULL)
			fatal(NULL);
		memcpy(virt, imsg->data, sizeof(*virt));
		TAILQ_INSERT_TAIL(&rdr->virts, virt, entry);
		break;
	case IMSG_RECONF_END:
		log_warnx("%s: configuration reloaded", __func__);
		init_tables(env);
		pfe_setup_events();
		pfe_sync();
		break;
	default:
		return (-1);
	}

	return (0);
}

int
pfe_dispatch_relay(int fd, struct privsep_proc *p, struct imsg *imsg)
{
	struct ctl_natlook	 cnl;
	struct ctl_stats	 crs;
	struct relay		*rlay;

	switch (imsg->hdr.type) {
	case IMSG_NATLOOK:
		IMSG_SIZE_CHECK(imsg, &cnl);
		bcopy(imsg->data, &cnl, sizeof(cnl));
		if (cnl.proc > env->sc_prefork_relay)
			fatalx("pfe_dispatch_relay: "
			    "invalid relay proc");
		if (natlook(env, &cnl) != 0)
			cnl.in = -1;
		proc_compose_imsg(env->sc_ps, PROC_RELAY, cnl.proc,
		    IMSG_NATLOOK, -1, &cnl, sizeof(cnl));
		break;
	case IMSG_STATISTICS:
		IMSG_SIZE_CHECK(imsg, &crs);
		bcopy(imsg->data, &crs, sizeof(crs));
		if (crs.proc > env->sc_prefork_relay)
			fatalx("pfe_dispatch_relay: "
			    "invalid relay proc");
		if ((rlay = relay_find(env, crs.id)) == NULL)
			fatalx("pfe_dispatch_relay: invalid relay id");
		bcopy(&crs, &rlay->rl_stats[crs.proc], sizeof(crs));
		rlay->rl_stats[crs.proc].interval =
		    env->sc_statinterval.tv_sec;
		break;
	default:
		return (-1);
	}

	return (0);
}

void
show(struct ctl_conn *c)
{
	struct rdr	*rdr;
	struct host	*host;
	struct relay	*rlay;
	struct router	*rt;
	struct netroute	*nr;

	if (env->sc_rdrs == NULL)
		goto relays;
	TAILQ_FOREACH(rdr, env->sc_rdrs, entry) {
		imsg_compose_event(&c->iev, IMSG_CTL_RDR, 0, 0, -1,
		    rdr, sizeof(*rdr));
		if (rdr->conf.flags & F_DISABLE)
			continue;

		imsg_compose_event(&c->iev, IMSG_CTL_RDR_STATS, 0, 0, -1,
		    &rdr->stats, sizeof(rdr->stats));

		imsg_compose_event(&c->iev, IMSG_CTL_TABLE, 0, 0, -1,
		    rdr->table, sizeof(*rdr->table));
		if (!(rdr->table->conf.flags & F_DISABLE))
			TAILQ_FOREACH(host, &rdr->table->hosts, entry)
				imsg_compose_event(&c->iev, IMSG_CTL_HOST,
				    0, 0, -1, host, sizeof(*host));

		if (rdr->backup->conf.id == EMPTY_TABLE)
			continue;
		imsg_compose_event(&c->iev, IMSG_CTL_TABLE, 0, 0, -1,
		    rdr->backup, sizeof(*rdr->backup));
		if (!(rdr->backup->conf.flags & F_DISABLE))
			TAILQ_FOREACH(host, &rdr->backup->hosts, entry)
				imsg_compose_event(&c->iev, IMSG_CTL_HOST,
				    0, 0, -1, host, sizeof(*host));
	}
relays:
	if (env->sc_relays == NULL)
		goto routers;
	TAILQ_FOREACH(rlay, env->sc_relays, rl_entry) {
		rlay->rl_stats[env->sc_prefork_relay].id = EMPTY_ID;
		imsg_compose_event(&c->iev, IMSG_CTL_RELAY, 0, 0, -1,
		    rlay, sizeof(*rlay));
		imsg_compose_event(&c->iev, IMSG_CTL_RELAY_STATS, 0, 0, -1,
		    &rlay->rl_stats, sizeof(rlay->rl_stats));

		if (rlay->rl_dsttable == NULL)
			continue;
		imsg_compose_event(&c->iev, IMSG_CTL_TABLE, 0, 0, -1,
		    rlay->rl_dsttable, sizeof(*rlay->rl_dsttable));
		if (!(rlay->rl_dsttable->conf.flags & F_DISABLE))
			TAILQ_FOREACH(host, &rlay->rl_dsttable->hosts, entry)
				imsg_compose_event(&c->iev, IMSG_CTL_HOST,
				    0, 0, -1, host, sizeof(*host));

		if (rlay->rl_conf.backuptable == EMPTY_TABLE)
			continue;
		imsg_compose_event(&c->iev, IMSG_CTL_TABLE, 0, 0, -1,
		    rlay->rl_backuptable, sizeof(*rlay->rl_backuptable));
		if (!(rlay->rl_backuptable->conf.flags & F_DISABLE))
			TAILQ_FOREACH(host, &rlay->rl_backuptable->hosts, entry)
				imsg_compose_event(&c->iev, IMSG_CTL_HOST,
				    0, 0, -1, host, sizeof(*host));
	}

routers:
	if (env->sc_rts == NULL)
		goto end;
	TAILQ_FOREACH(rt, env->sc_rts, rt_entry) {
		imsg_compose_event(&c->iev, IMSG_CTL_ROUTER, 0, 0, -1,
		    rt, sizeof(*rt));
		if (rt->rt_conf.flags & F_DISABLE)
			continue;

		TAILQ_FOREACH(nr, &rt->rt_netroutes, nr_entry)
			imsg_compose_event(&c->iev, IMSG_CTL_NETROUTE,
			    0, 0, -1, nr, sizeof(*nr));
		imsg_compose_event(&c->iev, IMSG_CTL_TABLE, 0, 0, -1,
		    rt->rt_gwtable, sizeof(*rt->rt_gwtable));
		if (!(rt->rt_gwtable->conf.flags & F_DISABLE))
			TAILQ_FOREACH(host, &rt->rt_gwtable->hosts, entry)
				imsg_compose_event(&c->iev, IMSG_CTL_HOST,
				    0, 0, -1, host, sizeof(*host));
	}

end:
	imsg_compose_event(&c->iev, IMSG_CTL_END, 0, 0, -1, NULL, 0);
}

void
show_sessions(struct ctl_conn *c)
{
	int			 n, proc, done;
	struct imsg		 imsg;
	struct imsgbuf		*ibuf;
	struct rsession		 con;

	for (proc = 0; proc < env->sc_prefork_relay; proc++) {
		ibuf = proc_ibuf(env->sc_ps, PROC_RELAY, proc);

		/*
		 * Request all the running sessions from the process
		 */
		proc_compose_imsg(env->sc_ps, PROC_RELAY, proc,
		    IMSG_CTL_SESSION, -1, NULL, 0);
		proc_flush_imsg(env->sc_ps, PROC_RELAY, proc);
		
		/*
		 * Wait for the reply and forward the messages to the
		 * control connection.
		 */
		done = 0;
		while (!done) {
			do {
				if ((n = imsg_read(ibuf)) == -1)
					fatalx("imsg_read error");
			} while (n == -2); /* handle non-blocking I/O */
			while (!done) {
				if ((n = imsg_get(ibuf, &imsg)) == -1)
					fatalx("imsg_get error");
				if (n == 0)
					break;

				switch (imsg.hdr.type) {
				case IMSG_CTL_SESSION:
					IMSG_SIZE_CHECK(&imsg, &con);
					memcpy(&con, imsg.data, sizeof(con));

					imsg_compose_event(&c->iev,
					    IMSG_CTL_SESSION, 0, 0, -1,
					    &con, sizeof(con));
					break;
				case IMSG_CTL_END:
					done = 1;
					break;
				default:
					fatalx("wrong message for session");
					break;
				}
				imsg_free(&imsg);
			}
		}
	}

	imsg_compose_event(&c->iev, IMSG_CTL_END, 0, 0, -1, NULL, 0);
}

int
disable_rdr(struct ctl_conn *c, struct ctl_id *id)
{
	struct rdr	*rdr;

	if (id->id == EMPTY_ID)
		rdr = rdr_findbyname(env, id->name);
	else
		rdr = rdr_find(env, id->id);
	if (rdr == NULL)
		return (-1);
	id->id = rdr->conf.id;

	if (rdr->conf.flags & F_DISABLE)
		return (0);

	rdr->conf.flags |= F_DISABLE;
	rdr->conf.flags &= ~(F_ADD);
	rdr->conf.flags |= F_DEL;
	rdr->table->conf.flags |= F_DISABLE;
	log_debug("%s: redirect %d", __func__, rdr->conf.id);
	pfe_sync();
	return (0);
}

int
enable_rdr(struct ctl_conn *c, struct ctl_id *id)
{
	struct rdr	*rdr;
	struct ctl_id	 eid;

	if (id->id == EMPTY_ID)
		rdr = rdr_findbyname(env, id->name);
	else
		rdr = rdr_find(env, id->id);
	if (rdr == NULL)
		return (-1);
	id->id = rdr->conf.id;

	if (!(rdr->conf.flags & F_DISABLE))
		return (0);

	rdr->conf.flags &= ~(F_DISABLE);
	rdr->conf.flags &= ~(F_DEL);
	rdr->conf.flags |= F_ADD;
	log_debug("%s: redirect %d", __func__, rdr->conf.id);

	bzero(&eid, sizeof(eid));

	/* XXX: we're syncing twice */
	eid.id = rdr->table->conf.id;
	if (enable_table(c, &eid) == -1)
		return (-1);
	if (rdr->backup->conf.id == EMPTY_ID)
		return (0);
	eid.id = rdr->backup->conf.id;
	if (enable_table(c, &eid) == -1)
		return (-1);
	return (0);
}

int
disable_table(struct ctl_conn *c, struct ctl_id *id)
{
	struct table	*table;
	struct host	*host;

	if (id->id == EMPTY_ID)
		table = table_findbyname(env, id->name);
	else
		table = table_find(env, id->id);
	if (table == NULL)
		return (-1);
	id->id = table->conf.id;
	if (table->conf.rdrid > 0 && rdr_find(env, table->conf.rdrid) == NULL)
		fatalx("disable_table: desynchronised");

	if (table->conf.flags & F_DISABLE)
		return (0);
	table->conf.flags |= (F_DISABLE|F_CHANGED);
	table->up = 0;
	TAILQ_FOREACH(host, &table->hosts, entry)
		host->up = HOST_UNKNOWN;
	proc_compose_imsg(env->sc_ps, PROC_HCE, -1, IMSG_TABLE_DISABLE, -1,
	    &table->conf.id, sizeof(table->conf.id));

	/* Forward to relay engine(s) */
	proc_compose_imsg(env->sc_ps, PROC_RELAY, -1, IMSG_TABLE_DISABLE, -1,
	    &table->conf.id, sizeof(table->conf.id));

	log_debug("%s: table %d", __func__, table->conf.id);
	pfe_sync();
	return (0);
}

int
enable_table(struct ctl_conn *c, struct ctl_id *id)
{
	struct table	*table;
	struct host	*host;

	if (id->id == EMPTY_ID)
		table = table_findbyname(env, id->name);
	else
		table = table_find(env, id->id);
	if (table == NULL)
		return (-1);
	id->id = table->conf.id;

	if (table->conf.rdrid > 0 && rdr_find(env, table->conf.rdrid) == NULL)
		fatalx("enable_table: desynchronised");

	if (!(table->conf.flags & F_DISABLE))
		return (0);
	table->conf.flags &= ~(F_DISABLE);
	table->conf.flags |= F_CHANGED;
	table->up = 0;
	TAILQ_FOREACH(host, &table->hosts, entry)
		host->up = HOST_UNKNOWN;
	proc_compose_imsg(env->sc_ps, PROC_HCE, -1, IMSG_TABLE_ENABLE, -1,
	    &table->conf.id, sizeof(table->conf.id));

	/* Forward to relay engine(s) */
	proc_compose_imsg(env->sc_ps, PROC_RELAY, -1, IMSG_TABLE_ENABLE, -1,
	    &table->conf.id, sizeof(table->conf.id));

	log_debug("%s: table %d", __func__, table->conf.id);
	pfe_sync();
	return (0);
}

int
disable_host(struct ctl_conn *c, struct ctl_id *id, struct host *host)
{
	struct host	*h;
	struct table	*table;

	if (host == NULL) {
		if (id->id == EMPTY_ID)
			host = host_findbyname(env, id->name);
		else
			host = host_find(env, id->id);
		if (host == NULL || host->conf.parentid)
			return (-1);
	}
	id->id = host->conf.id;

	if (host->flags & F_DISABLE)
		return (0);

	if (host->up == HOST_UP) {
		if ((table = table_find(env, host->conf.tableid)) == NULL)
			fatalx("disable_host: invalid table id");
		table->up--;
		table->conf.flags |= F_CHANGED;
	}

	host->up = HOST_UNKNOWN;
	host->flags |= F_DISABLE;
	host->flags |= F_DEL;
	host->flags &= ~(F_ADD);
	host->check_cnt = 0;
	host->up_cnt = 0;

	proc_compose_imsg(env->sc_ps, PROC_HCE, -1, IMSG_HOST_DISABLE, -1,
	    &host->conf.id, sizeof(host->conf.id));

	/* Forward to relay engine(s) */
	proc_compose_imsg(env->sc_ps, PROC_RELAY, -1, IMSG_HOST_DISABLE, -1,
	    &host->conf.id, sizeof(host->conf.id));
	log_debug("%s: host %d", __func__, host->conf.id);

	if (!host->conf.parentid) {
		/* Disable all children */
		SLIST_FOREACH(h, &host->children, child)
			disable_host(c, id, h);
		pfe_sync();
	}
	return (0);
}

int
enable_host(struct ctl_conn *c, struct ctl_id *id, struct host *host)
{
	struct host	*h;

	if (host == NULL) {
		if (id->id == EMPTY_ID)
			host = host_findbyname(env, id->name);
		else
			host = host_find(env, id->id);
		if (host == NULL || host->conf.parentid)
			return (-1);
	}
	id->id = host->conf.id;

	if (!(host->flags & F_DISABLE))
		return (0);

	host->up = HOST_UNKNOWN;
	host->flags &= ~(F_DISABLE);
	host->flags &= ~(F_DEL);
	host->flags &= ~(F_ADD);

	proc_compose_imsg(env->sc_ps, PROC_HCE, -1, IMSG_HOST_ENABLE, -1,
	    &host->conf.id, sizeof (host->conf.id));

	/* Forward to relay engine(s) */
	proc_compose_imsg(env->sc_ps, PROC_RELAY, -1, IMSG_HOST_ENABLE, -1,
	    &host->conf.id, sizeof(host->conf.id));

	log_debug("%s: host %d", __func__, host->conf.id);

	if (!host->conf.parentid) {
		/* Enable all children */
		SLIST_FOREACH(h, &host->children, child)
			enable_host(c, id, h);
		pfe_sync();
	}
	return (0);
}

void
pfe_sync(void)
{
	struct rdr		*rdr;
	struct table		*active;
	struct table		*table;
	struct ctl_id		 id;
	struct imsg		 imsg;
	struct ctl_demote	 demote;
	struct router		*rt;

	bzero(&id, sizeof(id));
	bzero(&imsg, sizeof(imsg));
	TAILQ_FOREACH(rdr, env->sc_rdrs, entry) {
		rdr->conf.flags &= ~(F_BACKUP);
		rdr->conf.flags &= ~(F_DOWN);

		if (rdr->conf.flags & F_DISABLE ||
		    (rdr->table->up == 0 && rdr->backup->up == 0)) {
			rdr->conf.flags |= F_DOWN;
			active = NULL;
		} else if (rdr->table->up == 0 && rdr->backup->up > 0) {
			rdr->conf.flags |= F_BACKUP;
			active = rdr->backup;
			active->conf.flags |=
			    rdr->table->conf.flags & F_CHANGED;
			active->conf.flags |=
			    rdr->backup->conf.flags & F_CHANGED;
		} else
			active = rdr->table;

		if (active != NULL && active->conf.flags & F_CHANGED) {
			id.id = active->conf.id;
			imsg.hdr.type = IMSG_CTL_TABLE_CHANGED;
			imsg.hdr.len = sizeof(id) + IMSG_HEADER_SIZE;
			imsg.data = &id;
			sync_table(env, rdr, active);
			control_imsg_forward(&imsg);
		}

		if (rdr->conf.flags & F_DOWN) {
			if (rdr->conf.flags & F_ACTIVE_RULESET) {
				flush_table(env, rdr);
				log_debug("%s: disabling ruleset", __func__);
				rdr->conf.flags &= ~(F_ACTIVE_RULESET);
				id.id = rdr->conf.id;
				imsg.hdr.type = IMSG_CTL_PULL_RULESET;
				imsg.hdr.len = sizeof(id) + IMSG_HEADER_SIZE;
				imsg.data = &id;
				sync_ruleset(env, rdr, 0);
				control_imsg_forward(&imsg);
			}
		} else if (!(rdr->conf.flags & F_ACTIVE_RULESET)) {
			log_debug("%s: enabling ruleset", __func__);
			rdr->conf.flags |= F_ACTIVE_RULESET;
			id.id = rdr->conf.id;
			imsg.hdr.type = IMSG_CTL_PUSH_RULESET;
			imsg.hdr.len = sizeof(id) + IMSG_HEADER_SIZE;
			imsg.data = &id;
			sync_ruleset(env, rdr, 1);
			control_imsg_forward(&imsg);
		}
	}

	TAILQ_FOREACH(rt, env->sc_rts, rt_entry) {
		rt->rt_conf.flags &= ~(F_BACKUP);
		rt->rt_conf.flags &= ~(F_DOWN);

		if ((rt->rt_gwtable->conf.flags & F_CHANGED))
			sync_routes(env, rt);
	}

	TAILQ_FOREACH(table, env->sc_tables, entry) {
		/*
		 * clean up change flag.
		 */
		table->conf.flags &= ~(F_CHANGED);

		/*
		 * handle demotion.
		 */
		if ((table->conf.flags & F_DEMOTE) == 0)
			continue;
		demote.level = 0;
		if (table->up && table->conf.flags & F_DEMOTED) {
			demote.level = -1;
			table->conf.flags &= ~F_DEMOTED;
		}
		else if (!table->up && !(table->conf.flags & F_DEMOTED)) {
			demote.level = 1;
			table->conf.flags |= F_DEMOTED;
		}
		if (demote.level == 0)
			continue;
		log_debug("%s: demote %d table '%s' group '%s'", __func__,
		    demote.level, table->conf.name, table->conf.demote_group);
		(void)strlcpy(demote.group, table->conf.demote_group,
		    sizeof(demote.group));
		proc_compose_imsg(env->sc_ps, PROC_PARENT, -1, IMSG_DEMOTE, -1,
		    &demote, sizeof(demote));
	}
}

void
pfe_statistics(int fd, short events, void *arg)
{
	struct rdr		*rdr;
	struct ctl_stats	*cur;
	struct timeval		 tv, tv_now;
	int			 resethour, resetday;
	u_long			 cnt;

	timerclear(&tv);
	if (gettimeofday(&tv_now, NULL) == -1)
		fatal("pfe_statistics: gettimeofday");

	TAILQ_FOREACH(rdr, env->sc_rdrs, entry) {
		cnt = check_table(env, rdr, rdr->table);
		if (rdr->conf.backup_id != EMPTY_TABLE)
			cnt += check_table(env, rdr, rdr->backup);

		resethour = resetday = 0;

		cur = &rdr->stats;
		cur->last = cnt > cur->cnt ? cnt - cur->cnt : 0;

		cur->cnt = cnt;
		cur->tick++;
		cur->avg = (cur->last + cur->avg) / 2;
		cur->last_hour += cur->last;
		if ((cur->tick % (3600 / env->sc_statinterval.tv_sec)) == 0) {
			cur->avg_hour = (cur->last_hour + cur->avg_hour) / 2;
			resethour++;
		}
		cur->last_day += cur->last;
		if ((cur->tick % (86400 / env->sc_statinterval.tv_sec)) == 0) {
			cur->avg_day = (cur->last_day + cur->avg_day) / 2;
			resethour++;
		}
		if (resethour)
			cur->last_hour = 0;
		if (resetday)
			cur->last_day = 0;

		rdr->stats.interval = env->sc_statinterval.tv_sec;
	}

	/* Schedule statistics timer */
	evtimer_set(&env->sc_statev, pfe_statistics, NULL);
	bcopy(&env->sc_statinterval, &tv, sizeof(tv));
	evtimer_add(&env->sc_statev, &tv);
}
