/**//*	$OpenBSD: src/usr.sbin/tcpdump/ipx.h,v 1.3 1996/06/10 07:47:19 deraadt Exp $	*/
/*	$NetBSD: ipx.h,v 1.2 1995/03/06 19:10:19 mycroft Exp $	*/

/*
 * IPX protocol formats 
 *
 * @(#) Header: ipx.h,v 1.1 94/06/09 11:47:03 mccanne Exp
 */

/* well-known sockets */
#define	IPX_SKT_NCP		0x0451
#define	IPX_SKT_SAP		0x0452
#define	IPX_SKT_RIP		0x0453
#define	IPX_SKT_NETBIOS		0x0455
#define	IPX_SKT_DIAGNOSTICS	0x0456

/* IPX transport header */
struct ipxHdr {
    u_short	cksum;		/* Checksum */
    u_short	length;		/* Length, in bytes, including header */
    u_char	tCtl;		/* Transport Control (i.e. hop count) */
    u_char	pType;		/* Packet Type (i.e. level 2 protocol) */
    u_short	dstNet[2];	/* destination net */
    u_char	dstNode[6];	/* destination node */
    u_short	dstSkt;		/* destination socket */
    u_short	srcNet[2];	/* source net */
    u_char	srcNode[6];	/* source node */
    u_short	srcSkt;		/* source socket */
} ipx_hdr_t;

#define ipxSize	30

