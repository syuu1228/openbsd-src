#	$OpenBSD: src/regress/sys/kern/splice/Client.pm,v 1.1 2011/01/07 22:06:08 bluhm Exp $

# Copyright (c) 2010 Alexander Bluhm <bluhm@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

package Client;
use parent 'Proc';
use Carp;
use Socket qw(IPPROTO_TCP TCP_NODELAY);
use Socket6;
use IO::Socket;
use IO::Socket::INET6;

sub new {
	my $class = shift;
	my %args = @_;
	$args{logfile} ||= "client.log";
	$args{up} ||= "Connected";
	$args{down} ||= "Shutdown|Broken pipe|Connection reset by peer";
	my $self = Proc::new($class, %args);
	$self->{connectdomain}
	    or croak "$class connect domain not given";
	$self->{connectaddr}
	    or croak "$class connect addr not given";
	$self->{connectport}
	    or croak "$class connect port not given";
	return $self;
}

sub child {
	my $self = shift;

	my $cs = IO::Socket::INET6->new(
	    Proto	=> "tcp",
	    Domain	=> $self->{connectdomain},
	    Blocking	=> ($self->{nonblocking} ? 0 : 1),
	) or die ref($self), " socket connect failed: $!";
	if ($self->{oobinline}) {
		setsockopt($cs, SOL_SOCKET, SO_OOBINLINE, pack('i', 1))
		    or die ref($self), " set oobinline connect failed: $!";
	}
	if ($self->{sndbuf}) {
		setsockopt($cs, SOL_SOCKET, SO_SNDBUF,
		    pack('i', $self->{sndbuf}))
		    or die ref($self), " set sndbuf connect failed: $!";
	}
	if ($self->{rcvbuf}) {
		setsockopt($cs, SOL_SOCKET, SO_RCVBUF,
		    pack('i', $self->{rcvbuf}))
		    or die ref($self), " set rcvbuf connect failed: $!";
	}
	setsockopt($cs, IPPROTO_TCP, TCP_NODELAY, pack('i', 1))
	    or die ref($self), " set nodelay connect failed: $!";
	my @rres = getaddrinfo($self->{connectaddr}, $self->{connectport},
	    $self->{connectdomain}, SOCK_STREAM);
	$cs->connect($rres[3])
	    or die ref($self), " connect failed: $!";
	print STDERR "connect sock: ",$cs->sockhost()," ",$cs->sockport(),"\n";
	print STDERR "connect peer: ",$cs->peerhost()," ",$cs->peerport(),"\n";

	open(STDOUT, '>&', $cs)
	    or die ref($self), " dup STDOUT failed: $!";
	open(STDIN, '<&', $cs)
	    or die ref($self), " dup STDIN failed: $!";
}

1;
