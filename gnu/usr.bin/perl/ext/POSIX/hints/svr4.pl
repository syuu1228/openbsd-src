# For NCR systems with uname -a output like the following:
#	foo foo 4.0 3.0 3441 Pentium III(TM)-ISA/PCI
#	foo foo 4.0 3.0 4400 Pentium II(TM)-ISA/PCI
# the system needs to explicitly link against -lmw to pull in some
# symbols such as _mwoflocheckl and possibly others.
#	Configure sets archname='3441-svr4.0' or '4400-svr4.0'
# The regex below is an attempt to get both systems as well as
# any reasonable future permutations.
# Thanks to Doug Hendricks for the original info.
#	A. Dougherty  Tue Oct 30 10:20:07 EST 2001
if ($Config{'archname'} =~ /[34]4[0-9][0-9]-svr4/) {
    $self->{LIBS} = ['-lm -posix -lcposix -lmw'];
}
# Not sure what OS this one is.
elsif ($Config{archname} =~ /RM\d\d\d-svr4/) {
    $self->{LIBS} = ['-lm -lc -lposix -lcposix'];
}
