# convexos.sh
# Thanks to David Starks-Browning <dstarks@rc.tudelft.nl>
# Date: Tue, 17 Jan 1995 10:45:03 -0500 (EST)
# Subject: Re: Hints for ConvexOS 10.2
# 
# uname -a output looks like
#   ConvexOS  xxxx C38xx  10.2 convex
# Configure may incorrectly assign $3 to $osvers.
#
set X $myuname
shift
osvers=$4
# ConvexOS 10.2 uses POSIX process group semantics for getpgrp but
# BSD semantics for setpgrp.  Perl assumes you don't have such
# a mixed system, so we undef d_getpgrp.
#   Andy Dougherty		doughera@lafcol.lafayette.edu
#
case "$osvers" in
10.2)	d_getpgrp='undef' ;;
esac
