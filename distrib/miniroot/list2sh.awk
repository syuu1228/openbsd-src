#	$OpenBSD: src/distrib/miniroot/list2sh.awk,v 1.8 2002/04/30 01:30:41 deraadt Exp $
#	$NetBSD: list2sh.awk,v 1.2 1996/05/04 15:45:31 pk Exp $

BEGIN {
	printf("cd ${OBJDIR}\n");
	printf("\n");
}
/^$/ || /^#/ {
	print $0;
	next;
}
$1 == "COPY" {
	printf("echo '%s'\n", $0);
	printf("test -f ${TARGDIR}/%s && rm -fr ${TARGDIR}/%s\n", $3, $3);
	printf("cp %s ${TARGDIR}/%s\n", $2, $3);
	next;
}
$1 == "LINK" {
	printf("echo '%s'\n", $0);
	for (i = 3; i <= NF; i++) {
		printf("test -f ${TARGDIR}/%s && rm -f ${TARGDIR}/%s\n", $i, $i);
		printf("(cd ${TARGDIR}; ln %s %s)\n", $2, $i);
	}
	next;
}
$1 == "SYMLINK" {
	printf("echo '%s'\n", $0);
	for (i = 3; i <= NF; i++) {
		printf("test -f ${TARGDIR}/%s && rm -f ${TARGDIR}/%s\n", $i, $i);
		printf("(cd ${TARGDIR}; ln -s %s %s)\n", $2, $i);
	}
	next;
}
$1 == "ARGVLINK" {
	# crunchgen directive; ignored here
	next;
}
$1 == "SRCDIRS" {
	# crunchgen directive; ignored here
	next;
}
$1 == "CRUNCHSPECIAL" {
	# crunchgen directive; ignored here
	next;
}
$1 == "COPYDIR" {
	printf("echo '%s'\n", $0);
	printf("(cd ${TARGDIR}/%s && find . ! -name . | xargs /bin/rm -rf)\n",
	    $3);
	printf("(cd %s && pax -pe -rw . ${TARGDIR}/%s)\n", $2, $3);
	next;
}
$1 == "SPECIAL" {
# escaping shell quotation is ugly whether you use " or ', use cat <<'!' ...
	work=$0;
	gsub("[\\\\]", "\\\\", work);
	gsub("[\"]", "\\\"", work);
	gsub("[$]", "\\$", work);
	gsub("[`]", "\\`", work);
	printf("echo \"%s\"\n", work);
	work=$0;
	sub("^[ 	]*" $1 "[ 	]*", "", work);
	printf("(cd ${TARGDIR}; %s)\n", work);
	next;
}
$1 == "TERMCAP" {
	printf("echo '%s'\n", $0);
	printf("(cd ${TARGDIR}; tic -C -x -r -e %s ${UTILS}/../../share/termtypes/termtypes.master | sed -e '/^#.*/d' -e 's,/usr/share/lib/tabset,/usr/share/tabset,g' -e 's,/usr/lib/tabset,/usr/share/tabset,g' > %s)\n",
	    $2, $3);
	next;
}
$1 == "SCRIPT" {
	printf("echo '%s'\n", $0);
	printf("sed -e '/^[ 	]*#[ 	].*$/d' -e '/^[ 	]*#$/d' -e \"s/^ARCH=ARCH$/ARCH=`arch -ks`/\" < %s > ${TARGDIR}/%s\n",
	    $2, $3);
	next;
}
{
	printf("echo '%s'\n", $0);
	printf("echo 'Unknown keyword \"%s\" at line %d of input.'\n", $1, NR);
	printf("exit 1\n");
	exit 1;
}
END {
	printf("\n");
	printf("exit 0\n");
	exit 0;
}
