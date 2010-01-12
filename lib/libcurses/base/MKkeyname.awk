# $OpenBSD: src/lib/libcurses/base/MKkeyname.awk,v 1.5 2010/01/12 23:22:05 nicm Exp $
# $Id: MKkeyname.awk,v 1.40 2008/07/12 18:40:00 tom Exp $
##############################################################################
# Copyright (c) 1999-2007,2008 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
BEGIN {
	print "/* generated by MKkeyname.awk */"
	print ""
	print "#include <curses.priv.h>"
	print "#include <tic.h>"
	print "#include <term_entry.h>"
	print ""
	first = 1;
}

/^[^#]/ {
		if (bigstrings) {
			if (first)  {
				print "struct kn { short offset; int code; };"
				print "static const struct kn _nc_key_names[] = {"
			}
			printf "\t{ %d, %s },\n", offset, $1
			offset += length($1) + 1
			names = names"\n\t\""$1"\\0\""
		} else {
			if (first) {
				print "struct kn { const char *name; int code; };"
				print "static const struct kn _nc_key_names[] = {"
			}
			printf "\t{ \"%s\", %s },\n", $1, $1;
		}
		first = 0;
	}

END {
	if (bigstrings) {
		printf "\t{ -1, 0 }};\n"
		print ""
		print "static const char key_names[] = "names";"
	} else {
		printf "\t{ 0, 0 }};\n"
	}
	print ""
	print "#define SIZEOF_TABLE 256"
	print "#define MyTable _nc_globals.keyname_table"
	print ""
	print "NCURSES_EXPORT(NCURSES_CONST char *) _nc_keyname (SCREEN *sp, int c)"
	print "{"
	print "	int i;"
	print "	char name[20];"
	print "	char *p;"
	print "	size_t psize;"
	print "	NCURSES_CONST char *result = 0;"
	print ""
	print "	if (c == -1) {"
	print "		result = \"-1\";"
	print "	} else {"
	if (bigstrings) {
		print "		for (i = 0; _nc_key_names[i].offset != -1; i++) {"
		print "			if (_nc_key_names[i].code == c) {"
		print "				result = (NCURSES_CONST char *)key_names + _nc_key_names[i].offset;"
		print "				break;"
		print "			}"
		print "		}"
	} else {
		print "		for (i = 0; _nc_key_names[i].name != 0; i++) {"
		print "			if (_nc_key_names[i].code == c) {"
		print "				result = (NCURSES_CONST char *)_nc_key_names[i].name;"
		print "				break;"
		print "			}"
		print "		}"
	}
	print ""
	print "		if (result == 0 && (c >= 0 && c < SIZEOF_TABLE)) {"
	print "			if (MyTable == 0)"
	print "				MyTable = typeCalloc(char *, SIZEOF_TABLE);"
	print "			if (MyTable != 0) {"
	print "				if (MyTable[c] == 0) {"
	print "					int cc = c;"
	print "					p = name;"
	print "					psize = sizeof(name);"
	print "					if (cc >= 128 && (sp == 0 || sp->_use_meta)) {"
	print "						strlcpy(p, \"M-\", psize);"
	print "						p += 2;"
	print "						psize -= 2;"
	print "						cc -= 128;"
	print "					}"
	print "					if (cc < 32)"
	print "						snprintf(p, psize, \"^%c\", cc + '@');"
	print "					else if (cc == 127)"
	print "						strlcpy(p, \"^?\", psize);"
	print "					else"
	print "						snprintf(p, psize, \"%c\", cc);"
	print "					MyTable[c] = strdup(name);"
	print "				}"
	print "				result = MyTable[c];"
	print "			}"
	print "#if NCURSES_EXT_FUNCS && NCURSES_XNAMES"
	print "		} else if (result == 0 && cur_term != 0) {"
	print "			int j, k;"
	print "			char * bound;"
	print "			TERMTYPE *tp = &(cur_term->type);"
	print "			int save_trace = _nc_tracing;"
	print ""
	print "			_nc_tracing = 0;	/* prevent recursion via keybound() */"
	print "			for (j = 0; (bound = keybound(c, j)) != 0; ++j) {"
	print "				for(k = STRCOUNT; k < (int) NUM_STRINGS(tp);  k++) {"
	print "					if (tp->Strings[k] != 0 && !strcmp(bound, tp->Strings[k])) {"
	print "						result = ExtStrname(tp, k, strnames);"
	print "						break;"
	print "					}"
	print "				}"
	print "				free(bound);"
	print "				if (result != 0)"
	print "					break;"
	print "			}"
	print "			_nc_tracing = save_trace;"
	print "#endif"
	print "		}"
	print "	}"
	print "	return result;"
	print "}"
	print ""
	print "NCURSES_EXPORT(NCURSES_CONST char *) keyname (int c)"
	print "{"
	print "\treturn _nc_keyname(SP, c);"
	print "}"
	print ""
	print "#if NO_LEAKS"
	print "void _nc_keyname_leaks(void)"
	print "{"
	print "	int j;"
	print "	if (MyTable != 0) {"
	print "		for (j = 0; j < SIZEOF_TABLE; ++j) {"
	print "			FreeIfNeeded(MyTable[j]);"
	print "		}"
	print "		FreeAndNull(MyTable);"
	print "	}"
	print "}"
	print "#endif /* NO_LEAKS */"
}
