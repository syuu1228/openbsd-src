#	$OpenBSD: src/distrib/sparc/floppies/Attic/runlist.sh,v 1.2 1997/05/01 11:30:46 niklas Exp $

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
