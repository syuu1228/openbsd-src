/*	$OpenBSD: src/usr.sbin/hoststated/Attic/parse.y,v 1.39 2007/05/26 19:58:49 pyr Exp $	*/

/*
 * Copyright (c) 2006 Pierre-Yves Ritschard <pyr@spootnik.org>
 * Copyright (c) 2004, 2005 Esben Norby <norby@openbsd.org>
 * Copyright (c) 2004 Ryan McBride <mcbride@openbsd.org>
 * Copyright (c) 2002, 2003, 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2001 Daniel Hartmeier.  All rights reserved.
 * Copyright (c) 2001 Theo de Raadt.  All rights reserved.
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

%{
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <event.h>
#include <limits.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>

#include <openssl/ssl.h>

#include "hoststated.h"

struct hoststated		*conf = NULL;
static FILE			*fin = NULL;
static int			 lineno = 1;
static int			 errors = 0;
const char			*infile;
objid_t				 last_service_id = 0;
objid_t				 last_table_id = 0;
objid_t				 last_host_id = 0;
objid_t				 last_relay_id = 0;
objid_t				 last_proto_id = 0;

static struct service		*service = NULL;
static struct table		*table = NULL;
static struct relay		*rlay = NULL;
static struct protocol		*proto = NULL;
static struct protonode		 node;

int	 yyerror(const char *, ...);
int	 yyparse(void);
int	 kw_cmp(const void *, const void *);
int	 lookup(char *);
int	 lgetc(FILE *, int *);
int	 lungetc(int);
int	 findeol(void);
int	 yylex(void);

TAILQ_HEAD(symhead, sym)	 symhead = TAILQ_HEAD_INITIALIZER(symhead);
struct sym {
	TAILQ_ENTRY(sym)	 entries;
	int			 used;
	int			 persist;
	char			*nam;
	char			*val;
};

int		 symset(const char *, const char *, int);
char		*symget(const char *);

struct address	*host_v4(const char *);
struct address	*host_v6(const char *);
int		 host_dns(const char *, struct addresslist *,
		    int, in_port_t, const char *);
int		 host(const char *, struct addresslist *,
		    int, in_port_t, const char *);

typedef struct {
	union {
		u_int32_t	 number;
		char		*string;
		struct host	*host;
		struct timeval	 tv;
	} v;
	int lineno;
} YYSTYPE;

%}

%token	SERVICE TABLE BACKUP HOST REAL
%token  CHECK TCP ICMP EXTERNAL REQUEST RESPONSE
%token  TIMEOUT CODE DIGEST PORT TAG INTERFACE
%token	VIRTUAL INTERVAL DISABLE STICKYADDR BACKLOG PATH
%token	SEND EXPECT NOTHING SSL LOADBALANCE ROUNDROBIN CIPHERS COOKIE
%token	RELAY LISTEN ON FORWARD TO NAT LOOKUP PREFORK NO MARK MARKED
%token	PROTO SESSION CACHE APPEND CHANGE REMOVE FROM FILTER HASH HEADER
%token	LOG UPDATES ALL DEMOTE NODELAY SACK SOCKET BUFFER URL RETRY IP
%token	ERROR
%token	<v.string>	STRING
%type	<v.string>	interface
%type	<v.number>	number port http_type loglevel sslcache optssl
%type	<v.number>	proto_type dstmode docheck retry log flag direction
%type	<v.host>	host
%type	<v.tv>		timeout

%%

grammar		: /* empty */
		| grammar '\n'
		| grammar varset '\n'
		| grammar main '\n'
		| grammar service '\n'
		| grammar table '\n'
		| grammar relay '\n'
		| grammar proto '\n'
		| grammar error '\n'		{ errors++; }
		;

number		: STRING	{
			const char	*estr;

			$$ = strtonum($1, 0, UINT_MAX, &estr);
			if (estr) {
				yyerror("cannot parse number %s : %s",
				    $1, estr);
				free($1);
				YYERROR;
			}
			free($1);
		}
		;

optssl		: /*empty*/	{ $$ = 0; }
		| SSL		{ $$ = 1; }
		;

http_type	: STRING	{
			if (strcmp("https", $1) == 0) {
				$$ = 1;
			} else if (strcmp("http", $1) == 0) {
				$$ = 0;
			} else {
				yyerror("invalid check type: $1", $1);
				free($1);
				YYERROR;
			}
			free($1);
		}
		;

proto_type	: TCP				{ $$ = RELAY_PROTO_TCP; }
		| STRING			{
			if (strcmp("http", $1) == 0) {
				$$ = RELAY_PROTO_HTTP;
			} else {
				yyerror("invalid protocol type: $1", $1);
				free($1);
				YYERROR;
			}
			free($1);
		}
		;

port		: PORT STRING {
			const char	*estr;
			struct servent	*servent;

			$$ = strtonum($2, 1, USHRT_MAX, &estr);
                        if (estr) {
				if (errno == ERANGE) {
					yyerror("port %s is out of range", $2);
					free($2);
					YYERROR;
				}
				servent = getservbyname($2, "tcp");
				if (servent == NULL) {
					yyerror("port %s is invalid", $2);
					free($2);
					YYERROR;
				}
				$$ = servent->s_port;
			} else
				$$ = htons($$);
			free($2);
		}
		;

varset		: STRING '=' STRING	{
			if (symset($1, $3, 0) == -1)
				fatal("cannot store variable");
			free($1);
			free($3);
		}
		;

sendbuf		: NOTHING		{
			table->sendbuf = NULL;
		}
		| STRING		{
			table->sendbuf = strdup($1);
			if (table->sendbuf == NULL)
				fatal("out of memory");
			free($1);
		}
		;

main		: INTERVAL number	{ conf->interval.tv_sec = $2; }
		| LOG loglevel		{ conf->opts |= $2; }
		| TIMEOUT timeout	{
			bcopy(&$2, &conf->timeout, sizeof(struct timeval));
		}
		| PREFORK number	{
			if ($2 <= 0 || $2 > RELAY_MAXPROC) {
				yyerror("invalid number of preforked "
				    "relays: %d", $2);
				YYERROR;
			}
			conf->prefork_relay = $2;
		}
		| DEMOTE STRING		{
			conf->flags |= F_DEMOTE;
			if (strlcpy(conf->demote_group, $2,
			    sizeof(conf->demote_group))
			    >= sizeof(conf->demote_group)) {
				yyerror("yyparse: demote group name too long");
				free($2);
				YYERROR;
			}
			free($2);
			if (carp_demote_init(conf->demote_group, 1) == -1) {
				yyerror("yyparse: error initializing group '%s'",
				    conf->demote_group);
				YYERROR;
			}
		}
		;

loglevel	: UPDATES		{ $$ = HOSTSTATED_OPT_LOGUPDATE; }
		| ALL			{ $$ = HOSTSTATED_OPT_LOGALL; }
		;

service		: SERVICE STRING	{
			struct service *srv;

			TAILQ_FOREACH(srv, &conf->services, entry)
				if (!strcmp(srv->name, $2))
					break;
			if (srv != NULL) {
				yyerror("service %s defined twice", $2);
				free($2);
				YYERROR;
			}
			if ((srv = calloc(1, sizeof (*srv))) == NULL)
				fatal("out of memory");

			if (strlcpy(srv->name, $2, sizeof(srv->name)) >=
			    sizeof(srv->name)) {
				yyerror("service name truncated");
				YYERROR;
			}
			free($2);
			srv->id = last_service_id++;
			if (last_service_id == INT_MAX) {
				yyerror("too many services defined");
				YYERROR;
			}
			service = srv;
		} '{' optnl serviceopts_l '}'	{
			if (service->table == NULL) {
				yyerror("service %s has no table",
				    service->name);
				YYERROR;
			}
			if (TAILQ_EMPTY(&service->virts)) {
				yyerror("service %s has no virtual ip",
				    service->name);
				YYERROR;
			}
			conf->servicecount++;
			if (service->backup == NULL)
				service->backup = &conf->empty_table;
			else if (service->backup->port !=
			    service->table->port) {
				yyerror("service %s uses two different ports "
				    "for its table and backup table",
				    service->name);
				YYERROR;
			}

			if (!(service->flags & F_DISABLE))
				service->flags |= F_ADD;
			TAILQ_INSERT_HEAD(&conf->services, service, entry);
		}
		;

serviceopts_l	: serviceopts_l serviceoptsl nl
		| serviceoptsl optnl
		;

serviceoptsl	: TABLE STRING	{
			struct table *tb;

			TAILQ_FOREACH(tb, &conf->tables, entry)
				if (!strcmp(tb->name, $2))
					break;
			if (tb == NULL) {
				yyerror("no such table: %s", $2);
				free($2);
				YYERROR;
			} else {
				service->table = tb;
				service->table->serviceid = service->id;
				service->table->flags |= F_USED;
				free($2);
			}
		}
		| BACKUP TABLE STRING	{
			struct table *tb;

			if (service->backup) {
				yyerror("backup already specified");
				free($3);
				YYERROR;
			}

			TAILQ_FOREACH(tb, &conf->tables, entry)
				if (!strcmp(tb->name, $3))
					break;

			if (tb == NULL) {
				yyerror("no such table: %s", $3);
				free($3);
				YYERROR;
			} else {
				service->backup = tb;
				service->backup->serviceid = service->id;
				service->backup->flags |= (F_USED|F_BACKUP);
				free($3);
			}
		}
		| VIRTUAL HOST STRING port interface {
			if (host($3, &service->virts,
				 SRV_MAX_VIRTS, $4, $5) <= 0) {
				yyerror("invalid virtual ip: %s", $3);
				free($3);
				free($5);
				YYERROR;
			}
			free($3);
			free($5);
		}
		| DISABLE			{ service->flags |= F_DISABLE; }
		| STICKYADDR			{ service->flags |= F_STICKY; }
		| TAG STRING {
			if (strlcpy(service->tag, $2, sizeof(service->tag)) >=
			    sizeof(service->tag)) {
				yyerror("service tag name truncated");
				free($2);
				YYERROR;
			}
			free($2);
		}
		;

table		: TABLE STRING	{
			struct table *tb;

			TAILQ_FOREACH(tb, &conf->tables, entry)
				if (!strcmp(tb->name, $2))
					break;
			if (tb != NULL) {
				yyerror("table %s defined twice");
				free($2);
				YYERROR;
			}

			if ((tb = calloc(1, sizeof (*tb))) == NULL)
				fatal("out of memory");

			if (strlcpy(tb->name, $2, sizeof(tb->name)) >=
			    sizeof(tb->name)) {
				yyerror("table name truncated");
				YYERROR;
			}
			tb->id = last_table_id++;
			bcopy(&conf->timeout, &tb->timeout,
			    sizeof(struct timeval));
			if (last_table_id == INT_MAX) {
				yyerror("too many tables defined");
				YYERROR;
			}
			free($2);
			table = tb;
		} '{' optnl tableopts_l '}'	{
			if (table->port == 0) {
				yyerror("table %s has no port", table->name);
				YYERROR;
			}
			if (TAILQ_EMPTY(&table->hosts)) {
				yyerror("table %s has no hosts", table->name);
				YYERROR;
			}
			if (table->check == CHECK_NOCHECK) {
				yyerror("table %s has no check", table->name);
				YYERROR;
			}
			conf->tablecount++;
			TAILQ_INSERT_HEAD(&conf->tables, table, entry);
		}
		;

tableopts_l	: tableopts_l tableoptsl nl
		| tableoptsl optnl
		;

tableoptsl	: host			{
			$1->tableid = table->id;
			$1->tablename = table->name;
			TAILQ_INSERT_HEAD(&table->hosts, $1, entry);
		}
		| TIMEOUT timeout	{
			bcopy(&$2, &table->timeout, sizeof(struct timeval));
		}
		| CHECK ICMP		{
			table->check = CHECK_ICMP;
		}
		| CHECK TCP		{
			table->check = CHECK_TCP;
		}
		| CHECK SSL		{
			table->check = CHECK_TCP;
			conf->flags |= F_SSL;
			table->flags |= F_SSL;
		}
		| CHECK http_type STRING CODE number {
			if ($2) {
				conf->flags |= F_SSL;
				table->flags |= F_SSL;
			}
			table->check = CHECK_HTTP_CODE;
			table->retcode = $5;
			if (asprintf(&table->sendbuf,
			    "HEAD %s HTTP/1.0\r\n\r\n", $3) == -1)
				fatal("asprintf");
			free($3);
			if (table->sendbuf == NULL)
				fatal("out of memory");
		}
		| CHECK http_type STRING DIGEST STRING {
			if ($2) {
				conf->flags |= F_SSL;
				table->flags |= F_SSL;
			}
			table->check = CHECK_HTTP_DIGEST;
			if (asprintf(&table->sendbuf,
			    "GET %s HTTP/1.0\r\n\r\n", $3) == -1)
				fatal("asprintf");
			free($3);
			if (table->sendbuf == NULL)
				fatal("out of memory");
			if (strlcpy(table->digest, $5,
			    sizeof(table->digest)) >= sizeof(table->digest)) {
				yyerror("http digest truncated");
				free($5);
				YYERROR;
			}
			free($5);
		}
		| CHECK SEND sendbuf EXPECT STRING optssl {
			table->check = CHECK_SEND_EXPECT;
			if ($6) {
				conf->flags |= F_SSL;
				table->flags |= F_SSL;
			}
			if (strlcpy(table->exbuf, $5, sizeof(table->exbuf))
			    >= sizeof(table->exbuf)) {
				yyerror("yyparse: expect buffer truncated");
				free($5);
				YYERROR;
			}
			free($5);
		}
		| REAL port {
			table->port = $2;
		}
		| DEMOTE STRING	{
			table->flags |= F_DEMOTE;
			if (strlcpy(table->demote_group, $2,
			    sizeof(table->demote_group))
			    >= sizeof(table->demote_group)) {
				yyerror("yyparse: demote group name too long");
				free($2);
				YYERROR;
			}
			free($2);
			if (carp_demote_init(table->demote_group, 1) == -1) {
				yyerror("yyparse: error initializing group "
				    "'%s'", table->demote_group);
				YYERROR;
			}
		}
		| DISABLE			{ table->flags |= F_DISABLE; }
		;

proto		: PROTO STRING	{
			struct protocol *p;

			TAILQ_FOREACH(p, &conf->protos, entry)
				if (!strcmp(p->name, $2))
					break;
			if (p != NULL) {
				yyerror("protocol %s defined twice", $2);
				free($2);
				YYERROR;
			}
			if ((p = calloc(1, sizeof (*p))) == NULL)
				fatal("out of memory");

			if (strlcpy(p->name, $2, sizeof(p->name)) >=
			    sizeof(p->name)) {
				yyerror("protocol name truncated");
				YYERROR;
			}
			free($2);
			p->id = last_proto_id++;
			p->cache = RELAY_CACHESIZE;
			p->type = RELAY_PROTO_TCP;
			p->tcpflags = TCPFLAG_DEFAULT;
			p->sslflags = SSLFLAG_DEFAULT;
			p->sslciphers = NULL;
			p->tcpbacklog = RELAY_BACKLOG;
			if (last_proto_id == INT_MAX) {
				yyerror("too many protocols defined");
				YYERROR;
			}
			RB_INIT(&p->request_tree);
			RB_INIT(&p->response_tree);
			proto = p;
		} '{' optnl protopts_l '}'	{
			conf->protocount++;

			if ((proto->sslflags & SSLFLAG_VERSION) == 0) {
				yyerror("invalid SSL protocol");
				YYERROR;
			}

			TAILQ_INSERT_HEAD(&conf->protos, proto, entry);
		}
		;

protopts_l	: protopts_l protoptsl nl
		| protoptsl optnl
		;

protoptsl	: SSL sslflags
		| SSL '{' sslflags_l '}'
		| TCP tcpflags
		| TCP '{' tcpflags_l '}'
		| PROTO proto_type		{ proto->type = $2; }
		| direction protonode log	{
			struct protonode 	*pn, pk;
			struct proto_tree	*tree;

			if ($1 == RELAY_DIR_RESPONSE)
				tree = &proto->response_tree;
			else
				tree = &proto->request_tree;
			pn = RB_FIND(proto_tree, tree, &node);
			if (pn != NULL) {
				yyerror("protocol node %s defined twice",
				    node.key);
				YYERROR;
			}
			if ((pn = calloc(1, sizeof (*pn))) == NULL)
				fatal("out of memory");

			bcopy(&node, pn, sizeof(*pn));
			pn->key = node.key;
			pn->value = node.value;
			pn->type = node.type;
			if ($1 == RELAY_DIR_RESPONSE)
				pn->id = proto->response_nodes++;
			else
				pn->id = proto->request_nodes++;
			if ($3)
				pn->flags |= PNFLAG_LOG;
			if (pn->id == INT_MAX) {
				yyerror("too many protocol nodes defined");
				YYERROR;
			}
			RB_INSERT(proto_tree, tree, pn);

			if (node.type == NODE_TYPE_COOKIE)	
				pk.key = "Cookie";
			else
				pk.key = "GET";
			if (node.type != NODE_TYPE_HEADER) {
				pk.type = NODE_TYPE_HEADER;
				pn = RB_FIND(proto_tree, tree, &pk);
				if (pn == NULL) {
					if ((pn = (struct protonode *)
					    calloc(1, sizeof(*pn))) == NULL)
						fatal("out of memory");
					pn->key = strdup(pk.key);
					if (pn->key == NULL)
						fatal("out of memory");
					pn->value = NULL;
					pn->action = NODE_ACTION_NONE;
					pn->type = pk.type;
					if ($1 == RELAY_DIR_RESPONSE)
						pn->id =
						    proto->response_nodes++;
					else
						pn->id = proto->request_nodes++;
					if (pn->id == INT_MAX) {
						yyerror("too many protocol "
						    "nodes defined");
						YYERROR;
					}
					RB_INSERT(proto_tree, tree, pn);
				}
				switch (node.type) {
				case NODE_TYPE_URL:
					pn->flags |= PNFLAG_LOOKUP_URL;
					break;
				case NODE_TYPE_COOKIE:
					pn->flags |= PNFLAG_LOOKUP_COOKIE;
					break;
				default:
					break;
				}
			}

			bzero(&node, sizeof(node));
		}
		;

direction	: /* empty */		{ $$ = RELAY_DIR_REQUEST; }
		| REQUEST		{ $$ = RELAY_DIR_REQUEST; }
		| RESPONSE		{ $$ = RELAY_DIR_RESPONSE; }
		;

tcpflags_l	: tcpflags comma tcpflags_l
		| tcpflags
		;

tcpflags	: SACK 			{ proto->tcpflags |= TCPFLAG_SACK; }
		| NO SACK		{ proto->tcpflags |= TCPFLAG_NSACK; }
		| NODELAY		{ proto->tcpflags |= TCPFLAG_NODELAY; }
		| NO NODELAY		{ proto->tcpflags |= TCPFLAG_NNODELAY; }
		| BACKLOG number	{
			if ($2 > RELAY_MAX_SESSIONS) {
				yyerror("invalid backlog: %d", $2);
				YYERROR;
			}
			proto->tcpbacklog = $2;
		}
		| SOCKET BUFFER number	{
			proto->tcpflags |= TCPFLAG_BUFSIZ;
			proto->tcpbufsiz = $3;
		}
		| IP STRING number	{
			if (strcasecmp("ttl", $2) == 0) {
				proto->tcpflags |= TCPFLAG_IPTTL;
				proto->tcpipttl = $3;
			} else if (strcasecmp("minttl", $2) == 0) {
				proto->tcpflags |= TCPFLAG_IPMINTTL;
				proto->tcpipminttl = $3;
			} else {
				yyerror("invalid TCP/IP flag: %s", $2);
				free($2);
				YYERROR;
			}
			free($2);
		}
		;

sslflags_l	: sslflags comma sslflags_l
		| sslflags
		;

sslflags	: SESSION CACHE sslcache	{ proto->cache = $3; }
		| CIPHERS STRING		{
			proto->sslciphers = strdup($2);
			if (proto->sslciphers == NULL)
				fatal("out of memory");
			free($2);
		}
		| NO flag			{ proto->sslflags &= ~($2); }
		| flag				{ proto->sslflags |= $1; }
		;

flag		: STRING			{
			if (strcmp("sslv2", $1) == 0)
				$$ = SSLFLAG_SSLV2;
			else if (strcmp("sslv3", $1) == 0)
				$$ = SSLFLAG_SSLV3;
			else if (strcmp("tlsv1", $1) == 0)
				$$ = SSLFLAG_TLSV1;
			else {
				yyerror("invalid SSL flag: %s", $1);
				free($1);
				YYERROR;
			}
			free($1);
		}
		;

protonode	: nodetype APPEND STRING TO STRING marked	{
			node.action = NODE_ACTION_APPEND;
			node.key = strdup($5);
			node.value = strdup($3);
			if (node.key == NULL || node.value == NULL)
				fatal("out of memory");
			if (strchr(node.value, '$') != NULL)
				node.flags |= PNFLAG_MACRO;
			free($5);
			free($3);
		}
		| nodetype CHANGE STRING TO STRING marked {
			node.action = NODE_ACTION_CHANGE;
			node.key = strdup($3);
			node.value = strdup($5);
			if (node.key == NULL || node.value == NULL)
				fatal("out of memory");
			if (strchr(node.value, '$') != NULL)
				node.flags |= PNFLAG_MACRO;
			free($5);
			free($3);
		}
		| nodetype REMOVE STRING marked	{
			node.action = NODE_ACTION_REMOVE;
			node.key = strdup($3);
			node.value = NULL;
			if (node.key == NULL)
				fatal("out of memory");
			free($3);
		}
		| nodetype EXPECT STRING FROM STRING mark	{
			node.action = NODE_ACTION_EXPECT;
			node.key = strdup($5);
			node.value = strdup($3);;
			if (node.key == NULL || node.value == NULL)
				fatal("out of memory");
			free($5);
			free($3);
		}
		| nodetype FILTER STRING FROM STRING mark	{
			node.action = NODE_ACTION_FILTER;
			node.key = strdup($5);
			node.value = strdup($3);;
			if (node.key == NULL || node.value == NULL)
				fatal("out of memory");
			free($5);
			free($3);
		}
		| nodetype HASH STRING marked			{
			node.action = NODE_ACTION_HASH;
			node.key = strdup($3);
			node.value = NULL;
			if (node.key == NULL)
				fatal("out of memory");
			free($3);
			proto->lateconnect++;
		}
		| nodetype LOG STRING marked			{
			node.action = NODE_ACTION_LOG;
			node.key = strdup($3);
			node.value = NULL;
			node.flags |= PNFLAG_LOG;
			if (node.key == NULL)
				fatal("out of memory");
			free($3);
		}
		;

mark		: /* empty */
		| MARK				{ node.flags |= PNFLAG_MARK; }
		;

marked		: /* empty */
		| MARKED			{ node.flags |= PNFLAG_MARK; }
		;

nodetype	: HEADER			{ node.type = NODE_TYPE_HEADER; }
		| URL				{ node.type = NODE_TYPE_URL; }
		| COOKIE			{ node.type = NODE_TYPE_COOKIE; }
		| PATH				{
				proto->flags |= F_LOOKUP_PATH;
				node.type = NODE_TYPE_PATH;
		}
		;

sslcache	: number			{ $$ = $1; }
		| DISABLE			{ $$ = -2; }
		;

relay		: RELAY STRING	{
			struct relay *r;

			TAILQ_FOREACH(r, &conf->relays, entry)
				if (!strcmp(r->name, $2))
					break;
			if (r != NULL) {
				yyerror("relay %s defined twice", $2);
				free($2);
				YYERROR;
			}
			if ((r = calloc(1, sizeof (*r))) == NULL)
				fatal("out of memory");

			if (strlcpy(r->name, $2, sizeof(r->name)) >=
			    sizeof(r->name)) {
				yyerror("relay name truncated");
				YYERROR;
			}
			free($2);
			r->id = last_relay_id++;
			r->timeout.tv_sec = RELAY_TIMEOUT;
			r->proto = NULL;
			r->dsttable = NULL;
			r->dstretry = 0;
			if (last_relay_id == INT_MAX) {
				yyerror("too many relays defined");
				YYERROR;
			}
			rlay = r;
		} '{' optnl relayopts_l '}'	{
			if (rlay->ss.ss_family == AF_UNSPEC) {
				yyerror("relay %s has no listener",
				    rlay->name);
				YYERROR;
			}
			if ((rlay->flags & F_NATLOOK) == 0 &&
			    rlay->dstss.ss_family == AF_UNSPEC &&
			    rlay->dsttable == NULL) {
				yyerror("relay %s has no target, service, "
				    "or table", rlay->name);
				YYERROR;
			}
			if (rlay->proto == NULL)
				rlay->proto = &conf->proto_default;
			conf->relaycount++;
			TAILQ_INIT(&rlay->sessions);
			TAILQ_INSERT_HEAD(&conf->relays, rlay, entry);
		}
		;

relayopts_l	: relayopts_l relayoptsl nl
		| relayoptsl optnl
		;

relayoptsl	: LISTEN ON STRING port optssl {
			struct addresslist 	 al;
			struct address		*h;

			if (rlay->ss.ss_family != AF_UNSPEC) {
				yyerror("relay %s listener already specified",
				    rlay->name);
				YYERROR;
			}

			TAILQ_INIT(&al);
			if (host($3, &al, 1, $4, NULL) <= 0) {
				yyerror("invalid listen ip: %s", $3);
				free($3);
				YYERROR;
			}
			free($3);
			h = TAILQ_FIRST(&al);
			bcopy(&h->ss, &rlay->ss, sizeof(rlay->ss));
			rlay->port = h->port;
			if ($5) {
				rlay->flags |= F_SSL;
				conf->flags |= F_SSL;
			}
		}
		| FORWARD TO STRING port retry {
			struct addresslist 	 al;
			struct address		*h;

			if (rlay->dstss.ss_family != AF_UNSPEC) {
				yyerror("relay %s target or service already "
				    "specified", rlay->name);
				free($3);
				YYERROR;
			}

			TAILQ_INIT(&al);
			if (host($3, &al, 1, $4, NULL) <= 0) {
				yyerror("invalid listen ip: %s", $3);
				free($3);
				YYERROR;
			}
			free($3);
			h = TAILQ_FIRST(&al);
			bcopy(&h->ss, &rlay->dstss, sizeof(rlay->dstss));
			rlay->dstport = h->port;
			rlay->dstretry = $5;
		}
		| SERVICE STRING retry {
			struct service	*svc;
			struct address	*h;

			if (rlay->dstss.ss_family != AF_UNSPEC) {
				yyerror("relay %s target or service already "
				    "specified", rlay->name);
				free($2);
				YYERROR;
			}

			if ((svc = service_findbyname(conf, $2)) == NULL) {
				yyerror("relay %s for unknown service %s",
				    rlay->name, $2);
				free($2);
				YYERROR;
			}
			free($2);
			h = TAILQ_FIRST(&svc->virts);
			bcopy(&h->ss, &rlay->dstss, sizeof(rlay->dstss));
			rlay->dstport = h->port;
			rlay->dstretry = $3;
		}
		| TABLE STRING dstmode docheck {
			struct table	*dsttable;

			if ((dsttable = table_findbyname(conf, $2)) == NULL) {
				yyerror("relay %d for unknown table %s",
				    rlay->name, $2);
				free($2);
				YYERROR;
			}
			free($2);
			rlay->dsttable = dsttable;
			rlay->dstmode = $3;
			rlay->dstcheck = $4;
			rlay->dsttable->flags |= F_USED;
		}
		| PROTO STRING {
			struct protocol *p;

			TAILQ_FOREACH(p, &conf->protos, entry)
				if (!strcmp(p->name, $2))
					break;
			if (p == NULL) {
				yyerror("no such protocol: %s", $2);
				free($2);
				YYERROR;
			}
			p->flags |= F_USED;
			rlay->proto = p;
			free($2);
		}
		| NAT LOOKUP retry	{
			rlay->flags |= F_NATLOOK;
			rlay->dstretry = $3;
		}
		| TIMEOUT number		{ rlay->timeout.tv_sec = $2; }
		| DISABLE			{ rlay->flags |= F_DISABLE; }
		;

dstmode		: /* empty */		{ $$ = RELAY_DSTMODE_DEFAULT; }
		| LOADBALANCE		{ $$ = RELAY_DSTMODE_LOADBALANCE; }
		| ROUNDROBIN		{ $$ = RELAY_DSTMODE_ROUNDROBIN; }
		| HASH			{ $$ = RELAY_DSTMODE_HASH; }
		;

docheck		: /* empty */		{ $$ = 1; }
		| NO CHECK		{ $$ = 0; }
		;

interface	: /*empty*/		{ $$ = NULL; }
		| INTERFACE STRING	{ $$ = $2; }
		;

host		: HOST STRING retry {
			struct address *a;
			struct addresslist al;

			if (($$ = calloc(1, sizeof(*($$)))) == NULL)
				fatal("out of memory");

			TAILQ_INIT(&al);
			if (host($2, &al, 1, 0, NULL) <= 0) {
				yyerror("invalid host %s", $2);
				free($2);
				free($$);
				YYERROR;
			}
			a = TAILQ_FIRST(&al);
			memcpy(&$$->ss, &a->ss, sizeof($$->ss));
			free(a);

			if (strlcpy($$->name, $2, sizeof($$->name)) >=
			    sizeof($$->name)) {
				yyerror("host name truncated");
				free($2);
				free($$);
				YYERROR;
			}
			free($2);
			$$->id = last_host_id++;
			$$->retry = $3;
			if (last_host_id == INT_MAX) {
				yyerror("too many hosts defined");
				free($$);
				YYERROR;
			}
		}
		;

retry		: /* nothing */		{ $$ = 0; }
		| RETRY number		{ $$ = $2; }
		;

timeout		: number
		{
			$$.tv_sec = $1 / 1000;
			$$.tv_usec = ($1 % 1000) * 1000;
		}
		;

log		: /* empty */		{ $$ = 0; }
		| LOG			{ $$ = 1; }
		;

comma		: ','
		| /* empty */
		;

optnl		: '\n' optnl
		|
		;

nl		: '\n' optnl
		;

%%

struct keywords {
	const char	*k_name;
	int		 k_val;
};

int
yyerror(const char *fmt, ...)
{
	va_list	ap;

	errors = 1;
	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", infile, yylval.lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	return (0);
}

int
kw_cmp(const void *k, const void *e)
{

	return (strcmp(k, ((const struct keywords *)e)->k_name));
}

int
lookup(char *s)
{
	/* this has to be sorted always */
	static const struct keywords keywords[] = {
		{ "all",		ALL },
		{ "append",		APPEND },
		{ "backlog",		BACKLOG },
		{ "backup",		BACKUP },
		{ "buffer",		BUFFER },
		{ "cache",		CACHE },
		{ "change",		CHANGE },
		{ "check",		CHECK },
		{ "ciphers",		CIPHERS },
		{ "code",		CODE },
		{ "cookie",		COOKIE },
		{ "demote",		DEMOTE },
		{ "digest",		DIGEST },
		{ "disable",		DISABLE },
		{ "expect",		EXPECT },
		{ "external",		EXTERNAL },
		{ "filter",		FILTER },
		{ "forward",		FORWARD },
		{ "from",		FROM },
		{ "hash",		HASH },
		{ "header",		HEADER },
		{ "host",		HOST },
		{ "icmp",		ICMP },
		{ "interface",		INTERFACE },
		{ "interval",		INTERVAL },
		{ "ip",			IP },
		{ "listen",		LISTEN },
		{ "loadbalance",	LOADBALANCE },
		{ "log",		LOG },
		{ "lookup",		LOOKUP },
		{ "mark",		MARK },
		{ "marked",		MARKED },
		{ "nat",		NAT },
		{ "no",			NO },
		{ "nodelay",		NODELAY },
		{ "nothing",		NOTHING },
		{ "on",			ON },
		{ "path",		PATH },
		{ "port",		PORT },
		{ "prefork",		PREFORK },
		{ "protocol",		PROTO },
		{ "real",		REAL },
		{ "relay",		RELAY },
		{ "remove",		REMOVE },
		{ "request",		REQUEST },
		{ "response",		RESPONSE },
		{ "retry",		RETRY },
		{ "roundrobin",		ROUNDROBIN },
		{ "sack",		SACK },
		{ "send",		SEND },
		{ "service",		SERVICE },
		{ "session",		SESSION },
		{ "socket",		SOCKET },
		{ "ssl",		SSL },
		{ "sticky-address",	STICKYADDR },
		{ "table",		TABLE },
		{ "tag",		TAG },
		{ "tcp",		TCP },
		{ "timeout",		TIMEOUT },
		{ "to",			TO },
		{ "updates",		UPDATES },
		{ "url",		URL },
		{ "virtual",		VIRTUAL }
	};
	const struct keywords	*p;

	p = bsearch(s, keywords, sizeof(keywords)/sizeof(keywords[0]),
	    sizeof(keywords[0]), kw_cmp);

	if (p)
		return (p->k_val);
	else
		return (STRING);
}

#define MAXPUSHBACK	128

char	*parsebuf;
int	 parseindex;
char	 pushback_buffer[MAXPUSHBACK];
int	 pushback_index = 0;

int
lgetc(FILE *f, int *keep)
{
	int	c, next;

	*keep = 0;
	if (parsebuf) {
		/* Read character from the parsebuffer instead of input. */
		if (parseindex >= 0) {
			c = parsebuf[parseindex++];
			if (c != '\0')
				return (c);
			parsebuf = NULL;
		} else
			parseindex++;
	}

	if (pushback_index)
		return (pushback_buffer[--pushback_index]);

	while ((c = getc(f)) == '\\') {
		next = getc(f);
		if (next == 'n') {
			*keep = 1;
			c = '\n';
			break;
		} else if (next == 'r') {
			*keep = 1;
			c = '\r';
			break;
		} else if (next != '\n') {
			c = next;
			break;
		}
		yylval.lineno = lineno;
		lineno++;
	}
	if (c == '\t' || c == ' ') {
		/* Compress blanks to a single space. */
		do {
			c = getc(f);
		} while (c == '\t' || c == ' ');
		ungetc(c, f);
		c = ' ';
	}

	return (c);
}

int
lungetc(int c)
{
	if (c == EOF)
		return (EOF);
	if (parsebuf) {
		parseindex--;
		if (parseindex >= 0)
			return (c);
	}
	if (pushback_index < MAXPUSHBACK-1)
		return (pushback_buffer[pushback_index++] = c);
	else
		return (EOF);
}

int
findeol(void)
{
	int	c;
	int	k;

	parsebuf = NULL;
	pushback_index = 0;

	/* skip to either EOF or the first real EOL */
	while (1) {
		c = lgetc(fin, &k);
		if (c == '\n' && k == 0) {
			lineno++;
			break;
		}
		if (c == EOF)
			break;
	}
	return (ERROR);
}

int
yylex(void)
{
	char	 buf[8096];
	char	*p, *val;
	int	 endc, c;
	int	 token;
	int	 keep;

top:
	p = buf;
	while ((c = lgetc(fin, &keep)) == ' ')
		; /* nothing */

	yylval.lineno = lineno;
	if (c == '#')
		do {
			while ((c = lgetc(fin, &keep)) != '\n' && c != EOF)
				; /* nothing */
		} while (keep == 1);
	if (c == '$' && parsebuf == NULL) {
		while (1) {
			if ((c = lgetc(fin, &keep)) == EOF)
				return (0);

			if (p + 1 >= buf + sizeof(buf) - 1) {
				yyerror("string too long");
				return (findeol());
			}
			if (isalnum(c) || c == '_') {
				*p++ = (char)c;
				continue;
			}
			*p = '\0';
			lungetc(c);
			break;
		}
		val = symget(buf);
		if (val == NULL) {
			yyerror("macro '%s' not defined", buf);
			return (findeol());
		}
		parsebuf = val;
		parseindex = 0;
		goto top;
	}

	switch (c) {
	case '\'':
	case '"':
		endc = c;
		while (1) {
			if ((c = lgetc(fin, &keep)) == EOF)
				return (0);
			if (c == endc) {
				*p = '\0';
				break;
			}
			if (c == '\n' && keep == 0) {
				lineno++;
				continue;
			}
			if (p + 1 >= buf + sizeof(buf) - 1) {
				yyerror("string too long");
				return (findeol());
			}
			*p++ = (char)c;
		}
		yylval.v.string = strdup(buf);
		if (yylval.v.string == NULL)
			errx(1, "yylex: strdup");
		return (STRING);
	}

#define allowed_in_string(x) \
	(isalnum(x) || (ispunct(x) && x != '(' && x != ')' && \
	x != '{' && x != '}' && \
	x != '!' && x != '=' && x != '#' && \
	x != ','))

	if (isalnum(c) || c == ':' || c == '_') {
		do {
			*p++ = c;
			if ((unsigned)(p-buf) >= sizeof(buf)) {
				yyerror("string too long");
				return (findeol());
			}
		} while ((c = lgetc(fin, &keep)) != EOF &&
		    (allowed_in_string(c)));
		lungetc(c);
		*p = '\0';
		if ((token = lookup(buf)) == STRING)
			if ((yylval.v.string = strdup(buf)) == NULL)
				err(1, "yylex: strdup");
		return (token);
	}
	if (c == '\n') {
		yylval.lineno = lineno;
		lineno++;
	}
	if (c == EOF)
		return (0);
	return (c);
}

struct hoststated *
parse_config(const char *filename, int opts)
{
	struct sym	*sym, *next;

	if ((conf = calloc(1, sizeof(*conf))) == NULL)
		return (NULL);

	TAILQ_INIT(&conf->services);
	TAILQ_INIT(&conf->tables);
	TAILQ_INIT(&conf->protos);
	TAILQ_INIT(&conf->relays);

	memset(&conf->empty_table, 0, sizeof(conf->empty_table));
	conf->empty_table.id = EMPTY_TABLE;
	conf->empty_table.flags |= F_DISABLE;
	(void)strlcpy(conf->empty_table.name, "empty",
	    sizeof(conf->empty_table.name));

	bzero(&conf->proto_default, sizeof(conf->proto_default));
	conf->proto_default.flags = F_USED;
	conf->proto_default.cache = RELAY_CACHESIZE;
	conf->proto_default.type = RELAY_PROTO_TCP;
	(void)strlcpy(conf->proto_default.name, "default",
	    sizeof(conf->proto_default.name));
	RB_INIT(&conf->proto_default.request_tree);
	RB_INIT(&conf->proto_default.response_tree);
	TAILQ_INSERT_TAIL(&conf->protos, &conf->proto_default, entry);

	conf->timeout.tv_sec = CHECK_TIMEOUT / 1000;
	conf->timeout.tv_usec = (CHECK_TIMEOUT % 1000) * 1000;
	conf->interval.tv_sec = CHECK_INTERVAL;
	conf->interval.tv_usec = 0;
	conf->prefork_relay = RELAY_NUMPROC;
	conf->statinterval.tv_sec = RELAY_STATINTERVAL;
	conf->opts = opts;

	if ((fin = fopen(filename, "r")) == NULL) {
		warn("%s", filename);
		free(conf);
		return (NULL);
	}
	infile = filename;
	setservent(1);
	yyparse();
	endservent();
	fclose(fin);

	/* Free macros and check which have not been used. */
	for (sym = TAILQ_FIRST(&symhead); sym != NULL; sym = next) {
		next = TAILQ_NEXT(sym, entries);
		if ((conf->opts & HOSTSTATED_OPT_VERBOSE) && !sym->used)
			fprintf(stderr, "warning: macro '%s' not "
			    "used\n", sym->nam);
		if (!sym->persist) {
			free(sym->nam);
			free(sym->val);
			TAILQ_REMOVE(&symhead, sym, entries);
			free(sym);
		}
	}

	if (TAILQ_EMPTY(&conf->services) && TAILQ_EMPTY(&conf->relays)) {
		log_warnx("no services, nothing to do");
		errors++;
	}

	if (timercmp(&conf->timeout, &conf->interval, >=)) {
		log_warnx("global timeout exceeds interval");
		errors++;
	}

	/* Verify that every table is used */
	TAILQ_FOREACH(table, &conf->tables, entry) {
		if (!(table->flags & F_USED)) {
			log_warnx("unused table: %s", table->name);
			errors++;
		}
		if (timercmp(&table->timeout, &conf->interval, >=)) {
			log_warnx("table timeout exceeds interval: %s",
			    table->name);
			errors++;
		}
	}

	/* Verify that every non-default protocol is used */
	TAILQ_FOREACH(proto, &conf->protos, entry) {
		if (!(proto->flags & F_USED)) {
			log_warnx("unused protocol: %s", proto->name);
			errors++;
		}
	}

	if (errors) {
		free(conf);
		return (NULL);
	}

	return (conf);
}

int
symset(const char *nam, const char *val, int persist)
{
	struct sym	*sym;

	for (sym = TAILQ_FIRST(&symhead); sym && strcmp(nam, sym->nam);
	    sym = TAILQ_NEXT(sym, entries))
		;	/* nothing */

	if (sym != NULL) {
		if (sym->persist == 1)
			return (0);
		else {
			free(sym->nam);
			free(sym->val);
			TAILQ_REMOVE(&symhead, sym, entries);
			free(sym);
		}
	}
	if ((sym = calloc(1, sizeof(*sym))) == NULL)
		return (-1);

	sym->nam = strdup(nam);
	if (sym->nam == NULL) {
		free(sym);
		return (-1);
	}
	sym->val = strdup(val);
	if (sym->val == NULL) {
		free(sym->nam);
		free(sym);
		return (-1);
	}
	sym->used = 0;
	sym->persist = persist;
	TAILQ_INSERT_TAIL(&symhead, sym, entries);
	return (0);
}

int
cmdline_symset(char *s)
{
	char	*sym, *val;
	int	ret;
	size_t	len;

	if ((val = strrchr(s, '=')) == NULL)
		return (-1);

	len = strlen(s) - strlen(val) + 1;
	if ((sym = malloc(len)) == NULL)
		errx(1, "cmdline_symset: malloc");

	(void)strlcpy(sym, s, len);

	ret = symset(sym, val + 1, 1);
	free(sym);

	return (ret);
}

char *
symget(const char *nam)
{
	struct sym	*sym;

	TAILQ_FOREACH(sym, &symhead, entries)
		if (strcmp(nam, sym->nam) == 0) {
			sym->used = 1;
			return (sym->val);
		}
	return (NULL);
}

struct address *
host_v4(const char *s)
{
	struct in_addr		 ina;
	struct sockaddr_in	*sain;
	struct address		*h;

	bzero(&ina, sizeof(ina));
	if (inet_pton(AF_INET, s, &ina) != 1)
		return (NULL);

	if ((h = calloc(1, sizeof(*h))) == NULL)
		fatal(NULL);
	sain = (struct sockaddr_in *)&h->ss;
	sain->sin_len = sizeof(struct sockaddr_in);
	sain->sin_family = AF_INET;
	sain->sin_addr.s_addr = ina.s_addr;

	return (h);
}

struct address *
host_v6(const char *s)
{
	struct in6_addr		 ina6;
	struct sockaddr_in6	*sin6;
	struct address		*h;

	bzero(&ina6, sizeof(ina6));
	if (inet_pton(AF_INET6, s, &ina6) != 1)
		return (NULL);

	if ((h = calloc(1, sizeof(*h))) == NULL)
		fatal(NULL);
	sin6 = (struct sockaddr_in6 *)&h->ss;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_family = AF_INET6;
	memcpy(&sin6->sin6_addr, &ina6, sizeof(ina6));

	return (h);
}

int
host_dns(const char *s, struct addresslist *al, int max,
	 in_port_t port, const char *ifname)
{
	struct addrinfo		 hints, *res0, *res;
	int			 error, cnt = 0;
	struct sockaddr_in	*sain;
	struct sockaddr_in6	*sin6;
	struct address		*h;

	bzero(&hints, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM; /* DUMMY */
	error = getaddrinfo(s, NULL, &hints, &res0);
	if (error == EAI_AGAIN || error == EAI_NODATA || error == EAI_NONAME)
		return (0);
	if (error) {
		log_warnx("host_dns: could not parse \"%s\": %s", s,
		    gai_strerror(error));
		return (-1);
	}

	for (res = res0; res && cnt < max; res = res->ai_next) {
		if (res->ai_family != AF_INET &&
		    res->ai_family != AF_INET6)
			continue;
		if ((h = calloc(1, sizeof(*h))) == NULL)
			fatal(NULL);

		h->port = port;
		if (ifname != NULL) {
			if (strlcpy(h->ifname, ifname, sizeof(h->ifname)) >=
			    sizeof(h->ifname))
				log_warnx("host_dns: interface name truncated");
			return (-1);
		}
		h->ss.ss_family = res->ai_family;
		if (res->ai_family == AF_INET) {
			sain = (struct sockaddr_in *)&h->ss;
			sain->sin_len = sizeof(struct sockaddr_in);
			sain->sin_addr.s_addr = ((struct sockaddr_in *)
			    res->ai_addr)->sin_addr.s_addr;
		} else {
			sin6 = (struct sockaddr_in6 *)&h->ss;
			sin6->sin6_len = sizeof(struct sockaddr_in6);
			memcpy(&sin6->sin6_addr, &((struct sockaddr_in6 *)
			    res->ai_addr)->sin6_addr, sizeof(struct in6_addr));
		}

		TAILQ_INSERT_HEAD(al, h, entry);
		cnt++;
	}
	if (cnt == max && res) {
		log_warnx("host_dns: %s resolves to more than %d hosts",
		    s, max);
	}
	freeaddrinfo(res0);
	return (cnt);
}

int
host(const char *s, struct addresslist *al, int max,
    in_port_t port, const char *ifname)
{
	struct address *h;

	h = host_v4(s);

	/* IPv6 address? */
	if (h == NULL)
		h = host_v6(s);

	if (h != NULL) {
		h->port = port;
		if (ifname != NULL) {
			if (strlcpy(h->ifname, ifname, sizeof(h->ifname)) >=
			    sizeof(h->ifname)) {
				log_warnx("host: interface name truncated");
				return (-1);
			}
		}

		TAILQ_INSERT_HEAD(al, h, entry);
		return (1);
	}

	return (host_dns(s, al, max, port, ifname));
}
