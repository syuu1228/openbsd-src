#!/bin/sh
#
# Copyright (C) 2004  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2001  Internet Software Consortium.
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# $ISC: setup.sh,v 1.1.206.1 2004/03/06 10:22:07 marka Exp $

rm -f ns1/*.db ns1/*.jnl

cat <<EOF >ns1/named.conf
options {
	query-source address 10.53.0.1;
	notify-source 10.53.0.1;
	transfer-source 10.53.0.1;
	port 5300;
	pid-file "named.pid";
	listen-on { 10.53.0.1; };
	listen-on-v6 { none; };
	recursion no;
	notify yes;
};

key rndc_key {
	secret "1234abcd8765";
	algorithm hmac-md5;
};

controls {
	inet 10.53.0.1 port 9953 allow { any; } keys { rndc_key; };
};
EOF
