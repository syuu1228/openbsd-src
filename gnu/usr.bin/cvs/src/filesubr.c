/* filesubr.c --- subroutines for dealing with files
   Jim Blandy <jimb@cyclic.com>

   This file is part of GNU CVS.

   GNU CVS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/* These functions were moved out of subr.c because they need different
   definitions under operating systems (like, say, Windows NT) with different
   file system semantics.  */

#include "cvs.h"

/*
 * I don't know of a convenient way to test this at configure time, or else
 * I'd certainly do it there.
 */
#if defined(NeXT)
#define LOSING_TMPNAM_FUNCTION
#endif

static int deep_remove_dir PROTO((const char *path));

/*
 * Copies "from" to "to".
 */
void
copy_file (from, to)
    const char *from;
    const char *to;
{
    struct stat sb;
    struct utimbuf t;
    int fdin, fdout;

    if (trace)
#ifdef SERVER_SUPPORT
	(void) fprintf (stderr, "%c-> copy(%s,%s)\n",
			(server_active) ? 'S' : ' ', from, to);
#else
	(void) fprintf (stderr, "-> copy(%s,%s)\n", from, to);
#endif
    if (noexec)
	return;

    if ((fdin = open (from, O_RDONLY)) < 0)
	error (1, errno, "cannot open %s for copying", from);
    if (fstat (fdin, &sb) < 0)
	error (1, errno, "cannot fstat %s", from);
    if ((fdout = creat (to, (int) sb.st_mode & 07777)) < 0)
	error (1, errno, "cannot create %s for copying", to);
    if (sb.st_size > 0)
    {
	char buf[BUFSIZ];
	int n;

	for (;;) 
	{
	    n = read (fdin, buf, sizeof(buf));
	    if (n == -1)
	    {
#ifdef EINTR
		if (errno == EINTR)
		    continue;
#endif
		error (1, errno, "cannot read file %s for copying", from);
	    }
            else if (n == 0) 
		break;
  
	    if (write(fdout, buf, n) != n) {
		error (1, errno, "cannot write file %s for copying", to);
	    }
	}

#ifdef HAVE_FSYNC
	if (fsync (fdout)) 
	    error (1, errno, "cannot fsync file %s after copying", to);
#endif
    }

    if (close (fdin) < 0) 
	error (0, errno, "cannot close %s", from);
    if (close (fdout) < 0)
	error (1, errno, "cannot close %s", to);

    /* now, set the times for the copied file to match those of the original */
    memset ((char *) &t, 0, sizeof (t));
    t.actime = sb.st_atime;
    t.modtime = sb.st_mtime;
    (void) utime (to, &t);
}

/* FIXME-krp: these functions would benefit from caching the char * &
   stat buf.  */

/*
 * Returns non-zero if the argument file is a directory, or is a symbolic
 * link which points to a directory.
 */
int
isdir (file)
    const char *file;
{
    struct stat sb;

    if (stat (file, &sb) < 0)
	return (0);
    return (S_ISDIR (sb.st_mode));
}

/*
 * Returns non-zero if the argument file is a symbolic link.
 */
int
islink (file)
    const char *file;
{
#ifdef S_ISLNK
    struct stat sb;

    if (lstat (file, &sb) < 0)
	return (0);
    return (S_ISLNK (sb.st_mode));
#else
    return (0);
#endif
}

/*
 * Returns non-zero if the argument file exists.
 */
int
isfile (file)
    const char *file;
{
    return isaccessible(file, F_OK);
}

/*
 * Returns non-zero if the argument file is readable.
 */
int
isreadable (file)
    const char *file;
{
    return isaccessible(file, R_OK);
}

/*
 * Returns non-zero if the argument file is writable.
 */
int
iswritable (file)
    const char *file;
{
    return isaccessible(file, W_OK);
}

/*
 * Returns non-zero if the argument file is accessable according to
 * mode.  If compiled with SETXID_SUPPORT also works if cvs has setxid
 * bits set.
 */
int
isaccessible (file, mode)
    const char *file;
    const int mode;
{
#ifdef SETXID_SUPPORT
    struct stat sb;
    int umask = 0;
    int gmask = 0;
    int omask = 0;
    int uid;
    
    if (stat(file, &sb) == -1)
	return 0;
    if (mode == F_OK)
	return 1;

    uid = geteuid();
    if (uid == 0)		/* superuser */
    {
	if (mode & X_OK)
	    return sb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
	else
	    return 1;
    }
	
    if (mode & R_OK)
    {
	umask |= S_IRUSR;
	gmask |= S_IRGRP;
	omask |= S_IROTH;
    }
    if (mode & W_OK)
    {
	umask |= S_IWUSR;
	gmask |= S_IWGRP;
	omask |= S_IWOTH;
    }
    if (mode & X_OK)
    {
	umask |= S_IXUSR;
	gmask |= S_IXGRP;
	omask |= S_IXOTH;
    }

    if (sb.st_uid == uid)
	return (sb.st_mode & umask) == umask;
    else if (sb.st_gid == getegid())
	return (sb.st_mode & gmask) == gmask;
    else
	return (sb.st_mode & omask) == omask;
#else
    return access(file, mode) == 0;
#endif
}

/*
 * Open a file and die if it fails
 */
FILE *
open_file (name, mode)
    const char *name;
    const char *mode;
{
    FILE *fp;

    if ((fp = fopen (name, mode)) == NULL)
	error (1, errno, "cannot open %s", name);
    return (fp);
}

/*
 * Make a directory and die if it fails
 */
void
make_directory (name)
    const char *name;
{
    struct stat sb;

    if (stat (name, &sb) == 0 && (!S_ISDIR (sb.st_mode)))
	    error (0, 0, "%s already exists but is not a directory", name);
    if (!noexec && mkdir (name, 0777) < 0)
	error (1, errno, "cannot make directory %s", name);
}

/*
 * Make a path to the argument directory, printing a message if something
 * goes wrong.
 */
void
make_directories (name)
    const char *name;
{
    char *cp;

    if (noexec)
	return;

    if (mkdir (name, 0777) == 0 || errno == EEXIST)
	return;
    if (! existence_error (errno))
    {
	error (0, errno, "cannot make path to %s", name);
	return;
    }
    if ((cp = strrchr (name, '/')) == NULL)
	return;
    *cp = '\0';
    make_directories (name);
    *cp++ = '/';
    if (*cp == '\0')
	return;
    (void) mkdir (name, 0777);
}

/* Create directory NAME if it does not already exist; fatal error for
   other errors.  Returns 0 if directory was created; 1 if it already
   existed.  */
int
mkdir_if_needed (name)
    char *name;
{
    if (mkdir (name, 0777) < 0)
    {
	if (errno != EEXIST)
	    error (1, errno, "cannot make directory %s", name);
	return 1;
    }
    return 0;
}

/*
 * Change the mode of a file, either adding write permissions, or removing
 * all write permissions.  Either change honors the current umask setting.
 */
void
xchmod (fname, writable)
    char *fname;
    int writable;
{
    struct stat sb;
    mode_t mode, oumask;

    if (stat (fname, &sb) < 0)
    {
	if (!noexec)
	    error (0, errno, "cannot stat %s", fname);
	return;
    }
    oumask = umask (0);
    (void) umask (oumask);
    if (writable)
    {
	mode = sb.st_mode | (~oumask
			     & (((sb.st_mode & S_IRUSR) ? S_IWUSR : 0)
				| ((sb.st_mode & S_IRGRP) ? S_IWGRP : 0)
				| ((sb.st_mode & S_IROTH) ? S_IWOTH : 0)));
    }
    else
    {
	mode = sb.st_mode & ~(S_IWRITE | S_IWGRP | S_IWOTH) & ~oumask;
    }

    if (trace)
#ifdef SERVER_SUPPORT
	(void) fprintf (stderr, "%c-> chmod(%s,%o)\n",
			(server_active) ? 'S' : ' ', fname,
			(unsigned int) mode);
#else
	(void) fprintf (stderr, "-> chmod(%s,%o)\n", fname,
			(unsigned int) mode);
#endif
    if (noexec)
	return;

    if (chmod (fname, mode) < 0)
	error (0, errno, "cannot change mode of file %s", fname);
}

/*
 * Rename a file and die if it fails
 */
void
rename_file (from, to)
    const char *from;
    const char *to;
{
    if (trace)
#ifdef SERVER_SUPPORT
	(void) fprintf (stderr, "%c-> rename(%s,%s)\n",
			(server_active) ? 'S' : ' ', from, to);
#else
	(void) fprintf (stderr, "-> rename(%s,%s)\n", from, to);
#endif
    if (noexec)
	return;

    if (rename (from, to) < 0)
	error (1, errno, "cannot rename file %s to %s", from, to);
}

/*
 * link a file, if possible.  Warning: the Windows NT version of this
 * function just copies the file, so only use this function in ways
 * that can deal with either a link or a copy.
 */
int
link_file (from, to)
    const char *from;
    const char *to;
{
    if (trace)
#ifdef SERVER_SUPPORT
	(void) fprintf (stderr, "%c-> link(%s,%s)\n",
			(server_active) ? 'S' : ' ', from, to);
#else
	(void) fprintf (stderr, "-> link(%s,%s)\n", from, to);
#endif
    if (noexec)
	return (0);

    return (link (from, to));
}

/*
 * unlink a file, if possible.
 */
int
unlink_file (f)
    const char *f;
{
    if (trace)
#ifdef SERVER_SUPPORT
	(void) fprintf (stderr, "%c-> unlink(%s)\n",
			(server_active) ? 'S' : ' ', f);
#else
	(void) fprintf (stderr, "-> unlink(%s)\n", f);
#endif
    if (noexec)
	return (0);

    return (unlink (f));
}

/*
 * Unlink a file or dir, if possible.  If it is a directory do a deep
 * removal of all of the files in the directory.  Return -1 on error
 * (in which case errno is set).
 */
int
unlink_file_dir (f)
    const char *f;
{
    if (trace)
#ifdef SERVER_SUPPORT
	(void) fprintf (stderr, "%c-> unlink_file_dir(%s)\n",
			(server_active) ? 'S' : ' ', f);
#else
	(void) fprintf (stderr, "-> unlink_file_dir(%s)\n", f);
#endif
    if (noexec)
	return (0);

    /* For at least some unices, if root tries to unlink() a directory,
       instead of doing something rational like returning EISDIR,
       the system will gleefully go ahead and corrupt the filesystem.
       So we first call isdir() to see if it is OK to call unlink().  This
       doesn't quite work--if someone creates a directory between the
       call to isdir() and the call to unlink(), we'll still corrupt
       the filesystem.  Where is the Unix Haters Handbook when you need
       it?  */
    if (isdir(f)) 
	return deep_remove_dir(f);
    else
    {
	if (unlink (f) != 0)
	    return -1;
    }
    /* We were able to remove the file from the disk */
    return 0;
}

/* Remove a directory and everything it contains.  Returns 0 for
 * success, -1 for failure (in which case errno is set).
 */

static int
deep_remove_dir (path)
    const char *path;
{
    DIR		  *dirp;
    struct dirent *dp;

    if (rmdir (path) != 0)
    {
	if (errno == ENOTEMPTY
	    || errno == EEXIST
	    /* Ugly workaround for ugly AIX 4.1 (and 3.2) header bug
	       (it defines ENOTEMPTY and EEXIST to 17 but actually
	       returns 87).  */
	    || (ENOTEMPTY == 17 && EEXIST == 17 && errno == 87))
	{
	    if ((dirp = opendir (path)) == NULL)
		/* If unable to open the directory return
		 * an error
		 */
		return -1;

	    while ((dp = readdir (dirp)) != NULL)
	    {
		char *buf;

		if (strcmp (dp->d_name, ".") == 0 ||
			    strcmp (dp->d_name, "..") == 0)
		    continue;

		buf = xmalloc (strlen (path) + strlen (dp->d_name) + 5);
		sprintf (buf, "%s/%s", path, dp->d_name);

		/* See comment in unlink_file_dir explanation of why we use
		   isdir instead of just calling unlink and checking the
		   status.  */
		if (isdir(buf)) 
		{
		    if (deep_remove_dir(buf))
		    {
			closedir(dirp);
			free (buf);
			return -1;
		    }
		}
		else
		{
		    if (unlink (buf) != 0)
		    {
			closedir(dirp);
			free (buf);
			return -1;
		    }
		}
		free (buf);
	    }
	    closedir (dirp);
	    return rmdir (path);
	}
	else
	    return -1;
    }

    /* Was able to remove the directory return 0 */
    return 0;
}

/* Read NCHARS bytes from descriptor FD into BUF.
   Return the number of characters successfully read.
   The number returned is always NCHARS unless end-of-file or error.  */
static size_t
block_read (fd, buf, nchars)
    int fd;
    char *buf;
    size_t nchars;
{
    char *bp = buf;
    size_t nread;

    do 
    {
	nread = read (fd, bp, nchars);
	if (nread == (size_t)-1)
	{
#ifdef EINTR
	    if (errno == EINTR)
		continue;
#endif
	    return (size_t)-1;
	}

	if (nread == 0)
	    break; 

	bp += nread;
	nchars -= nread;
    } while (nchars != 0);

    return bp - buf;
} 

    
/*
 * Compare "file1" to "file2". Return non-zero if they don't compare exactly.
 */
int
xcmp (file1, file2)
    const char *file1;
    const char *file2;
{
    char *buf1, *buf2;
    struct stat sb1, sb2;
    int fd1, fd2;
    int ret;

    if ((fd1 = open (file1, O_RDONLY)) < 0)
	error (1, errno, "cannot open file %s for comparing", file1);
    if ((fd2 = open (file2, O_RDONLY)) < 0)
	error (1, errno, "cannot open file %s for comparing", file2);
    if (fstat (fd1, &sb1) < 0)
	error (1, errno, "cannot fstat %s", file1);
    if (fstat (fd2, &sb2) < 0)
	error (1, errno, "cannot fstat %s", file2);

    /* A generic file compare routine might compare st_dev & st_ino here 
       to see if the two files being compared are actually the same file.
       But that won't happen in CVS, so we won't bother. */

    if (sb1.st_size != sb2.st_size)
	ret = 1;
    else if (sb1.st_size == 0)
	ret = 0;
    else
    {
	/* FIXME: compute the optimal buffer size by computing the least
	   common multiple of the files st_blocks field */
	size_t buf_size = 8 * 1024;
	size_t read1;
	size_t read2;

	buf1 = xmalloc (buf_size);
	buf2 = xmalloc (buf_size);

	do 
	{
	    read1 = block_read (fd1, buf1, buf_size);
	    if (read1 == (size_t)-1)
		error (1, errno, "cannot read file %s for comparing", file1);

	    read2 = block_read (fd2, buf2, buf_size);
	    if (read2 == (size_t)-1)
		error (1, errno, "cannot read file %s for comparing", file2);

	    /* assert (read1 == read2); */

	    ret = memcmp(buf1, buf2, read1);
	} while (ret == 0 && read1 == buf_size);

	free (buf1);
	free (buf2);
    }
	
    (void) close (fd1);
    (void) close (fd2);
    return (ret);
}

/* Just in case this implementation does not define this.  */
#ifndef L_tmpnam
#define	L_tmpnam 50
#endif

#ifdef LOSING_TMPNAM_FUNCTION
char *
cvs_temp_name ()
{
    char value[L_tmpnam + 1];

    /* FIXME: Should be using TMPDIR.  */
    strcpy (value, "/tmp/cvsXXXXXX");
    mktemp (value);
    return xstrdup (value);
}
#else
/* Generate a unique temporary filename.  Returns a pointer to a newly
   malloc'd string containing the name.  Returns successfully or not at
   all.  */
char *
cvs_temp_name ()
{
    char value[L_tmpnam + 1];
    char *retval;

    /* FIXME: should be using TMPDIR, perhaps by using tempnam on systems
       which have it.  */
    retval = tmpnam (value);
    if (retval == NULL)
	error (1, errno, "cannot generate temporary filename");
    return xstrdup (retval);
}
#endif

/* Return non-zero iff FILENAME is absolute.
   Trivial under Unix, but more complicated under other systems.  */
int
isabsolute (filename)
    const char *filename;
{
    return filename[0] == '/';
}


/* Return a pointer into PATH's last component.  */
char *
last_component (path)
    char *path;
{
    char *last = strrchr (path, '/');

    if (last)
        return last + 1;
    else
        return path;
}

/* Return the home directory.  Returns a pointer to storage
   managed by this function or its callees (currently getenv).
   This function will return the same thing every time it is
   called.  */
char *
get_homedir ()
{
    static char *home = NULL;
    char *env = getenv ("HOME");
    struct passwd *pw;

    if (home != NULL)
	return home;

    if (env)
	home = env;
    else if ((pw = (struct passwd *) getpwuid (getuid ()))
	     && pw->pw_dir)
	home = xstrdup (pw->pw_dir);
    else
	return 0;

    return home;
}

/* See cvs.h for description.  On unix this does nothing, because the
   shell expands the wildcards.  */
void
expand_wild (argc, argv, pargc, pargv)
    int argc;
    char **argv;
    int *pargc;
    char ***pargv;
{
    int i;
    *pargc = argc;
    *pargv = (char **) xmalloc (argc * sizeof (char *));
    for (i = 0; i < argc; ++i)
	(*pargv)[i] = xstrdup (argv[i]);
}

#ifdef SERVER_SUPPORT
/* Case-insensitive string compare.  I know that some systems
   have such a routine, but I'm not sure I see any reasons for
   dealing with the hair of figuring out whether they do (I haven't
   looked into whether this is a performance bottleneck; I would guess
   not).  */
int
cvs_casecmp (str1, str2)
    char *str1;
    char *str2;
{
    char *p;
    char *q;
    int pqdiff;

    p = str1;
    q = str2;
    while ((pqdiff = tolower (*p) - tolower (*q)) == 0)
    {
	if (*p == '\0')
	    return 0;
	++p;
	++q;
    }
    return pqdiff;
}

/* Case-insensitive file open.  As you can see, this is an expensive
   call.  We don't regard it as our main strategy for dealing with
   case-insensitivity.  Returns errno code or 0 for success.  Puts the
   new file in *FP.  NAME and MODE are as for fopen.  If PATHP is not
   NULL, then put a malloc'd string containing the pathname as found
   into *PATHP.  Note that a malloc'd string is put into *PATHP
   even if we return an error.  It doesn't mean anything, but it still
   must be freed.

   Might be cleaner to separate the file finding (which just gives
   *PATHP) from the file opening (which the caller can do).  For one
   thing, might make it easier to know whether to put NAME or *PATHP
   into error messages.  */
int
fopen_case (name, mode, fp, pathp)
    char *name;
    char *mode;
    FILE **fp;
    char **pathp;
{
    struct dirent *dp;
    DIR *dirp;
    char *dir;
    char *fname;
    char *found_name;
    int retval;

    /* Separate NAME into directory DIR and filename within the directory
       FNAME.  */
    dir = xstrdup (name);
    fname = strrchr (dir, '/');
    if (fname == NULL)
	error (1, 0, "internal error: relative pathname in fopen_case");
    *fname++ = '\0';

    found_name = NULL;
    dirp = CVS_OPENDIR (dir);
    if (dirp == NULL)
	error (1, errno, "cannot read directory %s", dir);
    errno = 0;
    while ((dp = readdir (dirp)) != NULL)
    {
	if (cvs_casecmp (dp->d_name, fname) == 0)
	{
	    if (found_name != NULL)
		error (1, 0, "%s is ambiguous; could mean %s or %s",
		       fname, dp->d_name, found_name);
	    found_name = xstrdup (dp->d_name);
	}
    }
    if (errno != 0)
	error (1, errno, "cannot read directory %s", dir);
    closedir (dirp);

    if (found_name == NULL)
    {
	*fp = NULL;
	retval = ENOENT;
    }
    else
    {
	char *p;

	/* Copy the found name back into DIR.  We are assuming that
	   found_name is the same length as fname, which is true as
	   long as the above code is just ignoring case and not other
	   aspects of filename syntax.  */
	p = dir + strlen (dir);
	*p++ = '/';
	strcpy (p, found_name);
	*fp = fopen (dir, mode);
	if (*fp == NULL)
	    retval = errno;
	else
	    retval = 0;
    }

    if (pathp == NULL)
	free (dir);
    else
	*pathp = dir;
    free (found_name);
    return retval;
}
#endif /* SERVER_SUPPORT */
