/*	$OpenBSD: src/usr.sbin/ripd/ripd.h,v 1.4 2006/11/15 20:21:46 deraadt Exp $ */

/*
 * Copyright (c) 2004 Esben Norby <norby@openbsd.org>
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
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

#ifndef _RIPD_H_
#define _RIPD_H_

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/tree.h>
#include <md5.h>
#include <net/if.h>
#include <netinet/in.h>
#include <event.h>

#define	CONF_FILE		"/etc/ripd.conf"
#define	RIPD_SOCKET		"/var/run/ripd.sock"
#define	RIPD_USER		"_ripd"

#define	NBR_HASHSIZE		128
#define	NBR_IDSELF		1
#define	NBR_CNTSTART		(NBR_IDSELF + 1)

#define	ROUTE_TIMEOUT		180
#define ROUTE_GARBAGE		120

#define	NBR_TIMEOUT		180

#define	READ_BUF_SIZE		65535
#define RT_BUF_SIZE		16384
#define MAX_RTSOCK_BUF		128 * 1024

#define	RIPD_FLAG_NO_FIB_UPDATE	0x0001

#define	F_RIPD_INSERTED		0x0001
#define	F_KERNEL		0x0002
#define	F_BGPD_INSERTED		0x0004
#define	F_CONNECTED		0x0008
#define	F_DOWN			0x0010
#define	F_STATIC		0x0020
#define	F_DYNAMIC		0x0040
#define	F_OSPFD_INSERTED	0x0080
#define	F_REDISTRIBUTED		0x0100

#define	REDISTRIBUTE_NONE	0x0
#define	REDISTRIBUTE_STATIC	0x01
#define	REDISTRIBUTE_CONNECTED	0x02
#define	REDISTRIBUTE_DEFAULT	0x04

#define	OPT_SPLIT_HORIZON	0x01
#define	OPT_SPLIT_POISONED	0x02
#define	OPT_TRIGGERED_UPDATES	0x04

/* buffer */
struct buf {
	TAILQ_ENTRY(buf)	 entry;
	u_char			*buf;
	size_t			 size;
	size_t			 max;
	size_t			 wpos;
	size_t			 rpos;
};

struct msgbuf {
	TAILQ_HEAD(, buf)	 bufs;
	u_int32_t		 queued;
	int			 fd;
};

#define IMSG_HEADER_SIZE	sizeof(struct imsg_hdr)
#define MAX_IMSGSIZE		8192

struct buf_read {
	u_char			 buf[READ_BUF_SIZE];
	u_char			*rptr;
	size_t			 wpos;
};

struct imsgbuf {
	TAILQ_HEAD(, imsg_fd)	fds;
	struct buf_read		r;
	struct msgbuf		w;
	struct event		ev;
	void			(*handler)(int, short, void *);
	int			fd;
	pid_t			pid;
	short			events;
};

enum imsg_type {
	IMSG_NONE,
	IMSG_CTL_END,
	IMSG_CTL_RELOAD,
	IMSG_CTL_IFINFO,
	IMSG_IFINFO,
	IMSG_CTL_FIB_COUPLE,
	IMSG_CTL_FIB_DECOUPLE,
	IMSG_CTL_KROUTE,
	IMSG_CTL_KROUTE_ADDR,
	IMSG_CTL_SHOW_INTERFACE,
	IMSG_CTL_SHOW_IFACE,
	IMSG_CTL_SHOW_NBR,
	IMSG_CTL_SHOW_RIB,
	IMSG_KROUTE_CHANGE,
	IMSG_KROUTE_DELETE,
	IMSG_KROUTE_GET,
	IMSG_NETWORK_ADD,
	IMSG_NETWORK_DEL,
	IMSG_ROUTE_FEED,
	IMSG_RESPONSE_ADD,
	IMSG_SEND_RESPONSE,
	IMSG_FULL_RESPONSE,
	IMSG_ROUTE_REQUEST,
	IMSG_ROUTE_REQUEST_END,
	IMSG_FULL_REQUEST,
	IMSG_REQUEST_ADD,
	IMSG_SEND_REQUEST,
	IMSG_SEND_TRIGGERED_UPDATE
};

struct imsg_hdr {
	enum imsg_type	type;
	u_int16_t	len;
	u_int32_t	peerid;
	pid_t		pid;
};

struct imsg {
	struct imsg_hdr	 hdr;
	void		*data;
};

struct imsg_fd {
	TAILQ_ENTRY(imsg_fd)	entry;
	int			fd;
};

/* interface states */
#define IF_STA_DOWN		0x01
#define IF_STA_ACTIVE		(~IF_STA_DOWN)
#define IF_STA_ANY		0x7f

/* interface events */
enum iface_event {
	IF_EVT_NOTHING,
	IF_EVT_UP,
	IF_EVT_DOWN
};

/* interface actions */
enum iface_action {
	IF_ACT_NOTHING,
	IF_ACT_STRT,
	IF_ACT_RST
};

/* interface types */
enum iface_type {
	IF_TYPE_POINTOPOINT,
	IF_TYPE_BROADCAST,
	IF_TYPE_NBMA,
	IF_TYPE_POINTOMULTIPOINT
};

/* neighbor states */
#define NBR_STA_DOWN		0x01
#define	NBR_STA_REQ_RCVD	0x02
#define NBR_STA_ACTIVE		(~NBR_STA_DOWN)
#define NBR_STA_ANY		0xff

struct auth_md {
	TAILQ_ENTRY(auth_md)	 entry;
	u_int32_t		 seq_modulator;
	char			 key[MD5_DIGEST_LENGTH];
	u_int8_t		 keyid;
};

#define MAX_SIMPLE_AUTH_LEN	16

/* auth types */
enum auth_type {
	AUTH_NONE = 1,
	AUTH_SIMPLE,
	AUTH_CRYPT
};

TAILQ_HEAD(auth_md_head, auth_md);
TAILQ_HEAD(packet_head, packet_entry);

struct iface {
	LIST_ENTRY(iface)	 entry;
	LIST_HEAD(, nbr)         nbr_list;
	LIST_HEAD(, nbr_failed)	 failed_nbr_list;
	char                     name[IF_NAMESIZE];
	char			 auth_key[MAX_SIMPLE_AUTH_LEN];
	struct in_addr           addr;
	struct in_addr           dst;
	struct in_addr           mask;
	struct packet_head	 rq_list;
	struct packet_head	 rp_list;
	struct auth_md_head	 auth_md_list;

	time_t			 uptime;
	u_long			 mtu;
	u_long			 baudrate;
	int			 fd; /* XXX */
	int			 state;
	u_short			 ifindex;
	u_int16_t		 cost;
	u_int16_t		 flags;
	enum iface_type		 type;
	enum auth_type		 auth_type;
	u_int8_t		 linktype;
	u_int8_t		 media_type;
	u_int8_t		 passive;
	u_int8_t		 linkstate;
	u_int8_t		 auth_keyid;
};

struct rip_route {
	struct in_addr		 address;
	struct in_addr		 mask;
	struct in_addr		 nexthop;
	int			 refcount;
	u_short			 ifindex;
	u_int8_t		 metric;
};

struct packet_entry {
	TAILQ_ENTRY(packet_entry)	 entry;
	struct rip_route		*rr;
};

enum {
	PROC_MAIN,
	PROC_RIP_ENGINE,
	PROC_RDE_ENGINE
} ripd_process;

struct ripd_conf {
	struct event		 ev;
	struct event		 report_timer;
	LIST_HEAD(, iface)	 iface_list;
	u_int32_t		 opts;
#define RIPD_OPT_VERBOSE	0x00000001
#define	RIPD_OPT_VERBOSE2	0x00000002
#define	RIPD_OPT_NOACTION	0x00000004
	int			 flags;
	int			 redistribute_flags;
	int			 options;
	int			 rip_socket;
};

/* kroute */
struct kroute {
	struct in_addr   prefix;
	struct in_addr   netmask;
	struct in_addr	 nexthop;
	u_int8_t	 metric;
	u_int16_t	 flags;
	u_short		 ifindex;
};

struct kif {
	char		 ifname[IF_NAMESIZE];
	u_long		 baudrate;
	int		 flags;
	int		 mtu;
	u_short		 ifindex;
	u_int8_t	 media_type;
	u_int8_t	 link_state;
	u_int8_t	 nh_reachable;	/* for nexthop verification */
};

/* control data structures */
struct ctl_iface {
	char			 name[IF_NAMESIZE];
	struct in_addr		 addr;
	struct in_addr		 mask;

	time_t			 uptime;
	time_t			 report_timer;

	u_int32_t		 baudrate;

	unsigned int		 ifindex;
	int			 state;
	int			 mtu;

	u_int16_t		 flags;
	u_int16_t		 metric;
	enum iface_type		 type;
	u_int8_t		 linkstate;
	u_int8_t		 mediatype;
	u_int8_t		 passive;
};

struct ctl_rt {
	struct in_addr		 prefix;
	struct in_addr		 netmask;
	struct in_addr		 nexthop;
	time_t			 uptime;
	time_t			 expire;
	u_int32_t		 metric;
	u_int8_t		 flags;
};

struct ctl_nbr {
	char			 name[IF_NAMESIZE];
	struct in_addr		 id;
	struct in_addr		 addr;
	time_t			 dead_timer;
	time_t			 uptime;
	int			 nbr_state;
	int			 iface_state;
};

int		 kif_init(void);
int		 kr_init(int);
int		 kr_change(struct kroute *);
int		 kr_delete(struct kroute *);
void		 kr_shutdown(void);
void		 kr_fib_couple(void);
void		 kr_fib_decouple(void);
void		 kr_dispatch_msg(int, short, void *);
void		 kr_show_route(struct imsg *);
void		 kr_ifinfo(char *, pid_t);
struct kif	*kif_findname(char *);

in_addr_t	 prefixlen2mask(u_int8_t);
u_int8_t	 mask2prefixlen(in_addr_t);

/* ripd.c */
void		 main_imsg_compose_ripe(int, pid_t, void *, u_int16_t);
void		 main_imsg_compose_rde(int, pid_t, void *, u_int16_t);
int		 rip_redistribute(struct kroute *);

/* parse.y */
struct ripd_conf	*parse_config(char *, int);
int			 cmdline_symset(char *);

/* buffer.c */
struct buf	*buf_open(size_t);
struct buf	*buf_dynamic(size_t, size_t);
int		 buf_add(struct buf *, void *, size_t);
void		*buf_reserve(struct buf *, size_t);
void		*buf_seek(struct buf *, size_t, size_t);
int		 buf_close(struct msgbuf *, struct buf *);
void		 buf_free(struct buf *);
void		 msgbuf_init(struct msgbuf *);
void		 msgbuf_clear(struct msgbuf *);
int		 msgbuf_write(struct msgbuf *);

/* imsg.c */
void		 imsg_init(struct imsgbuf *, int, void (*)(int, short, void *));
ssize_t		 imsg_read(struct imsgbuf *);
ssize_t		 imsg_get(struct imsgbuf *, struct imsg *);
int		 imsg_compose(struct imsgbuf *, enum imsg_type, u_int32_t,
		    pid_t, void *, u_int16_t);
struct buf	*imsg_create(struct imsgbuf *, enum imsg_type, u_int32_t, pid_t,
		    u_int16_t);
int		 imsg_add(struct buf *, void *, u_int16_t);
int		 imsg_close(struct imsgbuf *, struct buf *);
void		 imsg_free(struct imsg *);
void		 imsg_event_add(struct imsgbuf *);

/* printconf.c */
void		 print_config(struct ripd_conf *);

/* log.c */
const char	*if_state_name(int);
const char	*if_auth_name(enum auth_type);
const char	*nbr_state_name(int);

/* interface.c */
struct iface	*if_find_index(u_short);

#endif /* _RIPD_H_ */
