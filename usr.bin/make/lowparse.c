/*	$OpenBSD: src/usr.bin/make/lowparse.c,v 1.2 2000/06/23 16:40:50 espie Exp $ */

/* low-level parsing functions. */

/*
 * Copyright (c) 1999,2000 Marc Espie.
 *
 * Extensive code changes for the OpenBSD project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE OPENBSD PROJECT AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OPENBSD
 * PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "make.h"
#include "buf.h"
#include "lowparse.h"

#ifdef CLEANUP
static LIST	    fileNames;	/* file names to free at end */
#endif

/* Definitions for handling #include specifications */
typedef struct IFile_ {
    char           	*fname;	/* name of file */
    unsigned long      	lineno;	/* line number */
    FILE 		*F;	/* open stream */
    char 		*str;	/* read from char area */	
    char 		*ptr;	/* where we are */
    char 		*end;	/* don't overdo it */
} IFile;

static IFile	*current;



static LIST     includes;  	/* stack of IFiles generated by
				 * #includes */

static IFile *new_ifile __P((char *, FILE *));
static IFile *new_istring __P((char *, char *, unsigned long));
static void free_ifile __P((IFile *));
static void ParseVErrorInternal __P((char *, unsigned long, int, char *, va_list));
static int skiptoendofline __P((void));
static int newline __P((void));
#define ParseReadc()	current->ptr < current->end ? *current->ptr++ : newline()
static void ParseUnreadc __P((char));
static int ParseSkipEmptyLines __P((Buffer));
static int	    fatals = 0;

/*-
 * ParseVErrorInternal  --
 *	Error message abort function for parsing. Prints out the context
 *	of the error (line number and file) as well as the message with
 *	two optional arguments.
 *
 * Side Effects:
 *	"fatals" is incremented if the level is PARSE_FATAL.
 */
/* VARARGS */
static void
#ifdef __STDC__
ParseVErrorInternal(char *cfname, unsigned long clineno, int type, char *fmt,
    va_list ap)
#else
ParseVErrorInternal(va_alist)
	va_dcl
#endif
{
	(void)fprintf(stderr, "\"%s\", line %lu: ", cfname, clineno);
	if (type == PARSE_WARNING)
		(void)fprintf(stderr, "warning: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	if (type == PARSE_FATAL)
		fatals ++;
}

/*-
 * Parse_Error  --
 *	External interface to ParseVErrorInternal; uses the default filename
 *	Line number.
 */
/* VARARGS */
void
#ifdef __STDC__
Parse_Error(int type, char *fmt, ...)
#else
Parse_Error(va_alist)
	va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	int type;		/* Error type (PARSE_WARNING, PARSE_FATAL) */
	char *fmt;

	va_start(ap);
	type = va_arg(ap, int);
	fmt = va_arg(ap, char *);
#endif

	ParseVErrorInternal(current->fname, current->lineno, type, fmt, ap);
}

static IFile *
new_ifile(name, stream)
    char *name;
    FILE *stream;
{
    IFile *ifile;
#ifdef CLEANUP
    Lst_AtEnd(&fileNames, name);
#endif

    ifile = emalloc(sizeof(*ifile));
    ifile->fname = name;
    /* Naturally enough, we start reading at line 0 */
    ifile->lineno = 0;
    ifile->F = stream;
    ifile->ptr = ifile->end = NULL;
    return ifile;
}

static void
free_ifile(ifile)
    IFile *ifile;
{
    if (ifile->F)
    	(void)fclose(ifile->F);
    else
    	free(ifile->str);
    /* Note we can't free the file names yet, as they are embedded in GN for
     * error reports. */
    free(ifile);
}

static IFile *
new_istring(str, name, lineno)
    char *str;
    char *name;
    unsigned long lineno;
{
    IFile *ifile;

    ifile = emalloc(sizeof(*ifile));
    ifile->fname = name;
    ifile->F = NULL;
    /* Strings are used from for loops... */
    ifile->lineno = lineno;
    ifile->ptr = ifile->str = str;
    ifile->end = str + strlen(str);
    return ifile;
}


/*-
 *---------------------------------------------------------------------
 * Parse_FromString  --
 *	Start Parsing from the given string
 *
 * Side Effects:
 *	A structure is added to the includes Lst and readProc, lineno,
 *	fname and curFILE are altered for the new file
 *---------------------------------------------------------------------
 */
void
Parse_FromString(str, lineno)
    char 		*str;
    unsigned long 	lineno;
{
    if (DEBUG(FOR))
	(void)fprintf(stderr, "%s\n----\n", str);

    if (current != NULL)
	Lst_AtFront(&includes, current);
    current = new_istring(str, current->fname, lineno);
}


void 
Parse_FromFile(name, stream)
    char		*name;
    FILE		*stream;
{
    if (current != NULL)
    	Lst_AtFront(&includes, current);
    current = new_ifile(name, stream);
}

/*-
 *---------------------------------------------------------------------
 * Parse_NextFile -- 
 *	Called when EOF is reached in the current file. If we were reading
 *	an include file, the includes stack is popped and things set up
 *	to go back to reading the previous file at the previous location.
 *
 * Results:
 *	CONTINUE if there's more to do. DONE if not.
 *
 * Side Effects:
 *	The old curFILE, is closed. The includes list is shortened.
 *	lineno, curFILE, and fname are changed if CONTINUE is returned.
 *---------------------------------------------------------------------
 */
Boolean
Parse_NextFile()
{
    IFile *next;

    next = (IFile *)Lst_DeQueue(&includes);
    if (next != NULL) {
	if (current != NULL)
	    free_ifile(current);
	current = next;
    	return TRUE;
    } else
    	return FALSE;
}


int
newline()
{
    size_t len;

    if (current->F) {
	current->ptr = fgetln(current->F, &len);
	if (current->ptr) {
	    current->end = current->ptr + len;
	    return *current->ptr++;
	} else {
	    current->end = NULL;
	}
    }
    return EOF;
}

void 
ParseUnreadc(c)
	char c;
{
	current->ptr--;
	*current->ptr = c;
}

/* ParseSkipLine():
 *	Grab the next line
 */
char *
ParseSkipLine(skip)
    int skip; 		/* Skip lines that don't start with . */
{
    char *line;
    int c, lastc;
    BUFFER buf;

    Buf_Init(&buf, MAKE_BSIZE);

    for (;;) {
        Buf_Reset(&buf);
        lastc = '\0';

        while (((c = ParseReadc()) != '\n' || lastc == '\\')
               && c != EOF) {
            if (c == '\n') {
                Buf_ReplaceLastChar(&buf, ' ');
                current->lineno++;

                while ((c = ParseReadc()) == ' ' || c == '\t');

                if (c == EOF)
                    break;
            }

            Buf_AddChar(&buf, c);
            lastc = c;
        }

        line = Buf_Retrieve(&buf);
        current->lineno++;
	    /* allow for non-newline terminated lines while skipping */
	if (line[0] == '.')
	    break;

        if (c == EOF) {
            Parse_Error(PARSE_FATAL, "Unclosed conditional/for loop");
            Buf_Destroy(&buf);
            return NULL;
        }
	if (skip == 0)
	    break;

    }

    return line;
}


/*-
 *---------------------------------------------------------------------
 * ParseReadLine --
 *	Read an entire line from the input file. Called only by Parse_File.
 *	To facilitate escaped newlines and what have you, a character is
 *	buffered in 'lastc', which is '\0' when no characters have been
 *	read. When we break out of the loop, c holds the terminating
 *	character and lastc holds a character that should be added to
 *	the line (unless we don't read anything but a terminator).
 *
 * Results:
 *	A line w/o its newline
 *
 * Side Effects:
 *	Only those associated with reading a character
 *---------------------------------------------------------------------
 */
char *
ParseReadLine ()
{
    BUFFER  	  buf;	    	/* Buffer for current line */
    register int  c;	      	/* the current character */
    register int  lastc;    	/* The most-recent character */
    Boolean	  semiNL;     	/* treat semi-colons as newlines */
    Boolean	  ignDepOp;   	/* TRUE if should ignore dependency operators
				 * for the purposes of setting semiNL */
    Boolean 	  ignComment;	/* TRUE if should ignore comments (in a
				 * shell command */
    char 	  *line;    	/* Result */
    char          *ep;		/* to strip trailing blanks */

    semiNL = FALSE;
    ignDepOp = FALSE;
    ignComment = FALSE;

    /*
     * Handle special-characters at the beginning of the line. Either a
     * leading tab (shell command) or pound-sign (possible conditional)
     * forces us to ignore comments and dependency operators and treat
     * semi-colons as semi-colons (by leaving semiNL FALSE). This also
     * discards completely blank lines.
     */
    for (;;) {
	c = ParseReadc();

	if (c == '\t') {
	    ignComment = ignDepOp = TRUE;
	    break;
	} else if (c == '\n') {
	    current->lineno++;
	} else if (c == '#') {
	    ParseUnreadc(c);
	    break;
	} else {
	    /*
	     * Anything else breaks out without doing anything
	     */
	    break;
	}
    }

    if (c != EOF) {
	lastc = c;
	Buf_Init(&buf, MAKE_BSIZE);

	while (((c = ParseReadc ()) != '\n' || (lastc == '\\')) &&
	       (c != EOF))
	{
test_char:
	    switch(c) {
	    case '\n':
		/*
		 * Escaped newline: read characters until a non-space or an
		 * unescaped newline and replace them all by a single space.
		 * This is done by storing the space over the backslash and
		 * dropping through with the next nonspace. If it is a
		 * semi-colon and semiNL is TRUE, it will be recognized as a
		 * newline in the code below this...
		 */
		current->lineno++;
		lastc = ' ';
		while ((c = ParseReadc ()) == ' ' || c == '\t') {
		    continue;
		}
		if (c == EOF || c == '\n') {
		    goto line_read;
		} else {
		    /*
		     * Check for comments, semiNL's, etc. -- easier than
		     * ParseUnreadc(c); continue;
		     */
		    goto test_char;
		}
		/*NOTREACHED*/
		break;

	    case ';':
		/*
		 * Semi-colon: Need to see if it should be interpreted as a
		 * newline
		 */
		if (semiNL) {
		    /*
		     * To make sure the command that may be following this
		     * semi-colon begins with a tab, we push one back into the
		     * input stream. This will overwrite the semi-colon in the
		     * buffer. If there is no command following, this does no
		     * harm, since the newline remains in the buffer and the
		     * whole line is ignored.
		     */
		    ParseUnreadc('\t');
		    goto line_read;
		}
		break;
	    case '=':
		if (!semiNL) {
		    /*
		     * Haven't seen a dependency operator before this, so this
		     * must be a variable assignment -- don't pay attention to
		     * dependency operators after this.
		     */
		    ignDepOp = TRUE;
		} else if (lastc == ':' || lastc == '!') {
		    /*
		     * Well, we've seen a dependency operator already, but it
		     * was the previous character, so this is really just an
		     * expanded variable assignment. Revert semi-colons to
		     * being just semi-colons again and ignore any more
		     * dependency operators.
		     *
		     * XXX: Note that a line like "foo : a:=b" will blow up,
		     * but who'd write a line like that anyway?
		     */
		    ignDepOp = TRUE; semiNL = FALSE;
		}
		break;
	    case '#':
		if (!ignComment) {
		    if (
#if 0
		    compatMake &&
#endif
		    (lastc != '\\')) {
			/*
			 * If the character is a hash mark and it isn't escaped
			 * (or we're being compatible), the thing is a comment.
			 * Skip to the end of the line.
			 */
			do {
			    c = ParseReadc();
			} while ((c != '\n') && (c != EOF));
			goto line_read;
		    } else {
			/*
			 * Don't add the backslash. Just let the # get copied
			 * over.
			 */
			lastc = c;
			continue;
		    }
		}
		break;
	    case ':':
	    case '!':
		if (!ignDepOp && (c == ':' || c == '!')) {
		    /*
		     * A semi-colon is recognized as a newline only on
		     * dependency lines. Dependency lines are lines with a
		     * colon or an exclamation point. Ergo...
		     */
		    semiNL = TRUE;
		}
		break;
	    }
	    /*
	     * Copy in the previous character and save this one in lastc.
	     */
	    Buf_AddChar(&buf, lastc);
	    lastc = c;

	}
    line_read:
	current->lineno++;

	if (lastc != '\0') 
	    Buf_AddChar(&buf, lastc);
	line = Buf_Retrieve(&buf);

	/*
	 * Strip trailing blanks and tabs from the line.
	 * Do not strip a blank or tab that is preceeded by
	 * a '\'
	 */
	ep = line;
	while (*ep)
	    ++ep;
	while (ep > line + 1 && (ep[-1] == ' ' || ep[-1] == '\t')) {
	    if (ep > line + 1 && ep[-2] == '\\')
		break;
	    --ep;
	}
	*ep = 0;

	if (line[0] == '.') {
	    /*
	     * The line might be a conditional. Ask the conditional module
	     * about it and act accordingly
	     */
	    switch (Cond_Eval (line)) {
	    case COND_SKIP:
		/*
		 * Skip to next conditional that evaluates to COND_PARSE.
		 */
		do {
		    free (line);
		    line = ParseSkipLine(1);
		} while (line && Cond_Eval(line) != COND_PARSE);
		if (line == NULL)
		    break;
		/*FALLTHRU*/
	    case COND_PARSE:
		free(line);
		line = ParseReadLine();
		break;
	    case COND_INVALID:
	    	{
		For *loop;

		loop = For_Eval(line);
		if (loop != NULL) {
		    Boolean ok;

		    free(line);
		    do {
			/* Find the matching endfor.  */
			line = ParseSkipLine(0);
			if (line == NULL) {
			    Parse_Error(PARSE_FATAL,
				     "Unexpected end of file in for loop.\n");
			    return line;
			}
			ok = For_Accumulate(loop, line);
			free(line);
		    } while (ok);
		    For_Run(loop);
		    line = ParseReadLine();
		}
		break;
		}
	    }
	}
	return (line);

    } else {
	/*
	 * Hit end-of-file, so return a NULL line to indicate this.
	 */
	return((char *)NULL);
    }
}

unsigned long
Parse_Getlineno()
{
    return current->lineno;
}

const char *
Parse_Getfilename()
{
    return current->fname;
}

#ifdef CLEANUP
void
LowParse_Init()
{
    Lst_Init(&includes);
    current = NULL;
}

void
LowParse_End()
{
    Lst_Destroy(&includes, NOFREE);	/* Should be empty now */
}
#endif
	

void
Finish_Errors()
{
    if (current != NULL) {
    	free_ifile(current);
	current = NULL;
    }
    if (fatals) {
	fprintf(stderr, "Fatal errors encountered -- cannot continue\n");
	exit(1);
    }
}
