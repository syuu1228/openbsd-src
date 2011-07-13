/*	$OpenBSD: src/usr.sbin/smtpd/pack.c,v 1.3 2011/07/13 15:08:24 eric Exp $	*/
/*
 * Copyright (c) 2009,2010	Eric Faurot	<eric@faurot.net>
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

#include <string.h>

#include "dnsutil.h"

int		dname_check_label(const char*, size_t);
const char*	dname_nthlabel(int, const unsigned char*, size_t, size_t);
ssize_t 	dname_count_labels(const unsigned char*, size_t, size_t);
ssize_t		dname_expand(const unsigned char*, size_t, size_t,
		    size_t*, char *, size_t);


void
packed_init(struct packed *pack, char *data, size_t len)
{
	pack->data = data;
	pack->len = len;
	pack->offset = 0;
	pack->err = NULL;
}

ssize_t
dname_expand(const unsigned char *data, size_t len, size_t offset,
    size_t *newoffset, char *dst, size_t max)
{
	size_t		 n, count, end, ptr, start;
	ssize_t		 res;

	if (offset >= len)
		return (-1);

	res = 0;
	end = start = offset;

	for(; (n = data[offset]); ) {
		if ((n & 0xc0) == 0xc0) {
			if (offset + 2 > len)
				return (-1);
			ptr = 256 * (n & ~0xc0) + data[offset + 1];
			if (ptr >= start)
				return (-1);
			if (end < offset + 2)
				end = offset + 2;
			offset = ptr;
			continue;
		}
		if (offset + n + 1 > len)
			return (-1);

		if (dname_check_label(data + offset + 1, n) == -1)
			return (-1);

		/* copy n + at offset+1 */
		if (dst != NULL && max != 0) {
			count = (max < n + 1) ? (max) : (n + 1);
			memmove(dst, data + offset, count);
			dst += count;
			max -= count;
		}
		res += n + 1;
		offset += n + 1;
		if (end < offset)
			end = offset;
	}
	if (end < offset + 1)
		end = offset + 1;

	if (dst != NULL && max != 0)
		dst[0] = 0;
	if (newoffset)
		*newoffset = end;
	return (res + 1);
}

const char *
dname_nthlabel(int n, const unsigned char *data, size_t len, size_t offset)
{
	int		 i;
	size_t		 c, ptr, start;

	start = offset;
	for(i = 0;;) {
		c = data[offset];
		if (c == 0)
			return (NULL);
		if ((c & 0xc0) == 0xc0) {
			if (len < offset + 2)
				return (NULL);
			ptr = 256 * (c & ~0xc0) + data[offset + 1];
			if (ptr >= start)
				return (NULL);
			offset = ptr;
			continue;
		}
		if (i == n)
			break;
		offset += c + 1;
		i++;
	}
	return (data + offset);
}

ssize_t
dname_count_labels(const unsigned char *data, size_t len, size_t offset)
{
	size_t c, n, ptr, start;

	start = offset;
	for(n = 0; (c = data[offset]); ) {
		if ((c & 0xc0) == 0xc0) {
			if (len < offset + 2)
				return (-1);
			ptr = 256 * (c & ~0xc0) + data[offset + 1];
			if (ptr >= start)
				return (-1);
			offset = ptr;
			continue;
		}
		offset += c + 1;
		n += 1;
	}

	return (n);
}

int
unpack_data(struct packed *p, void *data, size_t len)
{
	if (p->err)
		return (-1);

	if (p->len - p->offset < len) {
		p->err = "too short";
		return (-1);
	}

	memmove(data, p->data + p->offset, len);
	p->offset += len;

	return (0);
}

int
unpack_u16(struct packed *p, uint16_t *u16)
{
	if (unpack_data(p, u16, 2) == -1)
		return (-1);

	*u16 = ntohs(*u16);

	return (0);
}

int
unpack_u32(struct packed *p, uint32_t *u32)
{
	if (unpack_data(p, u32, 4) == -1)
		return (-1);

	*u32 = ntohl(*u32);

	return (0);
}

int
unpack_inaddr(struct packed *p, struct in_addr *a)
{
	return (unpack_data(p, a, 4));
}

int
unpack_in6addr(struct packed *p, struct in6_addr *a6)
{
	return (unpack_data(p, a6, 16));
}

int
unpack_dname(struct packed *p, char *dst, size_t max)
{
	ssize_t e;

	if (p->err)
		return (-1);

	e = dname_expand(p->data, p->len, p->offset, &p->offset, dst, max);
	if (e == -1) {
		p->err = "bad domain name";
		return (-1);
	}
	if (e < 0 || e > DOMAIN_MAXLEN) {
		p->err = "domain name too long";
		return (-1);
	}

	return (0);
}

int
unpack_header(struct packed *p, struct header *h)
{
	if (unpack_data(p, h, HEADER_LEN) == -1)
		return (-1);

	h->flags = ntohs(h->flags);
	h->qdcount = ntohs(h->qdcount);
	h->ancount = ntohs(h->ancount);
	h->nscount = ntohs(h->nscount);
	h->arcount = ntohs(h->arcount);

	return (0);
}

int
unpack_query(struct packed *p, struct query *q)
{
	unpack_dname(p, q->q_dname, sizeof(q->q_dname));
	unpack_u16(p, &q->q_type);
	unpack_u16(p, &q->q_class);

	return (p->err) ? (-1) : (0);
}

int
unpack_rr(struct packed *p, struct rr *rr)
{
	uint16_t	rdlen;
	size_t		save_offset;

	unpack_dname(p, rr->rr_dname, sizeof(rr->rr_dname));
	unpack_u16(p, &rr->rr_type);
	unpack_u16(p, &rr->rr_class);
	unpack_u32(p, &rr->rr_ttl);
	unpack_u16(p, &rdlen);

	if (p->err)
		return (-1);

	if (p->len - p->offset < rdlen) {
		p->err = "too short";
		return (-1);
	}

	save_offset = p->offset;

	switch(rr->rr_type) {

	case T_CNAME:
		unpack_dname(p, rr->rr.cname.cname, sizeof(rr->rr.cname.cname));
		break;

	case T_MX:
		unpack_u16(p, &rr->rr.mx.preference);
		unpack_dname(p, rr->rr.mx.exchange, sizeof(rr->rr.mx.exchange));
		break;

	case T_NS:
		unpack_dname(p, rr->rr.ns.nsname, sizeof(rr->rr.ns.nsname));
		break;

	case T_PTR:
		unpack_dname(p, rr->rr.ptr.ptrname, sizeof(rr->rr.ptr.ptrname));
		break;

	case T_SOA:
		unpack_dname(p, rr->rr.soa.mname, sizeof(rr->rr.soa.mname));
		unpack_dname(p, rr->rr.soa.rname, sizeof(rr->rr.soa.rname));
		unpack_u32(p, &rr->rr.soa.serial);
		unpack_u32(p, &rr->rr.soa.refresh);
		unpack_u32(p, &rr->rr.soa.retry);
		unpack_u32(p, &rr->rr.soa.expire);
		unpack_u32(p, &rr->rr.soa.minimum);
		break;

	case T_A:
		if (rr->rr_class != C_IN)
			goto other;
		unpack_inaddr(p, &rr->rr.in_a.addr);
		break;

	case T_AAAA:
		if (rr->rr_class != C_IN)
			goto other;
		unpack_in6addr(p, &rr->rr.in_aaaa.addr6);
		break;
	default:
	other:
		rr->rr.other.rdata = p->data + p->offset;
		rr->rr.other.rdlen = rdlen;
		p->offset += rdlen;
	}

	if (p->err)
		return (-1);

	/* make sure that the advertised rdlen is really ok */
	if (p->offset - save_offset != rdlen)
		p->err = "bad dlen";

	return (p->err) ? (-1) : (0);
}

int
pack_data(struct packed *p, const void *data, size_t len)
{
	if (p->err)
		return (-1);

	if (p->len < p->offset + len) {
		p->err = "no space";
		return (-1);
	}

	memmove(p->data + p->offset, data, len);
	p->offset += len;

	return (0);
}

int
pack_u16(struct packed *p, uint16_t v)
{
	v = htons(v);

	return (pack_data(p, &v, 2));
}

int
pack_u32(struct packed *p, uint32_t v)
{
	v = htonl(v);

	return (pack_data(p, &v, 4));
}

int
pack_inaddr(struct packed *p, struct in_addr a)
{
	return (pack_data(p, &a, 4));
}

int
pack_in6addr(struct packed *p, struct in6_addr a6)
{
	return (pack_data(p, &a6, 16));
}

int
pack_dname(struct packed *p, const char *dname)
{
	/* dname compression would be nice to have here.
	 * need additionnal context.
	 */
	return (pack_data(p, dname, strlen(dname) + 1));
}

int
pack_header(struct packed *p, const struct header *h)
{
	struct header c;

	c.id = h->id;
	c.flags = htons(h->flags);
	c.qdcount = htons(h->qdcount);
	c.ancount = htons(h->ancount);
	c.nscount = htons(h->nscount);
	c.arcount = htons(h->arcount);	

	return (pack_data(p, &c, HEADER_LEN));
}

int
pack_query(struct packed *p, uint16_t type, uint16_t class, const char *dname)
{
	pack_dname(p, dname);
	pack_u16(p, type);
	pack_u16(p, class);

	return (p->err) ? (-1) : (0);
}

int     
pack_rrdynamic(struct packed *p, const struct rr_dynamic *rd)
{
	const union rr_subtype	*rr;
	struct packed		 save;

	pack_dname(p, rd->rd_dname);
	pack_u16(p, rd->rd_type);
	pack_u16(p, rd->rd_class);
	pack_u32(p, rd->rd_ttl);

	save = *p;
	pack_u16(p, 0); /* rdlen */

	rr = &rd->rd;
	switch(rd->rd_type) {
	case T_CNAME:
		pack_dname(p, rr->cname.cname);
		break;

	case T_MX:
		pack_u16(p, rr->mx.preference);
		pack_dname(p, rr->mx.exchange);
		break;

	case T_NS:
		pack_dname(p, rr->ns.nsname);
		break;

	case T_PTR:
		pack_dname(p, rr->ptr.ptrname);
		break;

	case T_SOA:
		pack_dname(p, rr->soa.mname);
		pack_dname(p, rr->soa.rname);
		pack_u32(p, rr->soa.serial);
		pack_u32(p, rr->soa.refresh);
		pack_u32(p, rr->soa.retry);
		pack_u32(p, rr->soa.expire);
		pack_u32(p, rr->soa.minimum);
		break;

	case T_A:
		if (rd->rd_class != C_IN)
			goto other;
		pack_inaddr(p, rr->in_a.addr);
		break;

	case T_AAAA:
		if (rd->rd_class != C_IN)
			goto other;
		pack_in6addr(p, rr->in_aaaa.addr6);
		break;
	default:
	other:
		pack_data(p, rr->other.rdata, rr->other.rdlen);
	}

        if (p->err)
		return (-1);

	/* rewrite rdlen */
	pack_u16(&save, p->offset - save.offset - 2);
	p->err = save.err;

        return (p->err) ? (-1) : (0);
}
