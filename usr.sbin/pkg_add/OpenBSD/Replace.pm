# ex:ts=8 sw=4:
# $OpenBSD: src/usr.sbin/pkg_add/OpenBSD/Replace.pm,v 1.37 2007/06/07 20:41:01 espie Exp $
#
# Copyright (c) 2004-2006 Marc Espie <espie@openbsd.org>
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
#

use strict;
use warnings;

use OpenBSD::Delete;
use OpenBSD::Interactive;

package OpenBSD::PackingElement;
sub can_update
{
	my ($self, $install, $state) = @_;

	my $issue = $self->update_issue($install);
	
	if (defined $issue) {
		$state->{okay} = 0;
	    	push(@{$state->{journal}}, $issue);
	}
}

sub validate_depend
{
}

sub update_issue($$) { undef }

sub extract
{
	my ($self, $state) = @_;
	$state->{partial}->{$self} = 1;
}

sub mark_lib
{
}

sub unmark_lib
{
}

sub separate_element
{
	my ($self, $libs, $c1, $c2) = @_;
	$c2->{$self} = 1;
}

sub extract_and_progress
{
	my ($self, $state, $donesize, $totsize) = @_;
	$self->extract($state);
	if ($state->{interrupted}) {
		die "Interrupted";
	}
	$self->mark_progress($state->progress, $donesize, $totsize);
}

package OpenBSD::PackingElement::Meta;

sub separate_element
{
	my ($self, $libs, $c1, $c2) = @_;
	$c1->{$self} = 1;
	$c2->{$self} = 1;
}

package OpenBSD::PackingElement::State;

sub separate_element
{
	&OpenBSD::PackingElement::Meta::separate_element;
}

package OpenBSD::PackingElement::FileBase;
use OpenBSD::Temp;

sub extract
{
	my ($self, $state) = @_;

	my $file = $self->prepare_to_extract($state);

	if (defined $self->{link} || defined $self->{symlink}) {
		$state->{archive}->skip;
		return;
	}
	
	$self->SUPER::extract($state);

	# figure out a safe directory where to put the temp file
	my $d = dirname($file->{destdir}.$file->{name});
	# we go back up until we find an existing directory.
	# hopefully this will be on the same file system.
	while (!-d $d && -e _) {
		$d = dirname($d);
	}
	if ($state->{not}) {
		print "extracting tempfile under $d\n" if $state->{very_verbose};
		$state->{archive}->skip;
	} else {
		if (!-e _) {
			File::Path::mkpath($d);
		}
		my ($fh, $tempname) = OpenBSD::Temp::permanent_file($d, "pkg");

		print "extracting $tempname\n" if $state->{very_verbose};
		$file->{name} = $tempname;
		$self->{tempname} = $tempname;
		$file->create;
	}
}

package OpenBSD::PackingElement::Dir;
sub extract
{
	my ($self, $state) = @_;
	my $fullname = $self->fullname;
	my $destdir = $state->{destdir};

	return if -e $destdir.$fullname;
	$self->SUPER::extract($state);
	print "new directory ", $destdir, $fullname, "\n" if $state->{very_verbose};
	return if $state->{not};
	File::Path::mkpath($destdir.$fullname);
}


package OpenBSD::PackingElement::Sample;
sub extract
{
}

package OpenBSD::PackingElement::Sampledir;
sub extract
{
}

package OpenBSD::PackingElement::ScriptFile;
sub update_issue($$) 
{ 
	return $_[0]->{name}." script";
}

package OpenBSD::PackingElement::FINSTALL;
sub update_issue($$) 
{ 
	return if !$_[1];
	return $_[0]->SUPER::update_issue($_[1]);
}

package OpenBSD::PackingElement::FDEINSTALL;
sub update_issue($$) 
{ 
	return if $_[1];
	return $_[0]->SUPER::update_issue($_[1]);
}

package OpenBSD::PackingElement::Exec;
sub update_issue($$) 
{ 
	return if !$_[1];
	return '@exec '.$_[0]->{expanded};
}

package OpenBSD::PackingElement::Unexec;
sub update_issue($$) 
{ 
	return if $_[1];
	my $self = $_[0];

	return '@unexec '.$self->{expanded};
}

package OpenBSD::PackingElement::Depend;
sub separate_element
{
	&OpenBSD::PackingElement::separate_element;
}

package OpenBSD::PackingElement::SpecialFile;
sub separate_element
{
	&OpenBSD::PackingElement::separate_element;
}

package OpenBSD::PackingElement::Dependency;
use OpenBSD::Error;

sub validate_depend
{
	my ($self, $state, $wanting, $toreplace, $replacement) = @_;

	# nothing to validate if old dependency doesn't concern us.
	return unless $self->spec->filter($toreplace);
	# nothing to do if new dependency just matches
	return if $self->spec->filter($replacement);

	if ($state->{forced}->{updatedepends}) {
	    Warn "Forward dependency of $wanting on $toreplace doesn't match $replacement, forcing it\n";
	    $state->{forcedupdates} = {} unless defined $state->{forcedupdates};
	    $state->{forcedupdates}->{$wanting} = 1;
	} elsif ($state->{interactive}) {

	    if (OpenBSD::Interactive::confirm("Forward dependency of $wanting on $toreplace doesn't match $replacement, proceed with update anyways", 1, 0, 'updatedepends')) {
		$state->{forcedupdates} = {} unless defined $state->{forcedupdates};
		$state->{forcedupdates}->{$wanting} = 1;
	    } else {
		$state->{okay} = 0;
	    }
	} else {
	    $state->{okay} = 0;
	    Warn "Can't update forward dependency of $wanting on $toreplace: $replacement doesn't match (use -F updatedepends to force it)\n";
	}
}

package OpenBSD::PackingElement::Lib;
sub mark_lib
{
	my ($self, $libs, $libpatterns) = @_;
	my $libname = $self->fullname;
	my ($stem, $major, $minor, $dir) = $self->parse($libname);
	if (defined $stem) {
		$libpatterns->{$stem}->{$dir} = [$major, $minor, $libname];
	}
	$libs->{$libname} = 1;
}

sub separate_element
{
	my ($self, $libs, $c1, $c2) = @_;
	if ($libs->{$self->fullname}) {
		$c1->{$self} = 1;
	} else {
		$c2->{$self} = 1;
	}
}

sub unmark_lib
{
	my ($self, $libs, $libpatterns) = @_;
	my $libname = $self->fullname;
	my ($stem, $major, $minor, $dir) = $self->parse($libname);
	if (defined $stem) {
		my $p = $libpatterns->{$stem}->{$dir};
		if (defined $p && $p->[0] == $major && $p->[1] <= $minor) {
			my $n = $p->[2];
			delete $libs->{$n};
		}
	}
	delete $libs->{$libname};
}

package OpenBSD::Replace;
use OpenBSD::RequiredBy;
use OpenBSD::PackingList;
use OpenBSD::PackageInfo;
use OpenBSD::Error;
use OpenBSD::Interactive;

sub perform_extraction
{
	my ($handle, $state) = @_;

	$handle->{partial} = {};
	$state->{partial} = $handle->{partial};
	my $totsize = $handle->{totsize};
	$state->{archive} = $handle->{location};
	my $donesize = 0;
	$state->{donesize} = 0;
	$handle->{plist}->extract_and_progress($state, \$donesize, $totsize);
}

sub can_do
{
	my ($toreplace, $replacement, $state, $ignore) = @_;

	$state->{okay} = 1;
	my $plist = OpenBSD::PackingList->from_installation($toreplace);
	if (!defined $plist) {
		Fatal "Couldn't find packing-list for $toreplace\n";
	}
	$state->{journal} = [];
	$plist->can_update(0, $state);
	if ($state->{okay} == 0) {
		Warn "Old package ", $plist->pkgname, " contains potentially unsafe operations\n";
		for my $i (@{$state->{journal}}) {
			Warn "\t$i\n";
		}
		if ($state->{forced}->{update}) {
			Warn "(forcing update)\n";
			$state->{okay} = 1;
		} elsif ($state->{interactive}) {

			if (OpenBSD::Interactive::confirm("proceed with update anyways", 1, 0, 'update')) {
			    $state->{okay} = 1;
			}
		}
	}
	my @wantlist = OpenBSD::RequiredBy->new($toreplace)->list;
	my @r = ();
	for my $wanting (@wantlist) {
		push(@r, $wanting) if !defined $ignore->{$wanting};
	}
	if (@r) {
		print "Verifying dependencies still match for ", 
		    join(', ', @r), "\n" if $state->{verbose};
		for my $wanting (@wantlist) {
			my $p2 = OpenBSD::PackingList->from_installation(
			    $wanting, \&OpenBSD::PackingList::DependOnly);
			if (!defined $p2) {
				Warn "Error: $wanting missing from installation\n"
			} else {
				$p2->validate_depend($state, $wanting, 
				    $toreplace, $replacement);
			}
		}
	}

	return $state->{okay} ? $plist : 0;
}

sub is_safe
{
	my ($plist, $state) = @_;
	$state->{okay} = 1;
	$state->{journal} = [];
	$plist->can_update(1, $state);
	if ($state->{okay} == 0) {
		Warn "New package ", $plist->pkgname, 
		    " contains potentially unsafe operations\n";
		for my $i (@{$state->{journal}}) {
			Warn "\t$i\n";
		}
		if ($state->{forced}->{update}) {
			Warn "(forcing update)\n";
			$state->{okay} = 1;
		} elsif ($state->{interactive}) {
			if (OpenBSD::Interactive::confirm("proceed with update anyways", 1, 0, 'update')) {
			    $state->{okay} = 1;
			}
		}
	}
	return $state->{okay};
}

sub split_some_libs
{
	my ($plist, $libs) = @_;
	my $c1 = {};
	my $c2 = {};
	$plist->separate_element($libs, $c1, $c2);
	my $p1 = $plist->make_deep_copy($c1);
	my $p2 = $plist->make_shallow_copy($c2);
	return ($p1, $p2);
}

# create a packing-list with only the libraries we want to keep around.
sub split_libs
{
	my ($plist, $to_split) = @_;

	(my $splitted, $plist) = split_some_libs($plist, $to_split);

	$splitted->set_pkgname(".libs-".$plist->pkgname);

	if (defined $plist->{'no-default-conflict'}) {
		# we conflict with the package we just removed...
		OpenBSD::PackingElement::Conflict->add($splitted, $plist->pkgname);
	} else {
		require OpenBSD::PackageName;

		my $stem = OpenBSD::PackageName::splitstem($plist->pkgname);
		OpenBSD::PackingElement::Conflict->add($splitted, $stem."-*");
	}
	return ($plist, $splitted);
}

sub adjust_depends_closure
{
	my ($oldname, $plist, $state) = @_;

	print "Packages that depend on those shared libraries:\n" 
	    if $state->{beverbose};

	my $write = OpenBSD::RequiredBy->new($plist->pkgname);
	for my $pkg (OpenBSD::RequiredBy->compute_closure($oldname)) {
		print "\t$pkg\n" if $state->{beverbose};
		$write->add($pkg);
		OpenBSD::Requiring->new($pkg)->add($plist->pkgname);
	}
}


sub save_old_libraries
{
	my ($set, $state) = @_;

	for my $o ($set->older) {

		my $oldname = $o->{pkgname};
		my $libs = {};
		my $p = {};

		print "Looking for changes in shared libraries\n" 
		    if $state->{beverbose};
		$o->{plist}->mark_lib($libs, $p);
		for my $n ($set->newer) {
			$n->{plist}->unmark_lib($libs, $p);
		}

		if (%$libs) {
			print "Libraries to keep: ", join(",", sort(keys %$libs)), "\n" 
			    if $state->{verbose};
			($o->{plist}, my $stub_list) = split_libs($o->{plist}, $libs);
			my $stub_name = $stub_list->pkgname;
			my $dest = installed_info($stub_name);
			print "Keeping them in $stub_name\n" if $state->{verbose};
			if ($state->{not}) {
				$stub_list->to_cache;
				$o->{plist}->to_cache;
			} else {
				require OpenBSD::md5;

				mkdir($dest);
				open my $descr, '>', $dest.DESC;
				print $descr "Stub libraries for $oldname\n";
				close $descr;
				my $f = OpenBSD::PackingElement::FDESC->add($stub_list, DESC);
				$f->{ignore} = 1;
				$f->{md5} = OpenBSD::md5::fromfile($dest.DESC);
				$stub_list->to_installation;
				$o->{plist}->to_installation;
			}
			add_installed($stub_name);

			require OpenBSD::PkgCfl;
			OpenBSD::PkgCfl::register($stub_list, $state);

			adjust_depends_closure($oldname, $stub_list, $state);
		} else {
			print "No libraries to keep\n" if $state->{verbose};
		}
	}
}

			
sub adjust_dependency
{
	my ($dep, $from, $into) = @_;

	my $l = OpenBSD::Requiring->new($dep);
	$l->delete($from);
	$l->add($into);
}

sub figure_out_libs
{
	my ($plist, $state, @libs) = @_;

	my $has = {};
	my $result = [];

	for my $item (@{$plist->{items}}) {
		next unless $item->IsFile;
		$has->{$item->fullname} = 1;
	}

	for my $oldlib (@libs) {
		print "Checking for collisions with $oldlib... " 
		    if $state->{verbose};

#		require OpenBSD::RequiredBy;
#
#		if (OpenBSD::RequiredBy->new($oldlib)->list == 0) {
#			require OpenBSD::Delete;
#
#			OpenBSD::Delete::delete_package($oldlib, $state);
#			delete_installed($oldlib);
#			next;
#		}

		my $p = OpenBSD::PackingList->from_installation($oldlib);
		my $n = [];
		my $delete = [];
		my $empty = 1;
		my $doit = 0;
		for my $file (@{$p->{items}}) {
			if ($file->IsFile) {
				if ($has->{$file->fullname}) {
					$doit = 1;
					push(@$delete, $file);
					next;
				} else {
					$empty = 0;
				}
			}
			push(@$n, $file);
		}
		$p->{items} = $n;
		if ($doit) {
			print "some found\n" if $state->{verbose};
			# XXX we don't use this code yet
			my $dummy = {items => $delete};
			push(@$result, 
			    { plist => $p, 
			      todelete => $dummy,
			      empty => $empty});
			#require OpenBSD::Delete;
			#OpenBSD::Delete::validate_plist($p, $state);
		} else {
			print "none found\n" if $state->{verbose};
		}
	}
	if (@$result) {
		#$plist->{old_libs} = $result;
		return 0;
	}
	return 1;
}

1;
