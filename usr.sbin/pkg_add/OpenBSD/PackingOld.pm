# ex:ts=8 sw=4:
# $OpenBSD: src/usr.sbin/pkg_add/OpenBSD/Attic/PackingOld.pm,v 1.3 2004/09/20 08:53:53 espie Exp $
#
# Copyright (c) 2004 Marc Espie <espie@openbsd.org>
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
use OpenBSD::PackingElement;

package OpenBSD::PackingElement::Src;
our @ISA=qw(OpenBSD::PackingElement);

__PACKAGE__->setOldKeyword('src');

sub keyword() { 'src' }

package OpenBSD::PackingElement::PkgConflict;
our @ISA=qw(OpenBSD::PackingElement);

__PACKAGE__->setOldKeyword('pkgcfl');
sub keyword() { "pkgcfl" }
sub category() { "pkgcfl" }

1;
