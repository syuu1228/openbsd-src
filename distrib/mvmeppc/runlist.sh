#	$OpenBSD: src/distrib/mvmeppc/Attic/runlist.sh,v 1.1 2001/06/26 22:23:23 smurph Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

( while [ "X$1" != "X" ]; do
	cat $1
	shift
done ) | awk -f ${TOPDIR}/list2sh.awk | ${SHELLCMD}
