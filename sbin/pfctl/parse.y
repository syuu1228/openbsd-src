/*	$OpenBSD: src/sbin/pfctl/parse.y,v 1.72 2002/06/07 18:24:33 itojun Exp $	*/

/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2001 Daniel Hartmeier.  All rights reserved.
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
%{
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <net/pfvar.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <pwd.h>

#include "pfctl_parser.h"

static struct pfctl *pf = NULL;
static FILE *fin = NULL;
static int debug = 0;
static int lineno = 1;
static int errors = 0;
static int natmode = 0;

struct node_if {
	char			 ifname[IFNAMSIZ];
	u_int8_t		 not;
	struct node_if		*next;
};

struct node_proto {
	u_int8_t		 proto;
	struct node_proto	*next;
};

struct node_host {
	struct pf_addr_wrap	 addr;
	struct pf_addr		 mask;
	u_int8_t		 af;
	u_int8_t		 not;
	u_int8_t		 noroute;
	struct node_host	*next;
	u_int32_t		 ifindex;
};

struct node_port {
	u_int16_t		 port[2];
	u_int8_t		 op;
	struct node_port	*next;
};

struct node_uid {
	uid_t			 uid[2];
	u_int8_t		 op;
	struct node_uid		*next;
};

struct node_gid {
	gid_t			 gid[2];
	u_int8_t		 op;
	struct node_gid		*next;
};

struct node_icmp {
	u_int8_t		 code;
	u_int8_t		 type;
	u_int8_t		 proto;
	struct node_icmp	*next;
};

struct peer {
	struct node_host	*host;
	struct node_port	*port;
};

int	rule_consistent(struct pf_rule *);
int	yyparse(void);
void	ipmask(struct pf_addr *, u_int8_t);
void	expand_rule(struct pf_rule *, struct node_if *, struct node_proto *,
    struct node_host *, struct node_port *, struct node_host *,
    struct node_port *, struct node_uid *, struct node_gid *,
    struct node_icmp *);

struct sym {
	struct sym *next;
	char *nam;
	char *val;
};
struct sym *symhead = NULL;

int	symset(char *name, char *val);
char *	symget(char *name);

struct ifaddrs    *ifa0_lookup(char *ifa_name);
struct ifaddrs    *ifa4_lookup(char *ifa_name);
struct ifaddrs    *ifa6_lookup(char *ifa_name);

typedef struct {
	union {
		u_int32_t		number;
		int			i;
		char			*string;
		struct {
			u_int8_t	b1;
			u_int8_t	b2;
			u_int16_t	w;
		}			b;
		struct range {
			int		a;
			int		b;
			int		t;
		}			range;
		struct node_if		*interface;
		struct node_proto	*proto;
		struct node_icmp	*icmp;
		struct node_host	*host;
		struct node_port	*port;
		struct node_uid		*uid;
		struct node_gid		*gid;
		struct peer		peer;
		struct {
			struct peer	src, dst;
		}			fromto;
		struct {
			char		*string;
			struct pf_addr	*addr;
			u_int8_t	rt;
			u_int8_t	af;
		}			route;
		struct redirection {
			struct node_host	*address;
			struct range		 rport;
		}			*redirection;
	} v;
	int lineno;
} YYSTYPE;

%}

%token	PASS BLOCK SCRUB RETURN IN OUT LOG LOGALL QUICK ON FROM TO FLAGS
%token	RETURNRST RETURNICMP RETURNICMP6 PROTO INET INET6 ALL ANY ICMPTYPE
%token  ICMP6TYPE CODE KEEP MODULATE STATE PORT RDR NAT BINAT ARROW NODF
%token	MINTTL IPV6ADDR ERROR ALLOWOPTS FASTROUTE ROUTETO DUPTO NO LABEL
%token	NOROUTE FRAGMENT USER GROUP MAXMSS
%token	<v.string> STRING
%token	<v.number> NUMBER
%token	<v.i>	PORTUNARY PORTBINARY
%type	<v.interface>	interface if_list if_item_not if_item
%type	<v.number>	port icmptype icmp6type minttl uid gid maxmss
%type	<v.i>	no dir log quick af keep nodf allowopts fragment
%type	<v.b>	action flag flags blockspec
%type	<v.range>	dport rport
%type	<v.proto>	proto proto_list proto_item
%type	<v.icmp>	icmpspec icmp_list icmp6_list icmp_item icmp6_item
%type	<v.fromto>	fromto
%type	<v.peer>	ipportspec
%type	<v.host>	ipspec xhost host address host_list IPV6ADDR
%type	<v.port>	portspec port_list port_item
%type	<v.uid>		uids uid_list uid_item
%type	<v.gid>		gids gid_list gid_item
%type	<v.route>	route
%type	<v.redirection>	redirection
%type	<v.string>	label
%%

ruleset		: /* empty */
		| ruleset '\n'
		| ruleset pfrule '\n'
		| ruleset natrule '\n'
		| ruleset binatrule '\n'
		| ruleset rdrrule '\n'
		| ruleset varset '\n'
		| ruleset error '\n'		{ errors++; }
		;

varset		: STRING PORTUNARY STRING
		{
			if (pf->opts & PF_OPT_VERBOSE)
				printf("%s = %s\n", $1, $3);
			if (symset($1, $3) == -1) {
				yyerror("cannot store variable %s", $1);
				YYERROR;
			}
		}
		;

pfrule		: action dir log quick interface route af proto fromto
		  uids gids flags icmpspec keep fragment nodf minttl
		  maxmss allowopts label
		{
			struct pf_rule r;

			if (natmode) {
				yyerror("filter rule not permitted in nat mode");
				YYERROR;
			}
			memset(&r, 0, sizeof(r));

			r.action = $1.b1;
			if ($1.b2)
				r.rule_flag |= PFRULE_RETURNRST;
			else
				r.return_icmp = $1.w;
			r.direction = $2;
			r.log = $3;
			r.quick = $4;


			r.af = $7;
			r.flags = $12.b1;
			r.flagset = $12.b2;

			r.keep_state = $14;

			if ($15)
				r.rule_flag |= PFRULE_FRAGMENT;
			if ($16)
				r.rule_flag |= PFRULE_NODF;
			if ($17)
				r.min_ttl = $17;
			if ($18)
				r.max_mss = $18;
			r.allow_opts = $19;

			if ($6.rt) {
				r.rt = $6.rt;
				if ($6.string) {
					memcpy(r.rt_ifname, $6.string,
					    sizeof(r.rt_ifname));
					free($6.string);
				}
				if ($6.addr) {
					if (!r.af)
						r.af = $6.af;
					else if (r.af != $6.af) {
						yyerror("address family"
						    " mismatch");
						YYERROR;
					}
					memcpy(&r.rt_addr, $6.addr,
					    sizeof(r.rt_addr));
					free($6.addr);
				}
			}

			if ($20) {
				if (strlen($20) >= PF_RULE_LABEL_SIZE) {
					yyerror("rule label too long (max "
					    "%d chars)", PF_RULE_LABEL_SIZE-1);
					YYERROR;
				}
				strlcpy(r.label, $20, sizeof(r.label));
				free($20);
			}

			expand_rule(&r, $5, $8, $9.src.host, $9.src.port,
			    $9.dst.host, $9.dst.port, $10, $11, $13);
		}
		;

action		: PASS			{ $$.b1 = PF_PASS; $$.b2 = $$.w = 0; }
		| BLOCK blockspec	{ $$ = $2; $$.b1 = PF_DROP; }
		| SCRUB			{ $$.b1 = PF_SCRUB; $$.b2 = $$.w = 0; }
		;

blockspec	: /* empty */		{ $$.b2 = 0; $$.w = 0; }
		| RETURNRST		{ $$.b2 = 1; $$.w = 0;}
		| RETURNICMP		{
			$$.b2 = 0;
			$$.w = (ICMP_UNREACH << 8) | ICMP_UNREACH_PORT;
		}
		| RETURNICMP6		{
			$$.b2 = 0;
			$$.w = (ICMP6_DST_UNREACH << 8) |
			    ICMP6_DST_UNREACH_NOPORT;
		}
		| RETURNICMP '(' NUMBER ')'	{
			$$.w = (ICMP_UNREACH << 8) | $3;
			$$.b2 = 0;
		}
		| RETURNICMP '(' STRING ')'	{
			struct icmpcodeent *p;

			if ((p = geticmpcodebyname(ICMP_UNREACH, $3,
			    AF_INET)) == NULL) {
				yyerror("unknown icmp code %s", $3);
				YYERROR;
			}
			$$.w = (p->type << 8) | p->code;
			$$.b2 = 0;
		}
		| RETURNICMP6 '(' NUMBER ')'	{
			$$.w = (ICMP6_DST_UNREACH << 8) | $3;
			$$.b2 = 0;
		}
		| RETURNICMP6 '(' STRING ')'	{
			struct icmpcodeent *p;

			if ((p = geticmpcodebyname(ICMP6_DST_UNREACH, $3,
			    AF_INET6)) == NULL) {
				yyerror("unknown icmp code %s", $3);
				YYERROR;
			}
			$$.w = (p->type << 8) | p->code;
			$$.b2 = 0;
		}
		;

dir		: IN			{ $$ = PF_IN; }
		| OUT				{ $$ = PF_OUT; }
		;

log		: /* empty */			{ $$ = 0; }
		| LOG				{ $$ = 1; }
		| LOGALL			{ $$ = 2; }
		;

quick		: /* empty */			{ $$ = 0; }
		| QUICK				{ $$ = 1; }
		;

interface	: /* empty */			{ $$ = NULL; }
		| ON if_item_not		{ $$ = $2; }
		| ON '{' if_list '}'		{ $$ = $3; }
		;

if_list		: if_item_not			{ $$ = $1; }
		| if_list ',' if_item_not	{ $3->next = $1; $$ = $3; }
		;

if_item_not	: '!' if_item			{ $$ = $2; $$->not = 1; }
		| if_item			{ $$ = $1; }

if_item		: STRING			{
			if (ifa0_lookup($1) == 0) {
				yyerror("unknown interface %s", $1);
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_if));
			if ($$ == NULL)
				err(1, "if_item: malloc");
			strlcpy($$->ifname, $1, IFNAMSIZ);
			$$->not = 0;
			$$->next = NULL;
		}
		;

af		: /* empty */			{ $$ = 0; }
		| INET				{ $$ = AF_INET; }
		| INET6				{ $$ = AF_INET6; }

proto		: /* empty */			{ $$ = NULL; }
		| PROTO proto_item		{ $$ = $2; }
		| PROTO '{' proto_list '}'	{ $$ = $3; }
		;

proto_list	: proto_item			{ $$ = $1; }
		| proto_list ',' proto_item	{ $3->next = $1; $$ = $3; }
		;

proto_item	: NUMBER			{
			struct protoent *p;

			if ((p = getprotobynumber($1)) == NULL) {
				yyerror("unknown protocol %d", $1);
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_proto));
			if ($$ == NULL)
				err(1, "proto_item: malloc");
			$$->proto = p->p_proto;
			$$->next = NULL;
		}
		| STRING			{
			struct protoent *p;

			if ((p = getprotobyname($1)) == NULL) {
				yyerror("unknown protocol %s", $1);
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_proto));
			if ($$ == NULL)
				err(1, "proto_item: malloc");
			$$->proto = p->p_proto;
			$$->next = NULL;
		}
		;

fromto		: ALL				{
			$$.src.host = NULL;
			$$.src.port = NULL;
			$$.dst.host = NULL;
			$$.dst.port = NULL;
		}
		| FROM ipportspec TO ipportspec	{
			$$.src = $2;
			$$.dst = $4;
		}
		;

ipportspec	: ipspec			{ $$.host = $1; $$.port = NULL; }
		| ipspec PORT portspec		{
			$$.host = $1;
			$$.port = $3;
		}
		;

ipspec		: ANY				{ $$ = NULL; }
		| xhost				{ $$ = $1; }
		| '{' host_list '}'		{ $$ = $2; }
		;

host_list	: xhost				{ $$ = $1; }
		| host_list ',' xhost		{
			/* both $1 and $3 may be lists, so join them */
			$$ = $3;
			while ($3->next)
				$3 = $3->next;
			$3->next = $1;
		}
		;

xhost		: '!' host			{ $$ = $2; $$->not = 1; }
		| host				{ $$ = $1; }
		| NOROUTE			{
			$$ = calloc(1, sizeof(struct node_host));
			if ($$ == NULL)
				err(1, "xhost: calloc");
			$$->noroute = 1;
		}
		;

host		: address			{
			struct node_host *n;
			for (n = $1; n; n = n->next)
				if (n->af == AF_INET)
					ipmask(&n->mask, 32);
				else
					ipmask(&n->mask, 128);
			$$ = $1;
		}
		| address '/' NUMBER		{
			struct node_host *n;
			for (n = $1; n; n = n->next) {
				if ($1->af == AF_INET) {
					if ($3 < 0 || $3 > 32) {
						yyerror(
						    "illegal netmask value %d",
						    $3);
						YYERROR;
					}
				} else {
					if ($3 < 0 || $3 > 128) {
						yyerror(
						    "illegal netmask value %d",
						    $3);
						YYERROR;
					}
				}
				ipmask(&n->mask, $3);
			}
			$$ = $1;
		}
		;

address		: '(' STRING ')'		{
			$$ = calloc(1, sizeof(struct node_host));
			if ($$ == NULL)
				err(1, "address: calloc");
			$$->af = 0;
			$$->addr.addr_dyn = (struct pf_addr_dyn *)1;
			strncpy($$->addr.addr.pfa.ifname, $2,
			    sizeof($$->addr.addr.pfa.ifname));
		}
		| STRING			{
			if (ifa0_lookup($1)) {
				struct ifaddrs *ifa;

				/* an interface with this name exists */
				if ((ifa = ifa4_lookup($1))) {
					struct sockaddr_in *sin =
					    (struct sockaddr_in *)
					    ifa->ifa_addr;

					$$ = calloc(1,
					    sizeof(struct node_host));
					if ($$ == NULL)
						err(1, "address: calloc");
					$$->af = AF_INET;
					$$->addr.addr_dyn = NULL;
					memcpy(&$$->addr.addr, &sin->sin_addr,
					    sizeof(u_int32_t));
				} else if ((ifa = ifa6_lookup($1))) {
					struct sockaddr_in6 *sin6 =
					    (struct sockaddr_in6 *)
					    ifa->ifa_addr;

					$$ = calloc(1,
					    sizeof(struct node_host));
					if ($$ == NULL)
						err(1, "address: calloc");
					$$->af = AF_INET6;
					$$->addr.addr_dyn = NULL;
					memcpy(&$$->addr.addr, &sin6->sin6_addr,
					    sizeof(struct pf_addr));
				} else {
					yyerror("interface %s has no IP "
					    "addresses", $1);
					YYERROR;
				}
			} else {
				struct node_host *h = NULL, *n;
				struct addrinfo hints, *res0, *res;
				int error;

				memset(&hints, 0, sizeof(hints));
				hints.ai_family = PF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM; /* DUMMY */
				error = getaddrinfo($1, NULL, &hints, &res0);
				if (error) {
					yyerror("cannot resolve %s: %s",
					    $1, gai_strerror(error));
					YYERROR;
				}
				for (res = res0; res; res = res->ai_next) {
					if (res->ai_family != AF_INET &&
					    res->ai_family != AF_INET6)
						continue;
					n = calloc(1, sizeof(struct node_host));
					if (n == NULL)
						err(1, "address: calloc");
					n->af = res->ai_family;
					n->addr.addr_dyn = NULL;
					if (res->ai_family == AF_INET)
						memcpy(&n->addr.addr,
						&((struct sockaddr_in *)
						    res->ai_addr)
						    ->sin_addr.s_addr,
						sizeof(struct in_addr));
					else {
						memcpy(&n->addr.addr,
						&((struct sockaddr_in6 *)
						    res->ai_addr)
						    ->sin6_addr.s6_addr,
						sizeof(struct in6_addr));
						n->ifindex =
						    ((struct sockaddr_in6 *)
						    res->ai_addr)
						    ->sin6_scope_id;
					}
					n->next = h;
					h = n;
				}
				freeaddrinfo(res0);
				if (h == NULL) {
					yyerror("no IP address found for %s", $1);
					YYERROR;
				}
				$$ = h;
			}
		}
		| NUMBER '.' NUMBER '.' NUMBER '.' NUMBER {
			if ($1 < 0 || $3 < 0 || $5 < 0 || $7 < 0 ||
			    $1 > 255 || $3 > 255 || $5 > 255 || $7 > 255) {
				yyerror("illegal ip address %d.%d.%d.%d",
				    $1, $3, $5, $7);
				YYERROR;
			}
			$$ = calloc(1, sizeof(struct node_host));
			if ($$ == NULL)
				err(1, "address: calloc");
			$$->af = AF_INET;
			$$->addr.addr_dyn = NULL;
			$$->addr.addr.addr32[0] = htonl(($1 << 24) |
			    ($3 << 16) | ($5 << 8) | $7);
		}
		| IPV6ADDR			{ $$ = $1; }
		;

portspec	: port_item			{ $$ = $1; }
		| '{' port_list '}'		{ $$ = $2; }
		;

port_list	: port_item			{ $$ = $1; }
		| port_list ',' port_item	{ $3->next = $1; $$ = $3; }
		;

port_item	: port				{
			$$ = malloc(sizeof(struct node_port));
			if ($$ == NULL)
				err(1, "port_item: malloc");
			$$->port[0] = $1;
			$$->port[1] = $1;
			$$->op = PF_OP_EQ;
			$$->next = NULL;
		}
		| PORTUNARY port		{
			$$ = malloc(sizeof(struct node_port));
			if ($$ == NULL)
				err(1, "port_item: malloc");
			$$->port[0] = $2;
			$$->port[1] = $2;
			$$->op = $1;
			$$->next = NULL;
		}
		| port PORTBINARY port		{
			$$ = malloc(sizeof(struct node_port));
			if ($$ == NULL)
				err(1, "port_item: malloc");
			$$->port[0] = $1;
			$$->port[1] = $3;
			$$->op = $2;
			$$->next = NULL;
		}
		;

port		: NUMBER			{
			if ($1 < 0 || $1 > 65535) {
				yyerror("illegal port value %d", $1);
				YYERROR;
			}
			$$ = htons($1);
		}
		| STRING			{
			struct servent *s = NULL;

			s = getservbyname($1, "tcp");
			if (s == NULL)
				s = getservbyname($1, "udp");
			if (s == NULL) {
				yyerror("unknown protocol %s", $1);
				YYERROR;
			}
			$$ = s->s_port;
		}
		;

uids		: /* empty */			{ $$ = NULL; }
		| USER uid_item			{ $$ = $2; }
		| USER '{' uid_list '}'		{ $$ = $3; }
		;

uid_list	: uid_item			{ $$ = $1; }
		| uid_list ',' uid_item		{ $3->next = $1; $$ = $3; }
		;

uid_item	: uid				{
			$$ = malloc(sizeof(struct node_uid));
			if ($$ == NULL)
				err(1, "uid_item: malloc");
			$$->uid[0] = $1;
			$$->uid[1] = $1;
			$$->op = PF_OP_EQ;
			$$->next = NULL;
		}
		| PORTUNARY uid			{
			if ($2 == UID_MAX && $1 != PF_OP_EQ && $1 != PF_OP_NE) {
				yyerror("user unknown requires operator = or !=");
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_uid));
			if ($$ == NULL)
				err(1, "uid_item: malloc");
			$$->uid[0] = $2;
			$$->uid[1] = $2;
			$$->op = $1;
			$$->next = NULL;
		}
		| uid PORTBINARY uid		{
			if ($1 == UID_MAX || $3 == UID_MAX) {
				yyerror("user unknown requires operator = or !=");
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_uid));
			if ($$ == NULL)
				err(1, "uid_item: malloc");
			$$->uid[0] = $1;
			$$->uid[1] = $3;
			$$->op = $2;
			$$->next = NULL;
		}
		;

uid		: NUMBER			{
			if ($1 < 0 || $1 >= UID_MAX) {
				yyerror("illegal uid value %d", $1);
				YYERROR;
			}
			$$ = $1;
		}
		| STRING			{
			if (!strcmp($1, "unknown"))
				$$ = UID_MAX;
			else {
				struct passwd *pw;

				if ((pw = getpwnam($1)) == NULL) {
					yyerror("unknown user %s", $1);
					YYERROR;
				}
				$$ = pw->pw_uid;
			}
		}
		;

gids		: /* empty */			{ $$ = NULL; }
		| GROUP gid_item		{ $$ = $2; }
		| GROUP '{' gid_list '}'	{ $$ = $3; }
		;

gid_list	: gid_item			{ $$ = $1; }
		| gid_list ',' gid_item		{ $3->next = $1; $$ = $3; }
		;

gid_item	: gid				{
			$$ = malloc(sizeof(struct node_gid));
			if ($$ == NULL)
				err(1, "gid_item: malloc");
			$$->gid[0] = $1;
			$$->gid[1] = $1;
			$$->op = PF_OP_EQ;
			$$->next = NULL;
		}
		| PORTUNARY gid			{
			if ($2 == GID_MAX && $1 != PF_OP_EQ && $1 != PF_OP_NE) {
				yyerror("group unknown requires operator = or !=");
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_gid));
			if ($$ == NULL)
				err(1, "gid_item: malloc");
			$$->gid[0] = $2;
			$$->gid[1] = $2;
			$$->op = $1;
			$$->next = NULL;
		}
		| gid PORTBINARY gid		{
			if ($1 == GID_MAX || $3 == GID_MAX) {
				yyerror("group unknown requires operator = or !=");
				YYERROR;
			}
			$$ = malloc(sizeof(struct node_gid));
			if ($$ == NULL)
				err(1, "gid_item: malloc");
			$$->gid[0] = $1;
			$$->gid[1] = $3;
			$$->op = $2;
			$$->next = NULL;
		}
		;

gid		: NUMBER			{
			if ($1 < 0 || $1 >= GID_MAX) {
				yyerror("illegal gid value %d", $1);
				YYERROR;
			}
			$$ = $1;
		}
		| STRING			{
			if (!strcmp($1, "unknown"))
				$$ = GID_MAX;
			else {
				struct passwd *pw;

				if ((pw = getpwnam($1)) == NULL) {
					yyerror("unknown group %s", $1);
					YYERROR;
				}
				$$ = pw->pw_uid;
			}
		}
		;

flag		: STRING			{
			int f;

			if ((f = parse_flags($1)) < 0) {
				yyerror("bad flags %s", $1);
				YYERROR;
			}
			$$.b1 = f;
		}
		;

flags		: /* empty */			{ $$.b1 = 0; $$.b2 = 0; }
		| FLAGS flag			{ $$.b1 = $2.b1; $$.b2 = PF_TH_ALL; }
		| FLAGS flag "/" flag		{ $$.b1 = $2.b1; $$.b2 = $4.b1; }
		| FLAGS "/" flag		{ $$.b1 = 0; $$.b2 = $3.b1; }
		;

icmpspec	: /* empty */			{ $$ = NULL; }
		| ICMPTYPE icmp_item		{ $$ = $2; }
		| ICMPTYPE '{' icmp_list '}'	{ $$ = $3; }
		| ICMP6TYPE icmp6_item		{ $$ = $2; }
		| ICMP6TYPE '{' icmp6_list '}'	{ $$ = $3; }
		;

icmp_list	: icmp_item			{ $$ = $1; }
		| icmp_list ',' icmp_item	{ $3->next = $1; $$ = $3; }
		;

icmp6_list	: icmp6_item			{ $$ = $1; }
		| icmp6_list ',' icmp6_item	{ $3->next = $1; $$ = $3; }
		;

icmp_item	: icmptype		{
			$$ = malloc(sizeof(struct node_icmp));
			if ($$ == NULL)
				err(1, "icmp_item: malloc");
			$$->type = $1;
			$$->code = 0;
			$$->proto = IPPROTO_ICMP;
			$$->next = NULL;
		}
		| icmptype CODE NUMBER	{
			$$ = malloc(sizeof(struct node_icmp));
			if ($$ == NULL)
				err(1, "icmp_item: malloc");
			if ($3 < 0 || $3 > 255) {
				yyerror("illegal icmp code %d", $3);
				YYERROR;
			}
			$$->type = $1;
			$$->code = $3 + 1;
			$$->proto = IPPROTO_ICMP;
			$$->next = NULL;
		}
		| icmptype CODE STRING	{
			struct icmpcodeent *p;

			$$ = malloc(sizeof(struct node_icmp));
			if ($$ == NULL)
				err(1, "icmp_item: malloc");
			$$->type = $1;
			if ((p = geticmpcodebyname($1, $3,
			    AF_INET)) == NULL) {
				yyerror("unknown icmp-code %s", $3);
				YYERROR;
			}
			$$->code = p->code + 1;
			$$->proto = IPPROTO_ICMP;
			$$->next = NULL;
		}
		;

icmp6_item	: icmp6type		{
			$$ = malloc(sizeof(struct node_icmp));
			if ($$ == NULL)
				err(1, "icmp_item: malloc");
			$$->type = $1;
			$$->code = 0;
			$$->proto = IPPROTO_ICMPV6;
			$$->next = NULL;
		}
		| icmp6type CODE NUMBER	{
			$$ = malloc(sizeof(struct node_icmp));
			if ($$ == NULL)
				err(1, "icmp_item: malloc");
			if ($3 < 0 || $3 > 255) {
				yyerror("illegal icmp6 code %d", $3);
				YYERROR;
			}
			$$->type = $1;
			$$->code = $3 + 1;
			$$->proto = IPPROTO_ICMPV6;
			$$->next = NULL;
		}
		| icmp6type CODE STRING	{
			struct icmpcodeent *p;

			$$ = malloc(sizeof(struct node_icmp));
			if ($$ == NULL)
				err(1, "icmp_item: malloc");
			$$->type = $1;
			if ((p = geticmpcodebyname($1, $3,
			    AF_INET6)) == NULL) {
				yyerror("unknown icmp6-code %s", $3);
				YYERROR;
			}
			$$->code = p->code + 1;
			$$->proto = IPPROTO_ICMPV6;
			$$->next = NULL;
		}
		;

icmptype	: STRING			{
			struct icmptypeent *p;

			if ((p = geticmptypebyname($1, AF_INET)) == NULL) {
				yyerror("unknown icmp-type %s", $1);
				YYERROR;
			}
			$$ = p->type + 1;
		}
		| NUMBER			{
			if ($1 < 0 || $1 > 255) {
				yyerror("illegal icmp type %d", $1);
				YYERROR;
			}
			$$ = $1 + 1;
		}
		;

icmp6type	: STRING			{
			struct icmptypeent *p;

			if ((p = geticmptypebyname($1, AF_INET6)) == NULL) {
				yyerror("unknown ipv6-icmp-type %s", $1);
				YYERROR;
			}
			$$ = p->type + 1;
		}
		| NUMBER			{
			if ($1 < 0 || $1 > 255) {
				yyerror("illegal icmp6 type %d", $1);
				YYERROR;
			}
			$$ = $1 + 1;
		}
		;

keep		: /* empty */			{ $$ = 0; }
		| KEEP STATE			{ $$ = PF_STATE_NORMAL; }
		| MODULATE STATE		{ $$ = PF_STATE_MODULATE; }
		;

fragment	: /* empty */			{ $$ = 0; }
		| FRAGMENT			{ $$ = 1; }

minttl		: /* empty */			{ $$ = 0; }
		| MINTTL NUMBER			{
			if ($2 < 0 || $2 > 255) {
				yyerror("illegal min-ttl value %d", $2);
				YYERROR;
			}
			$$ = $2;
		}
		;

nodf		: /* empty */			{ $$ = 0; }
		| NODF				{ $$ = 1; }
		;

maxmss		: /* empty */			{ $$ = 0; }
		| MAXMSS NUMBER			{
			if ($2 < 0) {
				yyerror("illegal max-mss value %d", $2);
				YYERROR;
			}
			$$ = $2;
		}
		;

allowopts	: /* empty */			{ $$ = 0; }
		| ALLOWOPTS			{ $$ = 1; }

label		: /* empty */			{ $$ = NULL; }
		| LABEL STRING			{
			if (($$ = strdup($2)) == NULL) {
				yyerror("rule label strdup() failed");
				YYERROR;
			}
		}
		;

no		: /* empty */			{ $$ = 0; }
		| NO				{ $$ = 1; }
		;

rport		: port				{
			$$.a = $1;
			$$.b = $$.t = 0;
		}
		| port ':' '*'			{
			$$.a = $1;
			$$.b = 0;
			$$.t = PF_RPORT_RANGE;
		}
		;

redirection	: /* empty */			{ $$ = NULL; }
		| ARROW address			{
			$$ = malloc(sizeof(struct redirection));
			if ($$ == NULL)
				err(1, "redirection: malloc");
			if ($2->next) {
				yyerror("multiple ip addresses");
				YYERROR;
			}
			$$->address = $2;
			$$->rport.a = $$->rport.b = $$->rport.t = 0;
		}
		| ARROW address PORT rport	{
			$$ = malloc(sizeof(struct redirection));
			if ($$ == NULL)
				err(1, "redirection: malloc");
			if ($2->next) {
				yyerror("multiple ip addresses");
				YYERROR;
			}
			$$->address = $2;
			$$->rport = $4;
		}
		;

natrule		: no NAT interface af proto FROM ipspec TO ipspec redirection
		{
			struct pf_nat nat;

			if (!natmode) {
				yyerror("nat rule not permitted in filter mode");
				YYERROR;
			}
			memset(&nat, 0, sizeof(nat));

			nat.no = $1;
			if ($3 != NULL) {
				memcpy(nat.ifname, $3->ifname,
				    sizeof(nat.ifname));
				nat.ifnot = $3->not;
				free($3);
			}
			nat.af = $4;
			if ($5 != NULL) {
				nat.proto = $5->proto;
				free($5);
			}
			if ($7 != NULL && $9 != NULL && $7->af != $9->af) {
				yyerror("nat ip versions must match");
				YYERROR;
			}
			if ($7 != NULL) {
				if ($7->next) {
					yyerror("multiple nat ip addresses");
					YYERROR;
				}
				if ($7->addr.addr_dyn != NULL) {
					if (!nat.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$7->af = nat.af;
				}
				if (nat.af && $7->af != nat.af) {
					yyerror("nat ip versions must match");
					YYERROR;
				}
				nat.af = $7->af;
				memcpy(&nat.saddr, &$7->addr,
				    sizeof(nat.saddr));
				memcpy(&nat.smask, &$7->mask,
				    sizeof(nat.smask));
				nat.snot = $7->not;
				free($7);
			}
			if ($9 != NULL) {
				if ($9->next) {
					yyerror("multiple nat ip addresses");
					YYERROR;
				}
				if ($9->addr.addr_dyn != NULL) {
					if (!nat.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$9->af = nat.af;
				}
				if (nat.af && $9->af != nat.af) {
					yyerror("nat ip versions must match");
					YYERROR;
				}
				nat.af = $9->af;
				memcpy(&nat.daddr, &$9->addr,
				    sizeof(nat.daddr));
				memcpy(&nat.dmask, &$9->mask,
				    sizeof(nat.dmask));
				nat.dnot = $9->not;
				free($9);
			}

			if (nat.no) {
				if ($10 != NULL) {
					yyerror("'no nat' rule does not need '->'");
					YYERROR;
				}
			} else {
				if ($10 == NULL || $10->address == NULL) {
					yyerror("'nat' rule requires '-> address'");
					YYERROR;
				}
				if ($10->address->addr.addr_dyn != NULL) {
					if (!nat.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$10->address->af = nat.af;
				}
				if (nat.af && $10->address->af != nat.af) {
					yyerror("nat ip versions must match");
					YYERROR;
				}
				nat.af = $10->address->af;
				memcpy(&nat.raddr, &$10->address->addr,
				    sizeof(nat.raddr));
				free($10->address);
				free($10);
			}

			pfctl_add_nat(pf, &nat);
		}
		;

binatrule	: no BINAT interface af proto FROM address TO ipspec redirection
		{
			struct pf_binat binat;

			if (!natmode) {
				yyerror("binat rule not permitted in filter mode");
				YYERROR;
			}
			memset(&binat, 0, sizeof(binat));

			binat.no = $1;
			if ($3 != NULL) {
				memcpy(binat.ifname, $3->ifname,
				    sizeof(binat.ifname));
				free($3);
			}
			binat.af = $4;
			if ($5 != NULL) {
				binat.proto = $5->proto;
				free($5);
			}
			if ($7 != NULL && $9 != NULL && $7->af != $9->af) {
				yyerror("binat ip versions must match");
				YYERROR;
			}
			if ($7 != NULL) {
				if ($7->next) {
					yyerror("multiple binat ip addresses");
					YYERROR;
				}
				if ($7->addr.addr_dyn != NULL) {
					if (!binat.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$7->af = binat.af;
				}
				if (binat.af && $7->af != binat.af) {
					yyerror("binat ip versions must match");
					YYERROR;
				}
				binat.af = $7->af;
				memcpy(&binat.saddr, &$7->addr,
				    sizeof(binat.saddr));
				free($7);
			}
			if ($9 != NULL) {
				if ($9->next) {
					yyerror("multiple binat ip addresses");
					YYERROR;
				}
				if ($9->addr.addr_dyn != NULL) {
					if (!binat.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$9->af = binat.af;
				}
				if (binat.af && $9->af != binat.af) {
					yyerror("binat ip versions must match");
					YYERROR;
				}
				binat.af = $9->af;
				memcpy(&binat.daddr, &$9->addr,
				    sizeof(binat.daddr));
				memcpy(&binat.dmask, &$9->mask,
				    sizeof(binat.dmask));
				binat.dnot  = $9->not;
				free($9);
			}

			if (binat.no) {
				if ($10 != NULL) {
					yyerror("'no binat' rule does not need"
					    " '->'");
					YYERROR;
				}
			} else {
				if ($10 == NULL || $10->address == NULL) {
					yyerror("'binat' rule requires"
					    " '-> address'");
					YYERROR;
				}
				if ($10->address->addr.addr_dyn != NULL) {
					if (!binat.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$10->address->af = binat.af;
				}
				if (binat.af && $10->address->af != binat.af) {
					yyerror("binat ip versions must match");
					YYERROR;
				}
				binat.af = $10->address->af;
				memcpy(&binat.raddr, &$10->address->addr,
				    sizeof(binat.raddr));
				free($10->address);
				free($10);
			}

			pfctl_add_binat(pf, &binat);
		}

rdrrule		: no RDR interface af proto FROM ipspec TO ipspec dport redirection
		{
			struct pf_rdr rdr;

			if (!natmode) {
				yyerror("rdr rule not permitted in filter mode");
				YYERROR;
			}
			memset(&rdr, 0, sizeof(rdr));

			rdr.no = $1;
			if ($3 != NULL) {
				memcpy(rdr.ifname, $3->ifname,
				    sizeof(rdr.ifname));
				rdr.ifnot = $3->not;
				free($3);
			}
			if ($5 != NULL) {
				rdr.proto = $5->proto;
				free($5);
			}
			if ($7 != NULL && $9 != NULL && $7->af != $9->af) {
				yyerror("rdr ip versions must match");
				YYERROR;
			}
			if ($7 != NULL) {
				if ($7->next) {
					yyerror("multiple rdr ip addresses");
					YYERROR;
				}
				if ($7->addr.addr_dyn != NULL) {
					if (!rdr.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$7->af = rdr.af;
				}
				if (rdr.af && $7->af != rdr.af) {
					yyerror("rdr ip versions must match");
					YYERROR;
				}
				rdr.af = $7->af;
				memcpy(&rdr.saddr, &$7->addr,
				    sizeof(rdr.saddr));
				memcpy(&rdr.smask, &$7->mask,
				    sizeof(rdr.smask));
				rdr.snot  = $7->not;
				free($7);
			}
			if ($9 != NULL) {
				if ($9->next) {
					yyerror("multiple rdr ip addresses");
					YYERROR;
				}
				if ($9->addr.addr_dyn != NULL) {
					if (!rdr.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$9->af = rdr.af;
				}
				if (rdr.af && $9->af != rdr.af) {
					yyerror("rdr ip versions must match");
					YYERROR;
				}
				rdr.af = $9->af;
				memcpy(&rdr.daddr, &$9->addr,
				    sizeof(rdr.daddr));
				memcpy(&rdr.dmask, &$9->mask,
				    sizeof(rdr.dmask));
				rdr.dnot  = $9->not;
				free($9);
			}

			rdr.dport  = $10.a;
			rdr.dport2 = $10.b;
			rdr.opts  |= $10.t;

			if (rdr.no) {
				if ($11 != NULL) {
					yyerror("'no rdr' rule does not need '->'");
					YYERROR;
				}
			} else {
				if ($11 == NULL || $11->address == NULL) {
					yyerror("'rdr' rule requires '-> address'");
					YYERROR;
				}
				if ($11->address->addr.addr_dyn != NULL) {
					if (!rdr.af) {
						yyerror("address family (inet/"
						    "inet6) undefined");
						YYERROR;
					}
					$11->address->af = rdr.af;
				}
				if (rdr.af && $11->address->af != rdr.af) {
					yyerror("rdr ip versions must match");
					YYERROR;
				}
				rdr.af = $11->address->af;
				memcpy(&rdr.raddr, &$11->address->addr,
				    sizeof(rdr.raddr));
				free($11->address);
				rdr.rport  = $11->rport.a;
				rdr.opts  |= $11->rport.t;
				free($11);
			}

			if (rdr.proto && rdr.proto != IPPROTO_TCP &&
			    rdr.proto != IPPROTO_UDP &&
			    (rdr.dport || rdr.dport2 || rdr.rport)) {
				yyerror("rdr ports are only valid for proto tcp/udp");
				YYERROR;
			}

			pfctl_add_rdr(pf, &rdr);
		}
		;

dport		: /* empty */			{
			$$.a = $$.b = $$.t = 0;
		}
		| PORT port			{
			$$.a = $2;
			$$.b = $$.t = 0;
		}
		| PORT port ':' port		{
			$$.a = $2;
			$$.b = $4;
			$$.t = PF_DPORT_RANGE;
		}
		;

route		: /* empty */			{
			$$.string = NULL;
			$$.rt = 0;
			$$.addr = NULL;
			$$.af = 0;
		}
		| FASTROUTE {
			$$.string = NULL;
			$$.rt = PF_FASTROUTE;
			$$.addr = NULL;
		}
		| ROUTETO STRING ':' address {
			$$.string = strdup($2);
			$$.rt = PF_ROUTETO;
			if ($4->addr.addr_dyn != NULL) {
				yyerror("route-to does not support"
				    " dynamic addresses");
				YYERROR;
			}
			if ($4->next) {
				yyerror("multiple routeto ip addresses");
				YYERROR;
			}
			$$.addr = &$4->addr.addr;
			$$.af = $4->af;
		}
		| ROUTETO STRING {
			$$.string = strdup($2);
			$$.rt = PF_ROUTETO;
			$$.addr = NULL;
		}
		| DUPTO STRING ':' address {
			$$.string = strdup($2);
			$$.rt = PF_DUPTO;
			if ($4->addr.addr_dyn != NULL) {
				yyerror("dup-to does not support"
				    " dynamic addresses");
				YYERROR;
			}
			if ($4->next) {
				yyerror("multiple dupto ip addresses");
				YYERROR;
			}
			$$.addr = &$4->addr.addr;
			$$.af = $4->af;
		}
		| DUPTO STRING {
			$$.string = strdup($2);
			$$.rt = PF_DUPTO;
			$$.addr = NULL;
		}
		;
%%

int
yyerror(char *fmt, ...)
{
	va_list ap;
	extern char *infile;
	errors = 1;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", infile, yylval.lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	return (0);
}

int
rule_consistent(struct pf_rule *r)
{
	int problems = 0;

	if (r->action == PF_SCRUB) {
		if (r->quick) {
			yyerror("quick does not apply to scrub");
			problems++;
		}
		if (r->keep_state == PF_STATE_MODULATE) {
			yyerror("modulate state does not apply to scrub");
			problems++;
		}
		if (r->keep_state == PF_STATE_NORMAL) {
			yyerror("keep state does not apply to scrub");
			problems++;
		}
		if (r->src.port_op) {
			yyerror("src port does not apply to scrub");
			problems++;
		}
		if (r->dst.port_op) {
			yyerror("dst port does not apply to scrub");
			problems++;
		}
		if (r->type || r->code) {
			yyerror("icmp-type/code does not apply to scrub");
			problems++;
		}
		if (r->rule_flag & PFRULE_FRAGMENT) {
			yyerror("fragment flag does not apply to scrub");
			problems++;
		}
	} else {
		if (r->rule_flag & PFRULE_NODF) {
			yyerror("nodf only applies to scrub");
			problems++;
		}
		if (r->min_ttl) {
			yyerror("min-ttl only applies to scrub");
			problems++;
		}
		if (r->max_mss) {
			yyerror("max-mss only applies to scrub");
			problems++;
		}
	}
	if (r->proto != IPPROTO_TCP && r->proto != IPPROTO_UDP &&
	    (r->src.port_op || r->dst.port_op)) {
		yyerror("port only applies to tcp/udp");
		problems++;
	}
	if (r->proto != IPPROTO_ICMP && r->proto != IPPROTO_ICMPV6 &&
	    (r->type || r->code)) {
		yyerror("icmp-type/code only applies to icmp");
		problems++;
	}
	if (!r->af && (r->type || r->code)) {
		yyerror("must indicate address family with icmp-type/code");
		problems++;
	}
	if ((r->proto == IPPROTO_ICMP && r->af == AF_INET6) ||
	    (r->proto == IPPROTO_ICMPV6 && r->af == AF_INET)) {
		yyerror("icmp version does not match address family");
		problems++;
	}
	if (!(r->rule_flag & PFRULE_RETURNRST) && r->return_icmp &&
	    ((r->af != AF_INET6  &&  (r->return_icmp>>8) != ICMP_UNREACH) ||
	    (r->af == AF_INET6 && (r->return_icmp>>8) != ICMP6_DST_UNREACH))) {
		yyerror("return-icmp version does not match address family");
		problems++;
	}
	if (r->keep_state == PF_STATE_MODULATE && r->proto &&
	    r->proto != IPPROTO_TCP) {
		yyerror("modulate state can only be applied to TCP rules");
		problems++;
	}
	if (r->allow_opts && r->action != PF_PASS) {
		yyerror("allow-opts can only be specified for pass rules");
		problems++;
	}
	if (!r->af && (r->src.addr.addr_dyn != NULL ||
	    r->dst.addr.addr_dyn != NULL)) {
		yyerror("dynamic addresses require address family (inet/inet6)");
		problems++;
	}
	if (r->rule_flag & PFRULE_FRAGMENT && (r->src.port_op ||
	    r->dst.port_op || r->flagset || r->type || r->code)) {
		yyerror("fragments can be filtered only on IP header fields");
		problems++;
	}
	return (-problems);
}

struct keywords {
	const char	*k_name;
	int	 k_val;
};

/* macro gore, but you should've seen the prior indentation nightmare... */

#define CHECK_ROOT(T,r) \
	do { \
		if (r == NULL) { \
			r = malloc(sizeof(T)); \
			if (r == NULL) \
				err(1, "malloc"); \
			memset(r, 0, sizeof(T)); \
		} \
	} while (0)

#define FREE_LIST(T,r) \
	do { \
		T *p, *n = r; \
		while (n != NULL) { \
			p = n; \
			n = n->next; \
			free(p); \
		} \
	} while (0)

#define LOOP_THROUGH(T,n,r,C) \
	do { \
		T *n = r; \
		while (n != NULL) { \
			do { \
				C; \
			} while (0); \
			n = n->next; \
		} \
	} while (0)

void
expand_rule(struct pf_rule *r,
    struct node_if *interfaces, struct node_proto *protos,
    struct node_host *src_hosts, struct node_port *src_ports,
    struct node_host *dst_hosts, struct node_port *dst_ports,
    struct node_uid *uids, struct node_gid *gids,
    struct node_icmp *icmp_types)
{
	int af = r->af, nomatch = 0, added = 0;
	char ifname[IF_NAMESIZE];

	CHECK_ROOT(struct node_if, interfaces);
	CHECK_ROOT(struct node_proto, protos);
	CHECK_ROOT(struct node_host, src_hosts);
	CHECK_ROOT(struct node_port, src_ports);
	CHECK_ROOT(struct node_host, dst_hosts);
	CHECK_ROOT(struct node_port, dst_ports);
	CHECK_ROOT(struct node_uid, uids);
	CHECK_ROOT(struct node_gid, gids);
	CHECK_ROOT(struct node_icmp, icmp_types);

	LOOP_THROUGH(struct node_if, interface, interfaces,
	LOOP_THROUGH(struct node_proto, proto, protos,
	LOOP_THROUGH(struct node_icmp, icmp_type, icmp_types,
	LOOP_THROUGH(struct node_host, src_host, src_hosts,
	LOOP_THROUGH(struct node_port, src_port, src_ports,
	LOOP_THROUGH(struct node_host, dst_host, dst_hosts,
	LOOP_THROUGH(struct node_port, dst_port, dst_ports,
	LOOP_THROUGH(struct node_uid, uid, uids,
	LOOP_THROUGH(struct node_gid, gid, gids,

		r->af = af;
		if ((r->af && src_host->af && r->af != src_host->af) ||
		    (r->af && dst_host->af && r->af != dst_host->af) ||
		    (src_host->af && dst_host->af &&
		    src_host->af != dst_host->af) ||
		    (src_host->ifindex && dst_host->ifindex &&
		     src_host->ifindex != dst_host->ifindex) ||
		    (src_host->ifindex && if_nametoindex(interface->ifname) &&
		     src_host->ifindex != if_nametoindex(interface->ifname)))
			continue;
		if (!r->af && src_host->af)
			r->af = src_host->af;
		else if (!r->af && dst_host->af)
			r->af = dst_host->af;

		if (if_indextoname(src_host->ifindex, ifname) == 0)
			memcpy(r->ifname, interface->ifname, sizeof(r->ifname));
		else
			memcpy(r->ifname, ifname, sizeof(r->ifname));
		r->proto = proto->proto;
		r->src.addr = src_host->addr;
		r->src.mask = src_host->mask;
		r->src.noroute = src_host->noroute;
		r->src.not = src_host->not;
		r->src.port[0] = src_port->port[0];
		r->src.port[1] = src_port->port[1];
		r->src.port_op = src_port->op;
		r->dst.addr = dst_host->addr;
		r->dst.mask = dst_host->mask;
		r->dst.noroute = dst_host->noroute;
		r->dst.not = dst_host->not;
		r->dst.port[0] = dst_port->port[0];
		r->dst.port[1] = dst_port->port[1];
		r->dst.port_op = dst_port->op;
		r->uid.op = uid->op;
		r->uid.uid[0] = uid->uid[0];
		r->uid.uid[1] = uid->uid[1];
		r->gid.op = gid->op;
		r->gid.gid[0] = gid->gid[0];
		r->gid.gid[1] = gid->gid[1];
		r->type = icmp_type->type;
		r->code = icmp_type->code;

		if (icmp_type->proto && r->proto != icmp_type->proto) {
			yyerror("icmp-type mismatch");
			nomatch++;
		}

		if (rule_consistent(r) < 0 || nomatch)
			yyerror("skipping rule due to errors");
		else {
			r->nr = pf->rule_nr++;
			pfctl_add_rule(pf, r);
			added++;
		}

	)))))))));

	FREE_LIST(struct node_if, interfaces);
	FREE_LIST(struct node_proto, protos);
	FREE_LIST(struct node_host, src_hosts);
	FREE_LIST(struct node_port, src_ports);
	FREE_LIST(struct node_host, dst_hosts);
	FREE_LIST(struct node_port, dst_ports);
	FREE_LIST(struct node_uid, uids);
	FREE_LIST(struct node_gid, gids);
	FREE_LIST(struct node_icmp, icmp_types);

	if (!added)
		yyerror("rule expands to no valid combination");
}

#undef FREE_LIST
#undef CHECK_ROOT
#undef LOOP_THROUGH

int
kw_cmp(k, e)
	const void *k, *e;
{
	return (strcmp(k, ((struct keywords *)e)->k_name));
}

int
lookup(char *s)
{
	/* this has to be sorted always */
	static const struct keywords keywords[] = {
		{ "all",	ALL},
		{ "allow-opts",	ALLOWOPTS},
		{ "any",	ANY},
		{ "binat",	BINAT},
		{ "block",	BLOCK},
		{ "code",	CODE},
		{ "dup-to",	DUPTO},
		{ "fastroute",	FASTROUTE},
		{ "flags",	FLAGS},
		{ "fragment",	FRAGMENT},
		{ "from",	FROM},
		{ "group",	GROUP},
		{ "icmp-type",	ICMPTYPE},
		{ "in",		IN},
		{ "inet",	INET},
		{ "inet6",	INET6},
		{ "ipv6-icmp-type", ICMP6TYPE},
		{ "keep",	KEEP},
		{ "label",	LABEL},
		{ "log",	LOG},
		{ "log-all",	LOGALL},
		{ "max-mss",	MAXMSS},
		{ "min-ttl",	MINTTL},
		{ "modulate",	MODULATE},
		{ "nat",	NAT},
		{ "no",		NO},
		{ "no-df",	NODF},
		{ "no-route",	NOROUTE},
		{ "on",		ON},
		{ "out",	OUT},
		{ "pass",	PASS},
		{ "port",	PORT},
		{ "proto",	PROTO},
		{ "quick",	QUICK},
		{ "rdr",	RDR},
		{ "return",	RETURN},
		{ "return-icmp",RETURNICMP},
		{ "return-icmp6",RETURNICMP6},
		{ "return-rst",	RETURNRST},
		{ "route-to",	ROUTETO},
		{ "scrub",	SCRUB},
		{ "state",	STATE},
		{ "to",		TO},
		{ "user",	USER},
	};
	const struct keywords *p;

	p = bsearch(s, keywords, sizeof(keywords)/sizeof(keywords[0]),
	    sizeof(keywords[0]), kw_cmp);

	if (p) {
		if (debug > 1)
			fprintf(stderr, "%s: %d\n", s, p->k_val);
		return (p->k_val);
	} else {
		if (debug > 1)
			fprintf(stderr, "string: %s\n", s);
		return (STRING);
	}
}

char	*parsebuf;
int	parseindex;

int
lgetc(FILE *fin)
{
	int c, next;

restart:
	if (parsebuf) {
		/* Reading characters from the parse buffer, instead of input */
		c = parsebuf[parseindex++];
		if (c != '\0')
			return (c);
		free(parsebuf);
		parsebuf = NULL;
		parseindex = 0;
		goto restart;
	}

	c = getc(fin);
	if (c == '\\') {
		next = getc(fin);
		if (next != '\n') {
			ungetc(next, fin);
			return (c);
		}
		yylval.lineno = lineno;
		lineno++;
		goto restart;
	}
	return (c);
}

int
lungetc(int c, FILE *fin)
{
	if (parsebuf && parseindex) {
		/* XXX breaks on index 0 */
		parseindex--;
		return (c);
	}
	return ungetc(c, fin);
}

int
findeol()
{
	int c;

	if (parsebuf) {
		free(parsebuf);
		parsebuf = NULL;
		parseindex = 0;
	}

	/* skip to either EOF or the first real EOL */
	while (1) {
		c = lgetc(fin);
		if (c == '\\') {
			c = lgetc(fin);
			if (c == '\n')
				continue;
		}
		if (c == EOF || c == '\n')
			break;
	}
	return (ERROR);
}

int
yylex(void)
{
	char buf[8096], *p, *val;
	int endc, c, next;
	int token;

top:
	p = buf;
	while ((c = lgetc(fin)) == ' ' || c == '\t')
		;

	yylval.lineno = lineno;
	if (c == '#')
		while ((c = lgetc(fin)) != '\n' && c != EOF)
			;
	if (c == '$' && parsebuf == NULL) {
		while (1) {
			if ((c = lgetc(fin)) == EOF)
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
			lungetc(c, fin);
			break;
		}
		val = symget(buf);
		if (val == NULL)
			return (ERROR);
		parsebuf = strdup(val);
		if (parsebuf == NULL)
			err(1, "parsebuf: strdup");
		parseindex = 0;
		goto top;
	}

	switch (c) {
	case '\'':
	case '"':
		endc = c;
		while (1) {
			if ((c = lgetc(fin)) == EOF)
				return (0);
			if (c == endc) {
				*p = '\0';
				break;
			}
			if (c == '\n')
				continue;
			if (p + 1 >= buf + sizeof(buf) - 1) {
				yyerror("string too long");
				return (findeol());
			}
			*p++ = (char)c;
		}
		yylval.v.string = strdup(buf);
		if (yylval.v.string == NULL)
			err(1, "yylex: strdup");
		return (STRING);
	case '=':
		yylval.v.i = PF_OP_EQ;
		return (PORTUNARY);
	case '!':
		next = lgetc(fin);
		if (next == '=') {
			yylval.v.i = PF_OP_NE;
			return (PORTUNARY);
		}
		lungetc(next, fin);
		break;
	case '<':
		next = lgetc(fin);
		if (next == '>') {
			yylval.v.i = PF_OP_XRG;
			return (PORTBINARY);
		} else  if (next == '=') {
			yylval.v.i = PF_OP_LE;
		} else {
			yylval.v.i = PF_OP_LT;
			lungetc(next, fin);
		}
		return (PORTUNARY);
		break;
	case '>':
		next = lgetc(fin);
		if (next == '<') {
			yylval.v.i = PF_OP_IRG;
			return (PORTBINARY);
		} else  if (next == '=') {
			yylval.v.i = PF_OP_GE;
		} else {
			yylval.v.i = PF_OP_GT;
			lungetc(next, fin);
		}
		return (PORTUNARY);
		break;
	case '-':
		next = lgetc(fin);
		if (next == '>')
			return (ARROW);
		lungetc(next, fin);
		break;
	}

	/* Need to parse v6 addresses before tokenizing numbers. ick */
	if (isxdigit(c) || c == ':') {
		struct node_host *node = NULL;
		u_int32_t addr[4];
		char lookahead[46];
		int i = 0;
		struct addrinfo hints, *res;

		lookahead[i] = c;

		while (i < sizeof(lookahead) &&
		    (isalnum(c) || c == ':' || c == '.' || c == '%')) {
			lookahead[++i] = c = lgetc(fin);
		}

		/* quick check avoids calling inet_pton too often */
		lungetc(lookahead[i], fin);
		lookahead[i] = '\0';

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_DGRAM;	/*dummy*/
		hints.ai_flags = AI_NUMERICHOST;
		if (getaddrinfo(lookahead, "0", &hints, &res) == 0) {
			node = calloc(1, sizeof(struct node_host));
			if (node == NULL)
				err(1, "yylex: calloc");
			node->af = AF_INET6;
			node->addr.addr_dyn = NULL;
			memcpy(&node->addr.addr,
			    &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,
			    sizeof(addr));
			node->ifindex = ((struct sockaddr_in6 *)res->ai_addr)
			    ->sin6_scope_id;
			yylval.v.host = node;
			return IPV6ADDR;
			freeaddrinfo(res);
		} else {
			free(node);
			while (i > 1)
				lungetc(lookahead[--i], fin);
			c = lookahead[--i];
		}
	}

	if (isdigit(c)) {
		int index = 0, base = 10;
		u_int64_t n = 0;

		yylval.v.number = 0;
		while (1) {
			if (base == 10) {
				if (!isdigit(c))
					break;
				c -= '0';
			} else if (base == 16) {
				if (isdigit(c))
					c -= '0';
				else if (c >= 'a' && c <= 'f')
					c -= 'a' - 10;
				else if (c >= 'A' && c <= 'F')
					c -= 'A' - 10;
				else
					break;
			}
			n = n * base + c;

			if (n > UINT_MAX) {
				yyerror("number is too large");
				return (ERROR);
			}
			c = lgetc(fin);
			if (c == EOF)
				break;
			if (index++ == 0 && n == 0 && c == 'x') {
				base = 16;
				c = lgetc(fin);
				if (c == EOF)
					break;
			}
		}
		yylval.v.number = (u_int32_t)n;

		if (c != EOF)
			lungetc(c, fin);
		if (debug > 1)
			fprintf(stderr, "number: %d\n", yylval.v.number);
		return (NUMBER);
	}

#define allowed_in_string(x) \
	(isalnum(x) || (ispunct(x) && x != '(' && x != ')' && \
	x != '{' && x != '}' && x != '<' && x != '>' && \
	x != '!' && x != '=' && x != '/' && x != '#' && \
	x != ',' && x != ':' && x != '(' && x != ')'))

	if (isalnum(c)) {
		do {
			*p++ = c;
			if (p-buf >= sizeof buf) {
				yyerror("string too long");
				return (ERROR);
			}
		} while ((c = lgetc(fin)) != EOF && (allowed_in_string(c)));
		lungetc(c, fin);
		*p = '\0';
		token = lookup(buf);
		yylval.v.string = strdup(buf);
		if (yylval.v.string == NULL)
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

int
parse_rules(FILE *input, struct pfctl *xpf)
{
	natmode = 0;
	fin = input;
	pf = xpf;
	lineno = 1;
	errors = 0;
	yyparse();
	return (errors ? -1 : 0);
}

int
parse_nat(FILE *input, struct pfctl *xpf)
{
	natmode = 1;
	fin = input;
	pf = xpf;
	lineno = 1;
	errors = 0;
	yyparse();
	return (errors ? -1 : 0);
}

void
ipmask(struct pf_addr *m, u_int8_t b)
{
	int i, j = 0;

	while (b >= 32) {
		m->addr32[j++] = 0xffffffff;
		b -= 32;
	}
	for (i = 31; i > 31-b; --i)
		m->addr32[j] |= (1 << i);
	if (b)
		m->addr32[j] = htonl(m->addr32[j]);
}

/*
 * Over-designed efficiency is a French and German concept, so how about
 * we wait until they discover this ugliness and make it all fancy.
 */
int
symset(char *nam, char *val)
{
	struct sym *sym;

	sym = calloc(1, sizeof(*sym));
	if (sym == NULL)
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
	sym->next = symhead;
	symhead = sym;
	return (0);
}

char *
symget(char *nam)
{
	struct sym *sym;

	for (sym = symhead; sym; sym = sym->next)
		if (strcmp(nam, sym->nam) == 0)
			return (sym->val);
	return (NULL);
}

struct ifaddrs **ifa0tab, **ifa4tab, **ifa6tab;
int ifa0len, ifa4len, ifa6len;

int
ifa_comp(const void *p1, const void *p2)
{
	struct ifaddrs *ifa1 = *(struct ifaddrs **)p1;
	struct ifaddrs *ifa2 = *(struct ifaddrs **)p2;

	return strcmp(ifa1->ifa_name, ifa2->ifa_name);
}

void
ifa_load(void)
{
	struct ifaddrs *ifap, *ifa;
	int ifalen = 0;

	if (getifaddrs(&ifap) < 0)
		err(1, "getifaddrs");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next)
		ifalen++;
	/* (over-)allocate tables */
	ifa0tab = malloc(ifalen * sizeof(void *));
	ifa4tab = malloc(ifalen * sizeof(void *));
	ifa6tab = malloc(ifalen * sizeof(void *));
	if (!ifa0tab || !ifa4tab || !ifa6tab)
		err(1, "malloc");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_LINK) {
			if (bsearch(&ifa, ifa0tab, ifa0len, sizeof(void *),
			    ifa_comp))
				continue; /* take only the first LINK address */
			ifa0tab[ifa0len++] = ifa;
			qsort(ifa0tab, ifa0len, sizeof(void *), ifa_comp);
		}
		if (ifa->ifa_addr->sa_family == AF_INET) {
			if (bsearch(&ifa, ifa4tab, ifa4len, sizeof(void *),
			    ifa_comp))
				continue; /* take only the first IPv4 address */
			ifa4tab[ifa4len++] = ifa;
			qsort(ifa4tab, ifa4len, sizeof(void *), ifa_comp);
		}
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			/* XXX - better address selection required! */
			if (bsearch(&ifa, ifa6tab, ifa6len, sizeof(void *),
			    ifa_comp))
				continue; /* take only the first IPv6 address */
			ifa6tab[ifa6len++] = ifa;
			qsort(ifa6tab, ifa6len, sizeof(void *), ifa_comp);
		}
	}
	/* shrink tables */
	ifa0tab = realloc(ifa0tab, ifa0len * sizeof(void *));
	ifa4tab = realloc(ifa4tab, ifa4len * sizeof(void *));
	ifa6tab = realloc(ifa6tab, ifa6len * sizeof(void *));
	if (!ifa0tab || !ifa4tab || !ifa6tab)
		err(1, "realloc");
}

struct ifaddrs *
ifa0_lookup(char *ifa_name)
{
	struct ifaddrs ifa, *ifp = &ifa, **ifpp;

	if (!ifa0tab)
		ifa_load();
	ifa.ifa_name = ifa_name;
	ifpp = bsearch(&ifp, ifa0tab, ifa0len, sizeof(void *), ifa_comp);
	return ifpp ? *ifpp : NULL;
}

struct ifaddrs *
ifa4_lookup(char *ifa_name)
{
	struct ifaddrs ifa, *ifp = &ifa, **ifpp;

	if (!ifa4tab)
		ifa_load();
	ifa.ifa_name = ifa_name;
	ifpp = bsearch(&ifp, ifa4tab, ifa4len, sizeof(void *), ifa_comp);
	return ifpp ? *ifpp : NULL;
}

struct ifaddrs *
ifa6_lookup(char *ifa_name)
{
	struct ifaddrs ifa, *ifp = &ifa, **ifpp;

	if (!ifa6tab)
		ifa_load();
	ifa.ifa_name = ifa_name;
	ifpp = bsearch(&ifp, ifa6tab, ifa6len, sizeof(void *), ifa_comp);
	return ifpp ? *ifpp : NULL;
}

