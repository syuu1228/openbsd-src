#	$OpenBSD: src/distrib/mvme88k/Attic/runlist.sh,v 1.1 1998/12/17 02:16:30 smurph Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh"
fi

( while [ "X$1" != "X" ]; do
	cat $1
	shift
done ) | awk -f ${TOPDIR}/list2sh.awk | ${SHELLCMD}
