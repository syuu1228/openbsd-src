#	$OpenBSD: src/sbin/isakmpd/genconstants.sh,v 1.11 2003/06/03 14:28:16 ho Exp $
#	$EOM: genconstants.sh,v 1.6 1999/04/02 01:15:53 niklas Exp $

#
# Copyright (c) 1998, 1999, 2001 Niklas Hallqvist.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# This code was written under funding by Ericsson Radio Systems.
#

base=`basename $1`
upcased_name=`echo $base |tr a-z A-Z`

awk=${AWK:-awk}

locase_function='function locase (str) {
  cmd = "echo " str " |tr A-Z a-z"
  cmd | getline retval;
  close (cmd);
  return retval;
}'

$awk "
$locase_function
"'
BEGIN {
  print "/* DO NOT EDIT-- this file is automatically generated.  */\n"
  print "#ifndef _'$upcased_name'_H_"
  print "#define _'$upcased_name'_H_\n"
  print "#include \"sysdep.h\"\n"
  print "#include \"constants.h\"\n"
}

/^[#.]/ {
  next
}

/^[^ 	]/ {
  prefix = $1
  printf ("extern struct constant_map %s_cst[];\n\n", locase(prefix));
  next
}

/^[ 	]/ && $1 {
  printf ("#define %s_%s %s\n", prefix, $1, $2)
  next
}

{
    print
}

END {
  printf ("\n")
  print "#endif /* _'$upcased_name'_H_ */"
}
' <$1.cst >$base.h

$awk "
$locase_function
"'
BEGIN {
  print "/* DO NOT EDIT-- this file is automatically generated.  */\n"
  print "#include \"sysdep.h\"\n"
  print "#include \"constants.h\""
  print "#include \"'$base'.h\"\n"
}

/^#/ {
  next
}

/^\./ {
  print "  { 0, 0 }\n};\n"
  next
}

/^[^ 	]/ {
  prefix = $1
  printf ("struct constant_map %s_cst[] = {\n", locase(prefix))
  next
}

/^[ 	]/ && $1 {
  printf ("  { %s_%s, \"%s\", %s }, \n", prefix, $1, $1,
    ($3 && substr($3,1,1) != "#") ? $3 : 0)
  next
}

{
  print
}
' <$1.cst >$base.c
