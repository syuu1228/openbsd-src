/*	$OpenBSD: src/usr.bin/vim/Attic/memfile.c,v 1.3 1996/10/14 03:55:15 downsj Exp $	*/
/* vi:set ts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/* for debugging */
#define CHECK(c, s)	if (c) printf(s)

/*
 * memfile.c: Contains the functions for handling blocks of memory which can
 * be stored in a file. This is the implementation of a sort of virtual memory.
 *
 * A memfile consists of a sequence of blocks. The blocks numbered from 0
 * upwards have been assigned a place in the actual file. The block number
 * is equal to the page number in the file. The
 * blocks with negative numbers are currently in memory only. They can be
 * assigned a place in the file when too much memory is being used. At that
 * moment they get a new, positive, number. A list is used for translation of
 * negative to positive numbers.
 *
 * The size of a block is a multiple of a page size, normally the page size of
 * the device the file is on. Most blocks are 1 page long. A Block of multiple
 * pages is used for a line that does not fit in a single page.
 *
 * Each block can be in memory and/or in a file. The block stays in memory
 * as long as it is locked. If it is no longer locked it can be swapped out to
 * the file. It is only written to the file if it has been changed.
 *
 * Under normal operation the file is created when opening the memory file and
 * deleted when closing the memory file. Only with recovery an existing memory
 * file is opened.
 */

#if defined MSDOS  ||  defined WIN32
# include <io.h>		/* for lseek(), must be before vim.h */
#endif

#include "vim.h"
#include "globals.h"
#include "proto.h"
#include "option.h"
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

/* 
 * Some systems have the page size in statfs.f_bsize, some in stat.st_blksize
 */
#ifdef HAVE_ST_BLKSIZE
# define STATFS stat
# define F_BSIZE st_blksize
# define fstatfs(fd, buf, len, nul) fstat((fd), (buf))
#else
# ifdef HAVE_SYS_STATFS_H
#  include <sys/statfs.h>
#  define STATFS statfs
#  define F_BSIZE f_bsize
#  ifdef MINT				/* do we still need this? */
#   define fstatfs(fd, buf, len, nul) fstat((fd), (buf))
#  endif
# endif
#endif

/*
 * for Amiga Dos 2.0x we use Flush
 */
#ifdef AMIGA
# ifndef NO_ARP
extern int dos2;						/* this is in amiga.c */
# endif
# ifdef SASC
#  include <proto/dos.h>
#  include <ios1.h>						/* for chkufb() */
# endif
#endif

#define MEMFILE_PAGE_SIZE 4096			/* default page size */

static long total_mem_used = 0;			/* total memory used for memfiles */

static void mf_ins_hash __ARGS((MEMFILE *, BHDR *));
static void mf_rem_hash __ARGS((MEMFILE *, BHDR *));
static BHDR *mf_find_hash __ARGS((MEMFILE *, blocknr_t));
static void mf_ins_used __ARGS((MEMFILE *, BHDR *));
static void mf_rem_used __ARGS((MEMFILE *, BHDR *));
static BHDR *mf_release __ARGS((MEMFILE *, int));
static BHDR *mf_alloc_bhdr __ARGS((MEMFILE *, int));
static void mf_free_bhdr __ARGS((BHDR *));
static void mf_ins_free __ARGS((MEMFILE *, BHDR *));
static BHDR *mf_rem_free __ARGS((MEMFILE *));
static int	mf_read __ARGS((MEMFILE *, BHDR *));
static int	mf_write __ARGS((MEMFILE *, BHDR *));
static int	mf_trans_add __ARGS((MEMFILE *, BHDR *));
static void mf_do_open __ARGS((MEMFILE *, char_u *, int));

/*
 * The functions for using a memfile:
 *
 * mf_open()		open a new or existing memfile
 * mf_open_file()	open a swap file for an existing memfile
 * mf_close()		close (and delete) a memfile
 * mf_new()			create a new block in a memfile and lock it
 * mf_get()			get an existing block and lock it
 * mf_put()			unlock a block, may be marked for writing
 * mf_free()		remove a block
 * mf_sync()		sync changed parts of memfile to disk
 * mf_release_all()	release as much memory as possible
 * mf_trans_del()	may translate negative to positive block number
 * mf_fullname()	make file name full path (use before first :cd)
 */

/*
 * mf_open: open an existing or new memory block file
 *
 *	fname:		name of file to use (NULL means no file at all)
 *				Note: fname must have been allocated, it is not copied!
 *						If opening the file fails, fname is NOT freed.
 *  trunc_file:		if TRUE: file should be truncated when opening
 *
 *  If fname != NULL and file cannot be opened, fail.
 *
 * return value: identifier for this memory block file.
 */
	MEMFILE *
mf_open(fname, trunc_file)
	char_u	*fname;
	int		trunc_file;
{
	MEMFILE			*mfp;
	int				i;
	off_t			size;
#if defined(STATFS) && defined(UNIX) && !defined(__QNX__)
# define USE_FSTATFS
	struct STATFS 	stf;
#endif

	if ((mfp = (MEMFILE *)alloc((unsigned)sizeof(MEMFILE))) == NULL)
		return NULL;

	if (fname == NULL)		/* no file for this memfile, use memory only */
	{
		mfp->mf_fname = NULL;
		mfp->mf_xfname = NULL;
		mfp->mf_fd = -1;
	}
	else
	{
		mf_do_open(mfp, fname, trunc_file);		/* try to open the file */

		/* if the file cannot be opened, return here */
		if (mfp->mf_fd < 0)
		{
			vim_free(mfp);
			return NULL;
		}
	}

	mfp->mf_free_first = NULL;			/* free list is empty */
	mfp->mf_used_first = NULL;			/* used list is empty */
	mfp->mf_used_last = NULL;
	mfp->mf_dirty = FALSE;
	mfp->mf_used_count = 0;
	for (i = 0; i < MEMHASHSIZE; ++i)
	{
		mfp->mf_hash[i] = NULL;			/* hash lists are empty */
		mfp->mf_trans[i] = NULL;		/* trans lists are empty */
	}
	mfp->mf_page_size = MEMFILE_PAGE_SIZE;

#ifdef USE_FSTATFS
	/*
	 * Try to set the page size equal to the block size of the device.
	 * Speeds up I/O a lot.
	 * NOTE: minimal block size depends on size of block 0 data! It's not done
	 * with a sizeof(), because block 0 is defined in memline.c (Sorry).
	 * The maximal block size is arbitrary.
	 */
	if (mfp->mf_fd >= 0 &&
					fstatfs(mfp->mf_fd, &stf, sizeof(struct statfs), 0) == 0 &&
					stf.F_BSIZE >= 1048 && stf.F_BSIZE <= 50000)
		mfp->mf_page_size = stf.F_BSIZE;
#endif

	if (mfp->mf_fd < 0 || trunc_file ||
								(size = lseek(mfp->mf_fd, 0L, SEEK_END)) <= 0)
		mfp->mf_blocknr_max = 0;		/* no file or empty file */
	else
		mfp->mf_blocknr_max = size / mfp->mf_page_size;
	mfp->mf_blocknr_min = -1;
	mfp->mf_neg_count = 0;
	mfp->mf_infile_count = mfp->mf_blocknr_max;
	if (mfp->mf_fd < 0)
		mfp->mf_used_count_max = 0;			/* no limit */
	else
		mfp->mf_used_count_max = p_mm * 1024 / mfp->mf_page_size;

	return mfp;
}

/*
 * mf_open_file: open a file for an existing memfile. Used when updatecount
 *				 set from 0 to some value.
 *
 *	fname:		name of file to use (NULL means no file at all)
 *				Note: fname must have been allocated, it is not copied!
 *						If opening the file fails, fname is NOT freed.
 *
 * return value: FAIL if file could not be opened, OK otherwise
 */
	int
mf_open_file(mfp, fname)
	MEMFILE		*mfp;
	char_u		*fname;
{
	mf_do_open(mfp, fname, TRUE);				/* try to open the file */

	if (mfp->mf_fd < 0)
		return FAIL;

	mfp->mf_dirty = TRUE;
	return OK;
}

/*
 * close a memory file and delete the associated file if 'del_file' is TRUE
 */
	void
mf_close(mfp, del_file)
	MEMFILE	*mfp;
	int		del_file;
{
	BHDR		*hp, *nextp;
	NR_TRANS	*tp, *tpnext;
	int			i;

	if (mfp == NULL)				/* safety check */
		return;
	if (mfp->mf_fd >= 0)
	{
		if (close(mfp->mf_fd) < 0)
			EMSG("Close error on swap file");
	}
	if (del_file && mfp->mf_fname != NULL)
		vim_remove(mfp->mf_fname);
											/* free entries in used list */
	for (hp = mfp->mf_used_first; hp != NULL; hp = nextp)
	{
		total_mem_used -= hp->bh_page_count * mfp->mf_page_size;
		nextp = hp->bh_next;
		mf_free_bhdr(hp);
	}
	while (mfp->mf_free_first != NULL)		/* free entries in free list */
		vim_free(mf_rem_free(mfp));
	for (i = 0; i < MEMHASHSIZE; ++i)		/* free entries in trans lists */
		for (tp = mfp->mf_trans[i]; tp != NULL; tp = tpnext)
		{
			tpnext = tp->nt_next;
			vim_free(tp);
		}
	vim_free(mfp->mf_fname);
	vim_free(mfp->mf_xfname);
	vim_free(mfp);
}

/*
 * get a new block
 *
 *   negative: TRUE if negative block number desired (data block)
 */
	BHDR *
mf_new(mfp, negative, page_count)
	MEMFILE		*mfp;
	int			negative;
	int			page_count;
{
	BHDR	*hp;			/* new BHDR */
	BHDR	*freep;			/* first block in free list */
	char_u	*p;

	/*
	 * If we reached the maximum size for the used memory blocks, release one
	 * If a BHDR is returned, use it and adjust the page_count if necessary.
	 */
	hp = mf_release(mfp, page_count);

/*
 * Decide on the number to use:
 * If there is a free block, use its number.
 * Otherwise use mf_block_min for a negative number, mf_block_max for
 * a positive number.
 */
	freep = mfp->mf_free_first;
	if (!negative && freep != NULL && freep->bh_page_count >= page_count)
	{
		/*
		 * If the block in the free list has more pages, take only the number
		 * of pages needed and allocate a new BHDR with data
		 *
		 * If the number of pages matches and mf_release did not return a BHDR,
		 * use the BHDR from the free list and allocate the data
		 *
		 * If the number of pages matches and mf_release returned a BHDR,
		 * just use the number and free the BHDR from the free list
		 */
		if (freep->bh_page_count > page_count)
		{
			if (hp == NULL && (hp = mf_alloc_bhdr(mfp, page_count)) == NULL)
				return NULL;
			hp->bh_bnum = freep->bh_bnum;
			freep->bh_bnum += page_count;
			freep->bh_page_count -= page_count;
		}
		else if (hp == NULL)		/* need to allocate memory for this block */
		{
			if ((p = (char_u *)alloc(mfp->mf_page_size * page_count)) == NULL)
				return NULL;
			hp = mf_rem_free(mfp);
			hp->bh_data = p;
		}
		else				/* use the number, remove entry from free list */
		{
			freep = mf_rem_free(mfp);
			hp->bh_bnum = freep->bh_bnum;
			vim_free(freep);
		}
	}
	else		/* get a new number */
	{
		if (hp == NULL && (hp = mf_alloc_bhdr(mfp, page_count)) == NULL)
			return NULL;
		if (negative)
		{
			hp->bh_bnum = mfp->mf_blocknr_min--;
			mfp->mf_neg_count++;
		}
		else
		{
			hp->bh_bnum = mfp->mf_blocknr_max;
			mfp->mf_blocknr_max += page_count;
		}
	}
	hp->bh_flags = BH_LOCKED | BH_DIRTY;		/* new block is always dirty */
	mfp->mf_dirty = TRUE;
	hp->bh_page_count = page_count;
	mf_ins_used(mfp, hp);
	mf_ins_hash(mfp, hp);

	/*
	 * Init the data to all zero, to avoid reading uninitialized data.
	 * This also avoids that the passwd file ends up in the swap file!
	 */
	(void)vim_memset((char *)(hp->bh_data), 0, (size_t)mfp->mf_page_size);

	return hp;
}

/*
 * get existing block 'nr' with 'page_count' pages
 *
 * Note: The caller should first check a negative nr with mf_trans_del()
 */
	BHDR *
mf_get(mfp, nr, page_count)
	MEMFILE		*mfp;
	blocknr_t	nr;
	int			page_count;
{
	BHDR	*hp;
												/* doesn't exist */
	if (nr >= mfp->mf_blocknr_max || nr <= mfp->mf_blocknr_min)
		return NULL;

	/*
	 * see if it is in the cache
	 */
	hp = mf_find_hash(mfp, nr);
	if (hp == NULL)		/* not in the hash list */
	{
		if (nr < 0 || nr >= mfp->mf_infile_count)	/* can't be in the file */
			return NULL;

		/* could check here if the block is in the free list */

		/*
		 * Check if we need to flush an existing block.
		 * If so, use that block.
		 * If not, allocate a new block.
		 */
		hp = mf_release(mfp, page_count);
		if (hp == NULL && (hp = mf_alloc_bhdr(mfp, page_count)) == NULL)
			return NULL;

		hp->bh_bnum = nr;
		hp->bh_flags = 0;
		hp->bh_page_count = page_count;
		if (mf_read(mfp, hp) == FAIL)		/* cannot read the block! */
		{
			mf_free_bhdr(hp);
			return NULL;
		}
	}
	else
	{
		mf_rem_used(mfp, hp);	/* remove from list, insert in front below */
		mf_rem_hash(mfp, hp);
	}

	hp->bh_flags |= BH_LOCKED;
	mf_ins_used(mfp, hp);		/* put in front of used list */
	mf_ins_hash(mfp, hp);		/* put in front of hash list */

	return hp;
}

/*
 * release the block *hp
 *
 *   dirty: Block must be written to file later
 *	 infile: Block should be in file (needed for recovery)
 *
 *  no return value, function cannot fail
 */
	void
mf_put(mfp, hp, dirty, infile)
	MEMFILE	*mfp;
	BHDR	*hp;
	int		dirty;
	int		infile;
{
	int		flags;

	flags = hp->bh_flags;
	CHECK((flags & BH_LOCKED) == 0, "block was not locked");
	flags &= ~BH_LOCKED;
	if (dirty)
	{
		flags |= BH_DIRTY;
		mfp->mf_dirty = TRUE;
	}
	hp->bh_flags = flags;
	if (infile)
		mf_trans_add(mfp, hp);		/* may translate negative in positive nr */
}

/*
 * block *hp is no longer in used, may put it in the free list of memfile *mfp
 */
	void
mf_free(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	vim_free(hp->bh_data);		/* free the memory */
	mf_rem_hash(mfp, hp);		/* get *hp out of the hash list */
	mf_rem_used(mfp, hp);		/* get *hp out of the used list */
	if (hp->bh_bnum < 0)
	{
		vim_free(hp);			/* don't want negative numbers in free list */
		mfp->mf_neg_count--;
	}
	else
		mf_ins_free(mfp, hp);	/* put *hp in the free list */
}

/*
 * sync the memory file *mfp to disk
 *	if 'all' is FALSE blocks with negative numbers are not synced, even when
 *  they are dirty!
 *  if 'check_char' is TRUE, stop syncing when a character becomes available,
 *  but sync at least one block.
 *  if 'do_fsync' is TRUE make sure buffers are flushed to disk, so they will
 *  survive a system crash.
 *
 * Return FAIL for failure, OK otherwise
 */
	int
mf_sync(mfp, all, check_char, do_fsync)
	MEMFILE	*mfp;
	int		all;
	int		check_char;
	int		do_fsync;
{
	int		status;
	BHDR	*hp;
#ifdef SYNC_DUP_CLOSE
	int		fd;
#endif

	if (mfp->mf_fd < 0)		/* there is no file, nothing to do */
	{
		mfp->mf_dirty = FALSE;
		return FAIL;
	}

	/*
	 * sync from last to first (may reduce the probability of an inconsistent
	 * file) If a write fails, it is very likely caused by a full filesystem.
	 * Then we only try to write blocks within the existing file. If that also
	 * fails then we give up.
	 */
	status = OK;
	for (hp = mfp->mf_used_last; hp != NULL; hp = hp->bh_prev)
		if ((all || hp->bh_bnum >= 0) && (hp->bh_flags & BH_DIRTY) &&
					(status == OK || (hp->bh_bnum >= 0 &&
						hp->bh_bnum < mfp->mf_infile_count)))
		{
			if (mf_write(mfp, hp) == FAIL)
			{
				if (status == FAIL)		/* double error: quit syncing */
					break;
				status = FAIL;
			}
			if (check_char && mch_char_avail())	/* char available now */
				break;
		}
	
	/*
	 * If the whole list is flushed, the memfile is not dirty anymore.
	 * In case of an error this flag is also set, to avoid trying all the time.
	 */
	if (hp == NULL || status == FAIL)
		mfp->mf_dirty = FALSE;

	if (do_fsync && *p_sws != NUL)
	{
#if defined(UNIX)
# ifdef HAVE_FSYNC
		/*
		 * most Unixes have the very useful fsync() function, just what we need.
		 * However, with OS/2 and EMX it is also available, but there are
		 * reports of bad problems with it (a bug in HPFS.IFS).
		 * So we disable use of it here in case someone tries to be smart
		 * and changes conf_os2.h... (even though there is no __EMX__ test
		 * in the #if, as __EMX__ does not have sync(); we hope for a timely
		 * sync from the system itself).
		 */
#  if defined(__EMX__)
   error "Dont use fsync with EMX! Read emxdoc.doc or emxfix01.doc for info."
#  endif
		if (STRCMP(p_sws, "fsync") == 0)
		{
			if (fsync(mfp->mf_fd))
				status = FAIL;
		}
		else
# endif
			 sync();
#endif
#ifdef DJGPP
		if (_dos_commit(mfp->mf_fd))
			status = FAIL;
#else
# ifdef SYNC_DUP_CLOSE
		/*
		 * MSdos is a bit more work: Duplicate the file handle and close it.
		 * This should flush the file to disk.
		 */
		if ((fd = dup(mfp->mf_fd)) >= 0)
			close(fd);
# endif
#endif
#ifdef AMIGA
		/*
		 * Flush() only exists for AmigaDos 2.0.
		 * For 1.3 it should be done with close() + open(), but then the risk
		 * is that the open() may fail and lose the file....
		 */
# ifndef NO_ARP
		if (dos2)
# endif
# ifdef SASC
		{
			struct UFB *fp = chkufb(mfp->mf_fd);

			if (fp != NULL)
				Flush(fp->ufbfh);
		}
# else
#  ifdef _DCC
		{
			BPTR fh = (BPTR)fdtofh(mfp->mf_fd);

			if (fh != 0)
				Flush(fh);
			}
#  else /* assume Manx */
			Flush(_devtab[mfp->mf_fd].fd);
#  endif
# endif
#endif /* AMIGA */
	}

	return status;
}

/*
 * insert block *hp in front of hashlist of memfile *mfp
 */
	static void
mf_ins_hash(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	BHDR	*hhp;
	int		hash;

	hash = MEMHASH(hp->bh_bnum);
	hhp = mfp->mf_hash[hash];
	hp->bh_hash_next = hhp;
	hp->bh_hash_prev = NULL;
	if (hhp != NULL)
		hhp->bh_hash_prev = hp;
	mfp->mf_hash[hash] = hp;
}

/*
 * remove block *hp from hashlist of memfile list *mfp
 */
	static void
mf_rem_hash(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	if (hp->bh_hash_prev == NULL)
		mfp->mf_hash[MEMHASH(hp->bh_bnum)] = hp->bh_hash_next;
	else
		hp->bh_hash_prev->bh_hash_next = hp->bh_hash_next;
	
	if (hp->bh_hash_next)
		hp->bh_hash_next->bh_hash_prev = hp->bh_hash_prev;
}

/*
 * look in hash lists of memfile *mfp for block header with number 'nr'
 */
	static BHDR *
mf_find_hash(mfp, nr)
	MEMFILE		*mfp;
	blocknr_t	nr;
{
	BHDR		*hp;

	for (hp = mfp->mf_hash[MEMHASH(nr)]; hp != NULL; hp = hp->bh_hash_next)
		if (hp->bh_bnum == nr)
			break;
	return hp;
}

/*
 * insert block *hp in front of used list of memfile *mfp
 */
	static void
mf_ins_used(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	hp->bh_next = mfp->mf_used_first;
	mfp->mf_used_first = hp;
	hp->bh_prev = NULL;
	if (hp->bh_next == NULL)		/* list was empty, adjust last pointer */
		mfp->mf_used_last = hp;
	else
		hp->bh_next->bh_prev = hp;
	mfp->mf_used_count += hp->bh_page_count;
	total_mem_used += hp->bh_page_count * mfp->mf_page_size;
}

/*
 * remove block *hp from used list of memfile *mfp
 */
	static void
mf_rem_used(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	if (hp->bh_next == NULL)		/* last block in used list */
		mfp->mf_used_last = hp->bh_prev;
	else
		hp->bh_next->bh_prev = hp->bh_prev;
	if (hp->bh_prev == NULL)		/* first block in used list */
		mfp->mf_used_first = hp->bh_next;
	else
		hp->bh_prev->bh_next = hp->bh_next;
	mfp->mf_used_count -= hp->bh_page_count;
	total_mem_used -= hp->bh_page_count * mfp->mf_page_size;
}

/*
 * Release the least recently used block from the used list if the number
 * of used memory blocks gets to big.
 *
 * Return the block header to the caller, including the memory block, so
 * it can be re-used. Make sure the page_count is right.
 */
	static BHDR *
mf_release(mfp, page_count)
	MEMFILE		*mfp;
	int			page_count;
{
	BHDR		*hp;

		/*
		 * don't release a block if
		 *		there is no file for this memfile
		 * or
		 * 		there is no limit to the number of blocks for this memfile or
		 *		the maximum is not reached yet
		 *    and
		 *		total memory used is not up to 'maxmemtot'
		 */
	if (mfp->mf_fd < 0 || ((mfp->mf_used_count < mfp->mf_used_count_max ||
						mfp->mf_used_count_max == 0) &&
						(total_mem_used >> 10) < p_mmt))
		return NULL;

	for (hp = mfp->mf_used_last; hp != NULL; hp = hp->bh_prev)
		if (!(hp->bh_flags & BH_LOCKED))
			break;
	if (hp == NULL)		/* not a single one that can be released */
		return NULL;

		/*
		 * If the block is dirty, write it.
		 * If the write fails we don't free it.
		 */
	if ((hp->bh_flags & BH_DIRTY) && mf_write(mfp, hp) == FAIL)
		return NULL;

	mf_rem_used(mfp, hp);
	mf_rem_hash(mfp, hp);

/*
 * If a BHDR is returned, make sure that the page_count of bh_data is right
 */
	if (hp->bh_page_count != page_count)
	{
		vim_free(hp->bh_data);
		if ((hp->bh_data = alloc(mfp->mf_page_size * page_count)) == NULL)
		{
			vim_free(hp);
			return NULL;
		}
		hp->bh_page_count = page_count;
	}
	return hp;
}

/*
 * release as many blocks as possible
 * Used in case of out of memory
 *
 * return TRUE if any memory was released
 */
	int
mf_release_all()
{
	BUF			*buf;
	MEMFILE		*mfp;
	BHDR		*hp;
	int			retval = FALSE;

	for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	{
		mfp = buf->b_ml.ml_mfp;
		if (mfp != NULL && mfp->mf_fd >= 0)		/* only if there is a memfile with a file */
			for (hp = mfp->mf_used_last; hp != NULL; )
			{
				if (!(hp->bh_flags & BH_LOCKED) &&
						(!(hp->bh_flags & BH_DIRTY) || mf_write(mfp, hp) != FAIL))
				{
					mf_rem_used(mfp, hp);
					mf_rem_hash(mfp, hp);
					mf_free_bhdr(hp);
					hp = mfp->mf_used_last;		/* re-start, list was changed */
					retval = TRUE;
				}
				else
					hp = hp->bh_prev;
			}
	}
	return retval;
}

/*
 * Allocate a block header and a block of memory for it
 */
	static BHDR *
mf_alloc_bhdr(mfp, page_count)
	MEMFILE		*mfp;
	int			page_count;
{
	BHDR	*hp;

	if ((hp = (BHDR *)alloc((unsigned)sizeof(BHDR))) != NULL)
	{
		if ((hp->bh_data = (char_u *)alloc(mfp->mf_page_size * page_count))
																	  == NULL)
		{
			vim_free(hp);			/* not enough memory */
			return NULL;
		}
		hp->bh_page_count = page_count;
	}
	return hp;
}

/*
 * Free a block header and the block of memory for it
 */
	static void
mf_free_bhdr(hp)
	BHDR		*hp;
{
	vim_free(hp->bh_data);
	vim_free(hp);
}

/*
 * insert entry *hp in the free list
 */
	static void
mf_ins_free(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	hp->bh_next = mfp->mf_free_first;
	mfp->mf_free_first = hp;
}

/*
 * remove the first entry from the free list and return a pointer to it
 * Note: caller must check that mfp->mf_free_first is not NULL!
 */
	static BHDR *
mf_rem_free(mfp)
	MEMFILE	*mfp;
{
	BHDR	*hp;

	hp = mfp->mf_free_first;
	mfp->mf_free_first = hp->bh_next;
	return hp;
}

/*
 * read a block from disk
 * 
 * Return FAIL for failure, OK otherwise
 */
	static int
mf_read(mfp, hp)
	MEMFILE		*mfp;
	BHDR		*hp;
{
	off_t		offset;
	unsigned	page_size;
	unsigned	size;

	if (mfp->mf_fd < 0)		/* there is no file, can't read */
		return FAIL;

	page_size = mfp->mf_page_size;
	offset = page_size * hp->bh_bnum;
	size = page_size * hp->bh_page_count;
	if (lseek(mfp->mf_fd, offset, SEEK_SET) != offset)
	{
		EMSG("Seek error in swap file read");
		return FAIL;
	}
	if ((unsigned)read(mfp->mf_fd, (char *)hp->bh_data, (size_t)size) != size)
	{
		EMSG("Read error in swap file");
		return FAIL;
	}
	return OK;
}

/*
 * write a block to disk
 * 
 * Return FAIL for failure, OK otherwise
 */
	static int
mf_write(mfp, hp)
	MEMFILE		*mfp;
	BHDR		*hp;
{
	off_t		offset;		/* offset in the file */
	blocknr_t	nr;			/* block nr which is being written */
	BHDR		*hp2;
	unsigned	page_size;	/* number of bytes in a page */
	unsigned	page_count;	/* number of pages written */
	unsigned	size;		/* number of bytes written */

	if (mfp->mf_fd < 0)		/* there is no file, can't write */
		return FAIL;

	if (hp->bh_bnum < 0)		/* must assign file block number */
		if (mf_trans_add(mfp, hp) == FAIL)
			return FAIL;

	page_size = mfp->mf_page_size;

	/*
	 * We don't want gaps in the file. Write the blocks in front of *hp
	 * to extend the file.
	 * If block 'mf_infile_count' is not in the hash list, it has been
	 * freed. Fill the space in the file with data from the current block.
	 */
	for (;;)
	{
		nr = hp->bh_bnum;
		if (nr > mfp->mf_infile_count)			/* beyond end of file */
		{
			nr = mfp->mf_infile_count;
			hp2 = mf_find_hash(mfp, nr);		/* NULL catched below */
		}
		else
			hp2 = hp;

		offset = page_size * nr;
		if (lseek(mfp->mf_fd, offset, SEEK_SET) != offset)
		{
			EMSG("Seek error in swap file write");
			return FAIL;
		}
		if (hp2 == NULL)			/* freed block, fill with dummy data */
			page_count = 1;
		else
			page_count = hp2->bh_page_count;
		size = page_size * page_count;
		if ((unsigned)write(mfp->mf_fd,
			 (char *)(hp2 == NULL ? hp : hp2)->bh_data, (size_t)size) != size)
		{
			/*
			 * Avoid repeating the error message, this mostly happens when the
			 * disk is full. We give the message again only after a succesful
			 * write or when hitting a key. We keep on trying, in case some
			 * space becomes available.
			 */
			if (!did_swapwrite_msg)
				EMSG("Write error in swap file");
			did_swapwrite_msg = TRUE;
			return FAIL;
		}
		did_swapwrite_msg = FALSE;
		if (hp2 != NULL)					/* written a non-dummy block */
			hp2->bh_flags &= ~BH_DIRTY;
											/* appended to the file */
		if (nr + (blocknr_t)page_count > mfp->mf_infile_count)
			mfp->mf_infile_count = nr + page_count;
		if (nr == hp->bh_bnum)				/* written the desired block */
			break;
	}
	return OK;
}

/*
 * Make block number for *hp positive and add it to the translation list
 * 
 * Return FAIL for failure, OK otherwise
 */
	static int
mf_trans_add(mfp, hp)
	MEMFILE	*mfp;
	BHDR	*hp;
{
	BHDR		*freep;
	blocknr_t	new_bnum;
	int			hash;
	NR_TRANS	*np;
	int			page_count;

	if (hp->bh_bnum >= 0)					/* it's already positive */
		return OK;

	if ((np = (NR_TRANS *)alloc((unsigned)sizeof(NR_TRANS))) == NULL)
		return FAIL;

/*
 * get a new number for the block.
 * If the first item in the free list has sufficient pages, use its number
 * Otherwise use mf_blocknr_max.
 */
	freep = mfp->mf_free_first;
	page_count = hp->bh_page_count;
	if (freep != NULL && freep->bh_page_count >= page_count)
	{
		new_bnum = freep->bh_bnum;
		/*
		 * If the page count of the free block was larger, recude it.
		 * If the page count matches, remove the block from the free list
		 */
		if (freep->bh_page_count > page_count)
		{
			freep->bh_bnum += page_count;
			freep->bh_page_count -= page_count;
		}
		else
		{
			freep = mf_rem_free(mfp);
			vim_free(freep);
		}
	}
	else
	{
		new_bnum = mfp->mf_blocknr_max;
		mfp->mf_blocknr_max += page_count;
	}

	np->nt_old_bnum = hp->bh_bnum;			/* adjust number */
	np->nt_new_bnum = new_bnum;

	mf_rem_hash(mfp, hp);					/* remove from old hash list */
	hp->bh_bnum = new_bnum;
	mf_ins_hash(mfp, hp);					/* insert in new hash list */

	hash = MEMHASH(np->nt_old_bnum);		/* insert in trans list */
	np->nt_next = mfp->mf_trans[hash];
	mfp->mf_trans[hash] = np;
	if (np->nt_next != NULL)
		np->nt_next->nt_prev = np;
	np->nt_prev = NULL;	

	return OK;
}

/*
 * Lookup a tranlation from the trans lists and delete the entry
 * 
 * Return the positive new number when found, the old number when not found
 */
 	blocknr_t
mf_trans_del(mfp, old_nr)
	MEMFILE		*mfp;
	blocknr_t	old_nr;
{
	int			hash;
	NR_TRANS	*np;
	blocknr_t	new_bnum;

	hash = MEMHASH(old_nr);
	for (np = mfp->mf_trans[hash]; np != NULL; np = np->nt_next)
		if (np->nt_old_bnum == old_nr)
			break;
	if (np == NULL)				/* not found */
		return old_nr;

	mfp->mf_neg_count--;
	new_bnum = np->nt_new_bnum;
	if (np->nt_prev != NULL)			/* remove entry from the trans list */
		np->nt_prev->nt_next = np->nt_next;
	else
		mfp->mf_trans[hash] = np->nt_next;
	if (np->nt_next != NULL)
		np->nt_next->nt_prev = np->nt_prev;
	vim_free(np);

	return new_bnum;
}

/*
 * Set mfp->mf_xfname according to mfp->mf_fname and some other things.
 * Don't get the full path name if did_cd is TRUE, then fname should
 * already be a full path name.
 */
	void
mf_set_xfname(mfp)
	MEMFILE		*mfp;
{
	mfp->mf_xfname = NULL;
	if (!did_cd)
		mfp->mf_xfname = FullName_save(mfp->mf_fname);
}

/*
 * Make the name of the file used for the memfile a full path.
 * Used before doing a :cd
 */
	void
mf_fullname(mfp)
	MEMFILE		*mfp;
{
	if (mfp != NULL && mfp->mf_fname != NULL && mfp->mf_xfname != NULL)
	{
		vim_free(mfp->mf_fname);
		mfp->mf_fname = mfp->mf_xfname;
		mfp->mf_xfname = NULL;
	}
}

/*
 * return TRUE if there are any translations pending for 'mfp'
 */
	int
mf_need_trans(mfp)
	MEMFILE		*mfp;
{
	return (mfp->mf_fname != NULL && mfp->mf_neg_count > 0);
}

#if 1			/* included for beta release, TODO: remove later */
/*
 * print statistics for a memfile (for debugging)
 */
	void
mf_statistics()
{
	MEMFILE		*mfp;
	BHDR		*hp;
	int			used = 0;
	int			locked = 0;
	int			dirty = 0;
	int			nfree = 0;
	int			negative = 0;

	mfp = curbuf->b_ml.ml_mfp;
	if (mfp == NULL)
		MSG("No memfile");
	else
	{
		for (hp = mfp->mf_used_first; hp != NULL; hp = hp->bh_next)
		{
			++used;
			if (hp->bh_flags & BH_LOCKED)
				++locked;
			if (hp->bh_flags & BH_DIRTY)
				++dirty;
			if (hp->bh_bnum < 0)
				++negative;
		}
		for (hp = mfp->mf_free_first; hp != NULL; hp = hp->bh_next)
			++nfree;
		sprintf((char *)IObuff, "%d used (%d locked, %d dirty, %d (%d) negative), %d free",
						used, locked, dirty, negative, (int)mfp->mf_neg_count, nfree);
		msg(IObuff);
		sprintf((char *)IObuff, "Total mem used is %ld bytes", total_mem_used);
		msg(IObuff);
	}
}
#endif

/*
 * open a swap file for a memfile
 */
	static void
mf_do_open(mfp, fname, trunc_file)
	MEMFILE		*mfp;
	char_u		*fname;
	int			trunc_file;
{
	mfp->mf_fname = fname;
	/*
	 * Get the full path name before the open, because this is
	 * not possible after the open on the Amiga.
	 * fname cannot be NameBuff, because it must have been allocated.
	 */
	mf_set_xfname(mfp);

	/*
	 * try to open the file
	 */
	mfp->mf_fd = open((char *)fname,
			(trunc_file ? (O_CREAT | O_RDWR | O_TRUNC) : (O_RDONLY)) | O_EXTRA

#ifdef AMIGA				/* Amiga has no mode argument */
					);
#endif
#ifdef UNIX					/* open in rw------- mode */
					, (mode_t)0600);
#endif
#if defined(MSDOS) || defined(WIN32) || defined(__EMX__)
					, S_IREAD | S_IWRITE);         /* open read/write */
#endif
#ifdef VMS					/* open in rw------- mode */
					, 0600);
#endif

	/*
	 * If the file cannot be opened, use memory only
	 */
	if (mfp->mf_fd < 0)
	{
		vim_free(mfp->mf_xfname);
		mfp->mf_fname = NULL;
		mfp->mf_xfname = NULL;
	}
}
