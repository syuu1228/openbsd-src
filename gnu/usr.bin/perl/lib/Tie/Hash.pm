package Tie::Hash;

our $VERSION = '1.00';

=head1 NAME

Tie::Hash, Tie::StdHash, Tie::ExtraHash - base class definitions for tied hashes

=head1 SYNOPSIS

    package NewHash;
    require Tie::Hash;

    @ISA = (Tie::Hash);

    sub DELETE { ... }		# Provides needed method
    sub CLEAR { ... }		# Overrides inherited method


    package NewStdHash;
    require Tie::Hash;

    @ISA = (Tie::StdHash);

    # All methods provided by default, define only those needing overrides
    # Accessors access the storage in %{$_[0]};
    # TIEHANDLE should return a reference to the actual storage
    sub DELETE { ... }

    package NewExtraHash;
    require Tie::Hash;

    @ISA = (Tie::ExtraHash);

    # All methods provided by default, define only those needing overrides
    # Accessors access the storage in %{$_[0][0]};
    # TIEHANDLE should return an array reference with the first element being
    # the reference to the actual storage 
    sub DELETE { 
      $_[0][1]->('del', $_[0][0], $_[1]); # Call the report writer
      delete $_[0][0]->{$_[1]};		  #  $_[0]->SUPER::DELETE($_[1]) }


    package main;

    tie %new_hash, 'NewHash';
    tie %new_std_hash, 'NewStdHash';
    tie %new_extra_hash, 'NewExtraHash',
	sub {warn "Doing \U$_[1]\E of $_[2].\n"};

=head1 DESCRIPTION

This module provides some skeletal methods for hash-tying classes. See
L<perltie> for a list of the functions required in order to tie a hash
to a package. The basic B<Tie::Hash> package provides a C<new> method, as well
as methods C<TIEHASH>, C<EXISTS> and C<CLEAR>. The B<Tie::StdHash> and
B<Tie::ExtraHash> packages
provide most methods for hashes described in L<perltie> (the exceptions
are C<UNTIE> and C<DESTROY>).  They cause tied hashes to behave exactly like standard hashes,
and allow for selective overwriting of methods.  B<Tie::Hash> grandfathers the
C<new> method: it is used if C<TIEHASH> is not defined
in the case a class forgets to include a C<TIEHASH> method.

For developers wishing to write their own tied hashes, the required methods
are briefly defined below. See the L<perltie> section for more detailed
descriptive, as well as example code:

=over 4

=item TIEHASH classname, LIST

The method invoked by the command C<tie %hash, classname>. Associates a new
hash instance with the specified class. C<LIST> would represent additional
arguments (along the lines of L<AnyDBM_File> and compatriots) needed to
complete the association.

=item STORE this, key, value

Store datum I<value> into I<key> for the tied hash I<this>.

=item FETCH this, key

Retrieve the datum in I<key> for the tied hash I<this>.

=item FIRSTKEY this

Return the first key in the hash.

=item NEXTKEY this, lastkey

Return the next key in the hash.

=item EXISTS this, key

Verify that I<key> exists with the tied hash I<this>.

The B<Tie::Hash> implementation is a stub that simply croaks.

=item DELETE this, key

Delete the key I<key> from the tied hash I<this>.

=item CLEAR this

Clear all values from the tied hash I<this>.

=back

=head1 Inheriting from B<Tie::StdHash>

The accessor methods assume that the actual storage for the data in the tied
hash is in the hash referenced by C<tied(%tiedhash)>.  Thus overwritten
C<TIEHANDLE> method should return a hash reference, and the remaining methods
should operate on the hash referenced by the first argument:

  package ReportHash;
  our @ISA = 'Tie::StdHash';

  sub TIEHASH  {
    my $storage = bless {}, shift;
    warn "New ReportHash created, stored in $storage.\n";
    $storage
  }
  sub STORE    {
    warn "Storing data with key $_[1] at $_[0].\n";
    $_[0]{$_[1]} = $_[2]
  }


=head1 Inheriting from B<Tie::ExtraHash>

The accessor methods assume that the actual storage for the data in the tied
hash is in the hash referenced by C<(tied(%tiedhash))[0]>.  Thus overwritten
C<TIEHANDLE> method should return an array reference with the first
element being a hash reference, and the remaining methods should operate on the
hash C<< %{ $_[0]->[0] } >>:

  package ReportHash;
  our @ISA = 'Tie::StdHash';

  sub TIEHASH  {
    my $storage = bless {}, shift;
    warn "New ReportHash created, stored in $storage.\n";
    [$storage, @_]
  }
  sub STORE    {
    warn "Storing data with key $_[1] at $_[0].\n";
    $_[0][0]{$_[1]} = $_[2]
  }

The default C<TIEHANDLE> method stores "extra" arguments to tie() starting
from offset 1 in the array referenced by C<tied(%tiedhash)>; this is the
same storage algorithm as in TIEHASH subroutine above.  Hence, a typical
package inheriting from B<Tie::ExtraHash> does not need to overwrite this
method.

=head1 C<UNTIE> and C<DESTROY>

The methods C<UNTIE> and C<DESTROY> are not defined in B<Tie::Hash>,
B<Tie::StdHash>, or B<Tie::ExtraHash>.  Tied hashes do not require
presense of these methods, but if defined, the methods will be called in
proper time, see L<perltie>.

If needed, these methods should be defined by the package inheriting from
B<Tie::Hash>, B<Tie::StdHash>, or B<Tie::ExtraHash>.

=head1 MORE INFORMATION

The packages relating to various DBM-related implementations (F<DB_File>,
F<NDBM_File>, etc.) show examples of general tied hashes, as does the
L<Config> module. While these do not utilize B<Tie::Hash>, they serve as
good working examples.

=cut

use Carp;
use warnings::register;

sub new {
    my $pkg = shift;
    $pkg->TIEHASH(@_);
}

# Grandfather "new"

sub TIEHASH {
    my $pkg = shift;
    if (defined &{"${pkg}::new"}) {
	warnings::warnif("WARNING: calling ${pkg}->new since ${pkg}->TIEHASH is missing");
	$pkg->new(@_);
    }
    else {
	croak "$pkg doesn't define a TIEHASH method";
    }
}

sub EXISTS {
    my $pkg = ref $_[0];
    croak "$pkg doesn't define an EXISTS method";
}

sub CLEAR {
    my $self = shift;
    my $key = $self->FIRSTKEY(@_);
    my @keys;

    while (defined $key) {
	push @keys, $key;
	$key = $self->NEXTKEY(@_, $key);
    }
    foreach $key (@keys) {
	$self->DELETE(@_, $key);
    }
}

# The Tie::StdHash package implements standard perl hash behaviour.
# It exists to act as a base class for classes which only wish to
# alter some parts of their behaviour.

package Tie::StdHash;
# @ISA = qw(Tie::Hash);		# would inherit new() only

sub TIEHASH  { bless {}, $_[0] }
sub STORE    { $_[0]->{$_[1]} = $_[2] }
sub FETCH    { $_[0]->{$_[1]} }
sub FIRSTKEY { my $a = scalar keys %{$_[0]}; each %{$_[0]} }
sub NEXTKEY  { each %{$_[0]} }
sub EXISTS   { exists $_[0]->{$_[1]} }
sub DELETE   { delete $_[0]->{$_[1]} }
sub CLEAR    { %{$_[0]} = () }

package Tie::ExtraHash;

sub TIEHASH  { my $p = shift; bless [{}, @_], $p }
sub STORE    { $_[0][0]{$_[1]} = $_[2] }
sub FETCH    { $_[0][0]{$_[1]} }
sub FIRSTKEY { my $a = scalar keys %{$_[0][0]}; each %{$_[0][0]} }
sub NEXTKEY  { each %{$_[0][0]} }
sub EXISTS   { exists $_[0][0]->{$_[1]} }
sub DELETE   { delete $_[0][0]->{$_[1]} }
sub CLEAR    { %{$_[0][0]} = () }

1;
