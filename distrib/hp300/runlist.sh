#	$OpenBSD: src/distrib/hp300/Attic/runlist.sh,v 1.1 1997/02/16 18:19:57 downsj Exp $
#	$NetBSD: runlist.sh,v 1.1 1995/07/18 04:13:01 briggs Exp $

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
