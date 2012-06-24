# ex:ts=8 sw=4:
# $OpenBSD: src/usr.bin/libtool/LT/Mode/Install.pm,v 1.1 2012/06/24 13:44:53 espie Exp $
#
# Copyright (c) 2007-2010 Steven Mestdagh <steven@openbsd.org>
# Copyright (c) 2012 Marc Espie <espie@openbsd.org>
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

package LT::Mode::Install;

use LT::Util;
use Getopt::Std;
use File::Basename;

sub run
{
	my ($class, $ltprog) = @_;
	# we just parse the options in order to find the actual arguments
	my @argvcopy = @ARGV;
	my %install_opts;
	LT::Trace::debug {"ltprog[-1]  = $$ltprog[-1]\n"};
	if ($$ltprog[-1] =~ m/install([.-]sh)?$/) {
		getopts('BbCcdf:g:m:o:pSs', \%install_opts);
		if (@ARGV < 2 && (!defined $install_opts{'d'} && @ARGV == 1)) {
			die "Wrong number of arguments for install\n";
		}
	} elsif ($$ltprog[-1] =~ m/cp$/) {
		getopts('HLPRfipr', \%install_opts);
		if (@ARGV < 2) {
			die "Wrong number of arguments for install\n";
		}
	} else {
		die "Unsupported install program $$ltprog[-1]\n";
	}
	my @instopts = @argvcopy[0 .. (@argvcopy - @ARGV - 1)];
	my $dst = pop @ARGV;
	my @src = @ARGV;
	my $dstdir;
	if (-d $dst) {
		$dstdir = $dst;
	} else {
		# dst is not a directory, i.e. a file
		if (@src > 1) {
			# XXX not really libtool's task to check this
			die "Multiple source files combined with file destination.\n";
		} else {
			$dstdir = dirname $dst;
		}
	}
	foreach my $s (@src) {
		my $dstfile = basename $s;
		# resolve symbolic links, so we don't try to test later
		# whether the symlink is a program wrapper etc.
		if (-l $s) {
			$s = readlink($s) or die "Cannot readlink $s: $!\n";
		}
		my $srcdir = dirname $s;
		my $srcfile = basename $s;
		LT::Trace::debug {"srcdir = $srcdir\nsrcfile = $srcfile\n"};
		LT::Trace::debug {"dstdir = $dstdir\ndstfile = $dstfile\n"};
		if ($srcfile =~ m/^\S+\.la$/) {
			require LT::LaFile;
			LT::LaFile->install($s, $dstdir, $ltprog, \@instopts, $install_opts{'s'});
		} elsif (-f "$srcdir/$ltdir/$srcfile" && is_wrapper($s)) {
			require LT::Program;
			LT::Program->install($s, $dst, $ltprog, \@instopts);
		} else {
			LT::Exec->install(@$ltprog, @instopts, $s, $dst);
		}
	}
	if (defined $install_opts{'d'}) {
		LT::Exec->install(@$ltprog, @instopts, @ARGV);
	}
}

sub is_wrapper
{
#	my $self = shift;
	my $program = shift;

	open(my $pw, '<', $program) or die "Cannot open $program: $!\n";
	return grep { m/wrapper\sfor/ } <$pw>;
}

1;
