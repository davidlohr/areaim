
/****************************************************************
**                                                             **
**    Copyright (c) 1996 - 2001 Caldera International, Inc.    **
**                    All Rights Reserved.                     **
**                                                             **
** This program is free software; you can redistribute it      **
** and/or modify it under the terms of the GNU General Public  **
** License as published by the Free Software Foundation;       **
** either version 2 of the License, or (at your option) any    **
** later version.                                              **
**                                                             **
** This program is distributed in the hope that it will be     **
** useful, but WITHOUT ANY WARRANTY; without even the implied  **
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR     **
** PURPOSE. See the GNU General Public License for more        **
** details.                                                    **
**                                                             **
** You should have received a copy of the GNU General Public   **
** License along with this program; if not, write to the Free  **
** Software Foundation, Inc., 59 Temple Place, Suite 330,      **
** Boston, MA  02111-1307  USA                                 **
**                                                             **
****************************************************************/
#define _POSIX_SOURCE 1
#define _XOPEN_SOURCE 1		/* so O_SYNC can be used */
#define _INCLUDE_XOPEN_SOURCE 1	/* so O_SYNC can be used on HP */
#define _M_XOUT 1		/* so O_SYNC can be used on Acer/Altos */

#include <stdio.h>		/* enable scanf(), etc.  */
#include <stdlib.h>		/* enable rand(), etc. */
#include <unistd.h>		/* enable write(), etc. */
#include <sys/file.h>
#include <sys/types.h>		/* required for creat() */
#include <sys/stat.h>		/* required for creat() */
#include <fcntl.h>		/* required for creat() */
#include <string.h>		/* for strchr() */
#include <errno.h>

#include "suite.h"		/* our includes */

static int
sync_disk_rw(),
sync_disk_wrt(),
sync_disk_cp(),
sync_disk_update(),
disk_rr(), disk_rw(), disk_rd(), disk_wrt(), disk_cp(),
disk_brr(), disk_brw(), disk_brd(), disk_bwrt(), disk_bcp(),
disk_dio_rr(), disk_dio_rw(), disk_dio_rd(), disk_dio_wrt(), disk_dio_cp();

#ifdef MARCIA
/* get rid of this if don't find substitute then change
 * to ifndef hpux
 */
static int disk_aio_rr(), disk_aio_rw();
static int disk_aiodio_rr(), disk_aiodio_rw();
#endif

#ifdef UNMODIFIED_REAIM_7
#define	DO_SYNC()	system("sync");
#else
/*
 * The default behavior for AIM is to do "system("sync");" during disk
 * activity tests, which affects even workloads such as "workload.compute".
 * This behavior is unlikely to be representative of normal user behavior
 * and slows down the aforementioned compute workload by more than a factor
 * of 3.  Here we replace the sync with a macro that checks a global variable
 * set by command line option.  The default value of the variable no_syncs
 * is 0, so by default, the behavior is the same as normal AIM.  However,
 * by using the -y option, the user can remove the sync and get a more
 * representative workload.  Note that each system("sync") does a fork and
 * exec of the /bin/sync or /usr/sbin/sync executable, which is both
 * slow and unnecessary given that the sync() system call is not privileged.
 */
extern int no_syncs;
#define	DO_SYNC()	if(!no_syncs) system("sync");
#endif

void aim_mktemp();

#ifdef hpux
#define no_posix_memalign
#endif

/*
 * Kludgey hpux substitution for posix_memalign.
 *
 * This routine returns the an aligned buffer plus the "real" start of the 
 * buffer. The real start of the buffer is what is to be passed to free.
 * By returning the address that is to be passed
 * to free, we save ourselves the trouble of doing any bookkeeping.
 *
 * Algorithm:
 *	malloc size+(align-1) as much memory as requested. By mallocing
 *	more than is needed we are guaranteed to find a chunk
 *	within the buffer that meets the alignment and size requirement.
 *	Determine the starting aligned address within the buffer.
 */
int
local_pma(void **align_buf, size_t align, size_t size, void **buf_start)
{
#ifdef no_posix_memalign
	void *mp;

	/* align and size MUST be a power of two */
	mp = (void *)malloc (size + align-1);
	if (mp == NULL) {
		return ENOMEM; /* should return errno */
	}
	*buf_start = mp; /* the start of the buffer that is to be freed */
	*align_buf = (void *)(((ulong_t)mp + ((ulong_t)align-1)) & ~((ulong_t)align-1));
	return 0;
#else  /* posix_memalign */
	void *mp;
	int success;

	success = posix_memalign(align_buf, align, size);
	*buf_start = *align_buf;
	return success;
#endif /* posix_memalign */
}

source_file *disk1_c()
{
	static source_file s = { " @(#) disk1.c:1.20 2/10/94 13:51:47",	/* SCCS info */
		__FILE__, __DATE__, __TIME__
	};

	if (NBUFSIZE != 1024) {	/* enforce known block size */
		fprintf(stderr, "NBUFSIZE changed to %d\n", NBUFSIZE);
		exit(1);
	}

	register_test("disk_rr", 1, "DISKS", disk_rr,
		      WAITING_FOR_DISK_COUNT, "Random Disk Reads (K)");
	register_test("disk_rw", 1, "DISKS", disk_rw,
		      WAITING_FOR_DISK_COUNT, "Random Disk Writes (K)");
	register_test("disk_rd", 1, "DISKS", disk_rd,
		      WAITING_FOR_DISK_COUNT, "Sequential Disk Reads (K)");
	register_test("disk_wrt", 1, "DISKS", disk_wrt,
		      WAITING_FOR_DISK_COUNT,
		      "Sequential Disk Writes (K)");
	register_test("disk_cp", 1, "DISKS", disk_cp,
		      WAITING_FOR_DISK_COUNT, "Disk Copies (K)");

	register_test("sync_disk_rw", 1, "DISKS", sync_disk_rw,
		      WAITING_FOR_DISK_COUNT,
		      "Sync Random Disk Writes (K)");
	register_test("sync_disk_wrt", 1, "DISKS", sync_disk_wrt,
		      WAITING_FOR_DISK_COUNT,
		      "Sync Sequential Disk Writes (K)");
	register_test("sync_disk_cp", 1, "DISKS", sync_disk_cp,
		      WAITING_FOR_DISK_COUNT, "Sync Disk Copies (K)");
	register_test("sync_disk_update", 1, "DISKS", sync_disk_update,
		      WAITING_FOR_DISK_COUNT, "Sync Disk Updates (K)");
	register_test("disk_dio_rr", 1, "DISKS", disk_dio_rr,
		      WAITING_FOR_DISK_COUNT,
		      "Random O_DIRECT Disk Reads (K)");
	register_test("disk_dio_rw", 1, "DISKS", disk_dio_rw,
		      WAITING_FOR_DISK_COUNT,
		      "Random O_DIRECT Disk Writes (K)");
	register_test("disk_dio_rd", 1, "DISKS", disk_dio_rd,
		      WAITING_FOR_DISK_COUNT,
		      "Sequential O_DIRECT Disk Reads (K)");
	register_test("disk_dio_wrt", 1, "DISKS", disk_dio_wrt,
		      WAITING_FOR_DISK_COUNT,
		      "Sequential Disk Writes (K)");
	register_test("disk_dio_cp", 1, "DISKS", disk_dio_cp,
		      WAITING_FOR_DISK_COUNT, "O_DIRECT Disk Copies (K)");
	register_test("disk_brr", 1, "DISKS", disk_brr,
		      WAITING_FOR_DISK_COUNT,
		      "Random Bufcache Disk Reads (K)");
	register_test("disk_brw", 1, "DISKS", disk_brw,
		      WAITING_FOR_DISK_COUNT,
		      "Random Bufcache Disk Writes (K)");
	register_test("disk_brd", 1, "DISKS", disk_brd,
		      WAITING_FOR_DISK_COUNT,
		      "Sequential Bufcache Disk Reads (K)");
	register_test("disk_bwrt", 1, "DISKS", disk_bwrt,
		      WAITING_FOR_DISK_COUNT,
		      "Sequential Bufcache Disk Writes (K)");
	register_test("disk_bcp", 1, "DISKS", disk_bcp,
		      WAITING_FOR_DISK_COUNT, "Bufcache Disk Copies (K)");

#ifdef MARCIA
	register_test("disk_aio_rr", 1, "DISKS", disk_aio_rr,
		      WAITING_FOR_DISK_COUNT,
		      "Random ASYNC I/O Disk Reads (K)");
	register_test("disk_aio_rw", 1, "DISKS", disk_aio_rw,
		      WAITING_FOR_DISK_COUNT,
		      "Random ASYNC I/O Disk Writes (K)");
	register_test("disk_aiodio_rr", 1, "DISKS", disk_aiodio_rr,
		      WAITING_FOR_DISK_COUNT,
		      "Random ASYNC DIRECT I/O Disk Reads (K)");
	register_test("disk_aiodio_rw", 1, "DISKS", disk_aiodio_rw,
		      WAITING_FOR_DISK_COUNT,
		      "Random ASYNC DIRECT I/O Disk Writes (K)");
#endif /* MARCIA */

	return &s;
}


static char fn1[STRLEN];
static char fn2[STRLEN];

static char nbuf[NBUFSIZE];	/* 1K blocks */

/* test file size is 2^10 */
#define SHIFT	10		/* Change with block size */

/*
 * "Semi"-Random disk read					 
 * TVL 11/28/89
 */
static int disk_rr(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	long sk;
	char myfn2[STRLEN];

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */
	if (**argv)
		sprintf(myfn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(myfn2, "%s", TMPFILE2);
	aim_mktemp(myfn2);	/* generate new file name */

	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if ((fd = creat(myfn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
		fprintf(stderr, "disk_rr : cannot create %s\n", myfn2);
		perror(__FILE__);
		return (-1);
	}
	/*
	 * We do this to "encourage" the system to read from disk
	 * instead of the buffer cache.
	 * 12/12/89 TVL
	 */
	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_rr()");
			fprintf(stderr, "disk_rr : cannot write %s\n",
				myfn2);
			close(fd);
			unlink(myfn2);
			return (-1);
		}
	}
	close(fd);
	DO_SYNC();
	if ((fd = open(myfn2, O_RDONLY)) < 0) {
		fprintf(stderr, "disk_rr : cannot open %s\n", myfn2);
		perror(__FILE__);
		return (-1);
	}


  /********** pseudo random read *************/
	for (i = 0; i < disk_iteration_count; i++) {
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    ((disk_iteration_count *
		      (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */
		/*
		 * sk specifies a specific block, multiply by blocksize to get offset in bytes 
		 */
		sk <<= SHIFT;
		if (lseek(fd, sk, 0) == -1) {
			perror("disk_rr()");
			fprintf(stderr, "disk_rr : can't lseek %s\n",
				myfn2);
			close(fd);
			return (-1);
		}
		if ((n = read(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			perror("disk_rr()");
			fprintf(stderr, "disk_rr : can't read %s\n",
				myfn2);
			close(fd);
			return (-1);
		}
	}
	close(fd);
	unlink(myfn2);
	DO_SYNC();
	res->d = n;
	return (0);
}



/*
 * "Semi"-Random disk write
 */


static int disk_rw(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	char myfn2[STRLEN];

	long sk;

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(myfn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(myfn2, "%s", TMPFILE2);
	aim_mktemp(myfn2);

	if ((fd = creat(myfn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
		fprintf(stderr, "disk_rw : cannot create %s\n", myfn2);
		perror(__FILE__);
		return (-1);
	}

	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_rw()");
			fprintf(stderr, "disk_rw : cannot write %s\n",
				myfn2);
			close(fd);
			unlink(myfn2);
			return (-1);
		}
	}
	close(fd);
	DO_SYNC();
	if ((fd = open(myfn2, O_WRONLY)) < 0) {
		fprintf(stderr, "disk_rw : cannot open %s\n", myfn2);
		perror(__FILE__);
		return (-1);
	}


  /********** pseudo random write *************/

	for (i = 0; i < disk_iteration_count; i++) {
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    ((disk_iteration_count *
		      (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */
		/*
		 * sk specifies a specific block, multiply by blocksize to get offset in bytes 
		 */
		sk <<= SHIFT;
		if (lseek(fd, sk, 0) == -1) {
			perror("disk_rw()");
			fprintf(stderr, "disk_rw : can't lseek %s\n",
				myfn2);
			close(fd);
			return (-1);
		}
		if ((n = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			perror("disk_rw()");
			fprintf(stderr, "disk_rw : can't read %s\n",
				myfn2);
			close(fd);
			return (-1);
		}
	}
	unlink(myfn2);
	close(fd);
	DO_SYNC();
	res->d = n;
	return (0);
}

static int sync_disk_rw(int argc, char **argv, struct Result *res)
{
	int i, fd, blocks, n;

	long sk;

	sk = 0l;
	n = blocks = disk_iteration_count / 2;	/* divide by 2 so as not to pound disk */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(fn2, "%s", TMPFILE2);
	aim_mktemp(fn2);

	if ((fd = open(fn2, (O_WRONLY | O_CREAT | O_TRUNC),	/* standard CREAT mode */
		       (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {	/* standard permissions */
		fprintf(stderr, "sync_disk_rw : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}

	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_rw()");
			fprintf(stderr, "sync_disk_rw : cannot write %s\n",
				fn2);
			close(fd);
			unlink(fn2);
			return (-1);
		}
	}
	close(fd);
	if ((fd = open(fn2, O_WRONLY | O_SYNC)) < 0) {	/* open it again synchronously */
		fprintf(stderr, "sync_disk_rw : cannot open %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}

  /********** pseudo random write *************/

	for (i = 0; i < blocks; i++) {	/* get random block to read, 
					 * making sure not to read past end of file */
		sk = aim_rand() %
		    ((blocks * (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */

		sk <<= SHIFT;	/* sk specifies a specific block, 
				 * multiply by blocksize to get offset in bytes */
		if (lseek(fd, sk, 0) == -1) {
			perror("sync_disk_rw()");
			fprintf(stderr, "sync_disk_rw : can't lseek %s\n",
				fn2);
			unlink(fn2);
			close(fd);
			return (-1);
		}
		if ((n = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			perror("sync_disk_rw()");
			fprintf(stderr,
				"sync_disk_rw : can't write %d bytes to %s\n",
				sizeof nbuf, fn2);
			unlink(fn2);
			close(fd);
			return (-1);
		}
	}
	close(fd);
	unlink(fn2);
	res->d = n;
	return (0);
}

static int disk_rd(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	char myfn1[STRLEN];

	n = disk_iteration_count;
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(myfn1, "%s/%s", *argv, TMPFILE6);
	else
		sprintf(myfn1, "%s", TMPFILE6);

	aim_mktemp(myfn1);
	if ((fd = creat(myfn1, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
		fprintf(stderr, "disk_rd : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}
	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_rd()");
			fprintf(stderr, "disk_rd : cannot write %s\n",
				fn2);
			close(fd);
			unlink(fn2);
			return (-1);
		}
	}
	close(fd);
	DO_SYNC();
	fd = open(myfn1, O_RDONLY);
	if (fd < 0) {		/*  */
		fprintf(stderr, "disk_rd : cannot open %s\n", myfn1);
		perror(__FILE__);
		return (-1);
	}
	/*
	 * forward sequential read only 
	 */
	if (lseek(fd, 0L, 0) < 0) {
		fprintf(stderr, "disk_rd : can't lseek %s\n", myfn1);
		perror(__FILE__);
		close(fd);
		return (-1);
	}
	for (i = 0; i < disk_iteration_count; i++) {
		if (read(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			fprintf(stderr, "disk_rd : can't read %s\n",
				myfn1);
			perror(__FILE__);
			close(fd);
			return (-1);
		}
	}
	close(fd);
	DO_SYNC();
	res->d = i;
	unlink(myfn1);
	return (0);
}

static int disk_cp(int argc, char **argv, struct Result *res)
{
	int status,		/* result of last system call */
	 n, fd, fd2;

	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv) {		/* are we passing a directory? */
		sprintf(fn1, "%s/%s", *argv, TMPFILE1);	/* if so, build source file name */
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);	/* and destination file name */
	} else {		/* else build names in this directory */
		sprintf(fn1, "%s", TMPFILE1);	/* source file name */
		sprintf(fn2, "%s", TMPFILE2);	/* desination file nam */
	}
	DO_SYNC();
	aim_mktemp(fn2);	/* convert into unique temporary name */
	fd = open(fn1, O_RDONLY);	/* open the file */
	if (fd < 0) {		/* open source file */
		fprintf(stderr, "disk_cp (1): cannot open %s\n", fn1);	/* handle error */
		perror(__FILE__);	/* print error */
		return (-1);	/* return error */
	}
	fd2 = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO));	/* create the file */
	if (fd2 < 0) {		/* create output file */
		fprintf(stderr, "disk_cp (2): cannot create %s\n", fn2);	/* talk to human on error */
		perror(__FILE__);
		close(fd);	/* close source file */
		return (-1);	/* return error */
	}
	DO_SYNC();
	status = lseek(fd, 0L, SEEK_SET);	/* move pointer to offset 0 (rewind) */
	if (status < 0) {	/* handle error case */
		fprintf(stderr, "disk_cp (3): cannot lseek %s\n", fn1);	/* talk to human */
		perror(__FILE__);
		close(fd);	/* close source file */
		close(fd2);	/* close this file */
		return (-1);	/* return error */
	}
	while (n--) {		/* while not done */
		status = read(fd, nbuf, sizeof nbuf);	/* do the read */
		if (status != sizeof nbuf) {	/* return the status */
			fprintf(stderr,
				"disk_cp (4): cannot read %s %d (status = %d)\n",
				fn1, fd, status);
			perror(__FILE__);
			close(fd);
			close(fd2);
			return (-1);
		}
		status = write(fd2, nbuf, sizeof nbuf);	/* do the write */
		if (status != sizeof nbuf) {	/* check for error */
			fprintf(stderr, "disk_cp (5): cannot write %s\n",
				fn2);
			perror(__FILE__);
			close(fd);
			close(fd2);
			return (-1);
		}
	}
	unlink(fn2);
	/*
	 * make it anonymous (and work NFS harder) 
	 */
	close(fd);		/* close input file */
	close(fd2);		/* close (and delete) output file */
	unlink(fn2);
	DO_SYNC();
	res->d = disk_iteration_count;	/* return number */
	return (0);		/* show success */
}

static int sync_disk_cp(int argc, char **argv, struct Result *res)
{
	int status,		/* result of last system call */
	 n, blocks, fd, fd2;

	n = blocks = disk_iteration_count / 2;	/* divide by 2 so as not to pound disk */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv) {		/* are we passing a directory? */
		sprintf(fn1, "%s/%s", *argv, TMPFILE1);	/* if so, build source file name */
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);	/* and destination file name */
	} else {		/* else build names in this directory */
		sprintf(fn1, "%s", TMPFILE1);	/* source file name */
		sprintf(fn2, "%s", TMPFILE2);	/* desination file nam */
	}
	aim_mktemp(fn2);	/* convert into unique temporary name */
	fd = open(fn1, O_RDONLY);	/* open the file */
	if (fd < 0) {		/* open source file */
		fprintf(stderr, "sync_disk_cp (1): cannot open %s\n", fn1);	/* handle error */
		perror(__FILE__);	/* print error */
		return (-1);	/* return error */
	}
	fd2 = open(fn2, (O_SYNC | O_WRONLY | O_CREAT | O_TRUNC), (S_IRWXU | S_IRWXG | S_IRWXO));	/* create the file */
	if (fd2 < 0) {		/* create output file */
		fprintf(stderr, "sync_disk_cp (2): cannot create %s\n", fn2);	/* talk to human on error */
		perror(__FILE__);
		close(fd);	/* close source file */
		return (-1);	/* return error */
	}

	status = lseek(fd, 0L, SEEK_SET);	/* move pointer to offset 0 (rewind) */
	if (status < 0) {	/* handle error case */
		fprintf(stderr, "sync_disk_cp (3): cannot lseek %s\n", fn1);	/* talk to human */
		perror(__FILE__);
		close(fd);	/* close source file */
		close(fd2);	/* close this file */
		return (-1);	/* return error */
	}
	while (n--) {		/* while not done */
		status = read(fd, nbuf, sizeof nbuf);	/* do the read */
		if (status != sizeof nbuf) {	/* return the status */
			fprintf(stderr,
				"sync_disk_cp (4): cannot read %s %d (status = %d)\n",
				fn1, fd, status);
			perror(__FILE__);
			unlink(fn2);	/* make it anonymous (and work NFS harder) */
			close(fd);
			close(fd2);
			return (-1);
		}
		status = write(fd2, nbuf, sizeof nbuf);	/* do the write (SYNC) */
		if (status != sizeof nbuf) {	/* check for error */
			fprintf(stderr,
				"sync_disk_cp (5): cannot write %s\n",
				fn2);
			perror(__FILE__);
			unlink(fn2);	/* make it anonymous (and work NFS harder) */
			close(fd);
			close(fd2);
			return (-1);
		}
	}
	close(fd);		/* close input file */
	close(fd2);		/* close (and delete) output file */
	unlink(fn2);
	res->d = blocks;	/* return number */
	return (0);		/* show success */
}

static int disk_wrt(int argc, char **argv, struct Result *res)
{
	int n, fd, i;

	n = disk_iteration_count;	/* user specified size */
	i = 0;
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(fn2, "%s", TMPFILE2);
	aim_mktemp(fn2);
	fd = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO));
	if (fd < 0) {
		fprintf(stderr, "disk_wrt : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}

	DO_SYNC();

	while (n--) {
		if ((i = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			fprintf(stderr, "disk_wrt : cannot write %s\n",
				fn2);
			perror(__FILE__);
			unlink(fn2);
			close(fd);
			return (-1);
		}
	}

	unlink(fn2);		/*
				 * unlink moved after write 10/17/95  
				 */
	close(fd);
	DO_SYNC();

	res->d = disk_iteration_count;
	return (0);
}

static int sync_disk_wrt(int argc, char **argv, struct Result *res)
{
	int n, blocks, fd, i;

	n = blocks = disk_iteration_count / 2;	/* divide by 2 so as not to pound disk */
	i = 0;
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(fn2, "%s", TMPFILE2);
	aim_mktemp(fn2);
	fd = open(fn2, (O_SYNC | O_WRONLY | O_CREAT | O_TRUNC), (S_IRWXU | S_IRWXG | S_IRWXO));	/* sync creat */
	if (fd < 0) {
		fprintf(stderr, "sync_disk_wrt : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}


	while (n--) {
		if ((i = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			fprintf(stderr,
				"sync_disk_wrt : cannot write %s\n", fn2);
			perror(__FILE__);
			unlink(fn2);
			close(fd);
			return (-1);
		}
	}
	close(fd);
	res->d = blocks;
	unlink(fn2);
	return (0);
}

static int sync_disk_update(int argc, char **argv, struct Result *res)
{
	int i, fd, blocks, loop, n;

	long sk;

	sk = 0l;
	n = blocks = disk_iteration_count;
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(fn2, "%s", TMPFILE2);
	aim_mktemp(fn2);

	/*
	 * create a "database" file 
	 */
	if ((fd = open(fn2, (O_WRONLY | O_CREAT | O_TRUNC),	/* standard CREAT mode */
		       (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {	/* standard permissions */
		fprintf(stderr, "sync_disk_update : cannot create %s\n",
			fn2);
		perror(__FILE__);
		return (-1);
	}

	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_rw()");
			fprintf(stderr,
				"sync_disk_update : cannot write %s\n",
				fn2);
			close(fd);
			unlink(fn2);
			return (-1);
		}
	}
	close(fd);
	if ((fd = open(fn2, O_RDWR | O_SYNC)) < 0) {	/* open it again synchronously */
		fprintf(stderr, "sync_disk_update : cannot open %s\n",
			fn2);
		perror(__FILE__);
		return (-1);
	}


	/*
	 * pseudo random read then update 
	 */
	loop = blocks / 2;	/* only touch 1/2 the blocks */
	for (i = 0; i < loop; i++) {	/* get random block to read, 
					 * making sure not to read past end of file */
		sk = aim_rand() %
		    ((blocks * (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */

		sk <<= SHIFT;	/* sk specifies a specific block, 
				 * multiply by blocksize to get offset in bytes */
		if (lseek(fd, sk, 0) == -1) {	/* look something up */
			perror("sync_disk_update()");
			fprintf(stderr,
				"sync_disk_update : can't lseek %s\n",
				fn2);
			unlink(fn2);
			close(fd);
			return (-1);
		}
		if ((n = read(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {	/* read it in */
			perror("sync_disk_update()");
			fprintf(stderr,
				"sync_disk_update : can't read %d bytes to %s\n",
				sizeof nbuf, fn2);
			unlink(fn2);
			close(fd);
			return (-1);
		}
		if ((n = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {	/* update it */
			perror("sync_disk_update()");
			fprintf(stderr,
				"sync_disk_update : can't write %d bytes to %s\n",
				sizeof nbuf, fn2);
			unlink(fn2);
			close(fd);
			return (-1);
		}
	}
	close(fd);
	unlink(fn2);
	res->d = n;
	return (0);
}

/* 
 * replaces the contents of the string pointed to by template with a unique file name.
 * The string in template should look like a file name with six trailing Xs.
 * aim_mktemp() will replace the Xs with a character string that can be used to create a
 * unique file name.
 * aim_mktemp() is a substitute for unix "mktemp()", which is not POSIX compliant.
 * tmpnam() does not allow a directory to be specified.
 * tempnam() is not POSIX compliant.
 * changed from 3 pid char to 5 pid char  10/17/95  1.1.
 */
void aim_mktemp(char *template)
{
	static int counter = -1;	/* used to fill in template */
	static int pid_end;	/* holds last 5 digits of pid, constant for this process */
	char *Xs;		/* points to string of Xs in template */

	if ((template == NULL) || (*template == '\0')) {	/* make sure caller passes parameter */
		fprintf(stderr,
			"aim_mktemp : template parameter empty \n");
		return;
	}
	Xs = template + (strlen(template) - sizeof("XXXXXXXXXX")) + 1;	/* find the X's */
	if (strcmp(Xs, "XXXXXXXXXX") != 0) {	/* bad parameter */
		fprintf(stderr,
			"aim_mktemp : template parameter needs 10 Xs \n");
		return;
	}
	if (counter++ == -1) {	/* initialize counter and pid */
		pid_end = getpid() % 100000;	/* use uniqueness of pid, only need 5 digits */
	} else if (counter == 100000)	/* reset, only need 5 digits */
		counter = 0;

	sprintf(Xs, "%05d%05d", pid_end, counter);	/* write over XXXXXXXXXX, zero pad counter */
}

/*
 * "Semi"-Random disk read					 
 * TVL 11/28/89
 */
static int disk_dio_rr(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	long sk;
	char mfn2[STRLEN];
	char nybuf[1024];	/* 1K blocks */
	void *align_buf;	/* aligned buffer */
	void *buf_start;	/* address to pass to free */
	struct stat s;
	int do_unlink = 1;

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */

	if (**argv && stat(*argv, &s) == 0 && 
	    (S_ISCHR(s.st_mode)  || S_ISBLK(s.st_mode))) {
		/*
		 * Block or Char device.
		 */
		do_unlink = 0;	/* to skip unlink at end of test */
	} else {
		/*
		 * Old way -- create temporary file.
		 */
		
		if (**argv)
			sprintf(mfn2, "%s/%s", *argv, TMPFILE5);
		else
			sprintf(mfn2, "%s", TMPFILE5);
		aim_mktemp(mfn2);	/* generate new file name */

		if (argc != 1) {
			fprintf(stderr, "bad args\n");
		}
		if ((fd = creat(mfn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
			fprintf(stderr, "disk_dio_rr : cannot create %s\n", mfn2);
			perror(__FILE__);
			return (-1);
		}
		/*
		 * We do this to "encourage" the system to read from disk
		 * instead of the buffer cache.
		 * 12/12/89 TVL
		 */
		while (n--) {
			if (write(fd, nybuf, sizeof nybuf) != sizeof nybuf) {
				perror("disk_dio_rr()");
				fprintf(stderr, "disk_dio_rr : cannot write %s\n",
					mfn2);
				close(fd);
				unlink(mfn2);
				return (-1);
			}
		}
		close(fd);
	}
	DO_SYNC();
	if ((n = local_pma(&align_buf, 4096, sizeof nbuf, &buf_start))) {
		fprintf(stderr,
			"disk_dio_rr : can't allocated aligned memory %s\n",
			strerror(n));
		return (-1);
	}
	if ((fd = open(mfn2, O_DIRECT | O_RDONLY, 0666)) < 0) {
		fprintf(stderr, "disk_dio_rr : cannot open %s\n", mfn2);
		perror(__FILE__);
		free(buf_start);
		return (-1);
	}


  /********** pseudo random read *************/
	for (i = 0; i < disk_iteration_count; i++) {
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    (disk_iteration_count * (long) sizeof(nybuf));
		sk >>= SHIFT;	/* round to block size */
		sk <<= SHIFT;	/* block to byte offset */
		/* rand() % (filesize/blocksize) */
// JFB: re-address.  This would be automatic on HP-UX in unistd.h (which
// _is_ included by this file) if #define _LARGEFILE64_SOURCE were used.
// Should it be?  Are we really using 64-bit offsets?  Should 32-bit
// compiles support 64-bit offsets?  I think the preferred solution
// for both Linux and HP-UX is to put #define _LARGEFILE64_SOURCE before
// the include of unistd.h at the top of the file.  It's interesting that
// none of the other lseeks are lseek64---did someone run across a bug here?
#ifdef __LP64__
#define lseek64 lseek
#endif // __LP64__
		if (lseek64(fd, sk , SEEK_SET) == -1) {
			perror("disk_dio_rr()");
			fprintf(stderr, "disk_dio_rr : can't lseek %s\n",
				mfn2);
			close(fd);
			return (-1);
		}
		if ((n = read(fd, align_buf, sizeof nbuf)) != sizeof nbuf) {
			perror("disk_dio_rr()");
			fprintf(stderr,
			"disk_dio_rr : can't read %s at %ld got '%s' size %d\n",
				mfn2, sk, (char *)align_buf, n);
			close(fd);
			free(buf_start);
			return (-1);
		}
	}
	if (do_unlink)
		unlink(mfn2);
	close(fd);
	DO_SYNC();
	res->d = n;
	free(buf_start);
	return (0);
}



/*
 * "Semi"-Random disk write
 */


static int disk_dio_rw(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	void *align_buf;
	void *buf_start;	/* address to pass to free */
	struct stat s;
	int do_unlink = 1;

	long sk;

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}

	if (**argv && stat(*argv, &s) == 0 && 
	    (S_ISCHR(s.st_mode)  || S_ISBLK(s.st_mode))) {
		/*
		 * Block or Char device.
		 */
		do_unlink = 0;	/* to skip unlink at end of test */
	} else {
		/*
		 * Old way -- create temporary file.
		 */
		if (**argv)
			sprintf(fn2, "%s/%s", *argv, TMPFILE2);
		else
			sprintf(fn2, "%s", TMPFILE2);
		aim_mktemp(fn2);

		if ((fd = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
			fprintf(stderr, "disk_dio_rw : cannot create %s\n", fn2);
			perror(__FILE__);
			return (-1);
		}

		while (n--) {
			if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
				perror("disk_dio_rw()");
				fprintf(stderr, "disk_dio_rw : cannot write %s\n",
					fn2);
				close(fd);
				if (do_unlink)
					unlink(fn2);
				return (-1);
			}
		}
		close(fd);
	}
	DO_SYNC();
	if ((n = local_pma(&align_buf, 4096, sizeof nbuf, &buf_start))) {
		fprintf(stderr,
			"disk_dio_rw : can't allocated aligned memory %s\n",
			strerror(n));
		return (-1);
	}
	if ((fd = open(fn2, O_DIRECT | O_WRONLY)) < 0) {
		fprintf(stderr, "disk_dio_rw : cannot open %s\n", fn2);
		perror(__FILE__);
		if (do_unlink)
			unlink(fn2);
		free(buf_start);
		return (-1);
	}


  /********** pseudo random write *************/

	for (i = 0; i < disk_iteration_count; i++) {
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    ((disk_iteration_count *
		      (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */
		/*
		 * sk specifies a specific block, multiply by blocksize to get offset in bytes 
		 */
		sk <<= SHIFT;
		if (lseek(fd, sk, 0) == -1) {
			perror("disk_dio_rw()");
			fprintf(stderr, "disk_dio_rw : can't lseek %s\n",
				fn2);
			close(fd);
			if (do_unlink)
				unlink(fn2);
			free(buf_start);
			return (-1);
		}
		if ((n = write(fd, align_buf, sizeof nbuf)) != sizeof nbuf) {
			perror("disk_dio_rw()");
			fprintf(stderr, "disk_dio_rw : can't read %s\n",
				fn2);
			close(fd);
			if (do_unlink)
				unlink(fn2);
			free(buf_start);
			return (-1);
		}
	}
	if (do_unlink)
		unlink(fn2);
	close(fd);
	DO_SYNC();
	res->d = n;
	free(buf_start);
	return (0);
}


static int disk_dio_rd(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	n = disk_iteration_count;
	void *align_buf;	/* aligned buffer */
	void *buf_start;	/* address to pass to free */
	struct stat s;
	int do_unlink = 1;

	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv && stat(*argv, &s) == 0 && 
	    (S_ISCHR(s.st_mode)  || S_ISBLK(s.st_mode))) {
		/*
		 * Block or Char device.
		 */
		do_unlink = 0;	/* to skip unlink at end of test */
	} else {
		/*
		 * Old way -- use temporary file.
		 */
		if (**argv)
			sprintf(fn1, "%s/%s", *argv, TMPFILE1);
		else
			sprintf(fn1, "%s", TMPFILE1);
	}

	if ((n = local_pma(&align_buf, 4096, sizeof nbuf, &buf_start))) {
		fprintf(stderr,
			"disk_dio_rw : can't allocated aligned memory %s\n",
			strerror(n));
		return (-1);
	}
	fd = open(fn1, O_DIRECT | O_RDONLY);
	if (fd < 0) {		/*  */
		fprintf(stderr, "disk_dio_rd : cannot open %s\n", fn1);
		perror(__FILE__);
		return (-1);
	}
	/*
	 * forward sequential read only 
	 */
	if (lseek(fd, 0L, 0) < 0) {
		fprintf(stderr, "disk_dio_rd : can't lseek %s\n", fn1);
		perror(__FILE__);
		close(fd);
		return (-1);
	}
	for (i = 0; i < disk_iteration_count; i++) {
		if (read(fd, align_buf, sizeof nbuf) != sizeof nbuf) {
			fprintf(stderr, "disk_dio_rd : can't read %s\n",
				fn1);
			perror(__FILE__);
			close(fd);
			free(buf_start);
			return (-1);
		}
	}
	close(fd);
	DO_SYNC();
	res->d = i;
	free(buf_start);
	return (0);
}

static int disk_dio_cp(int argc, char **argv, struct Result *res)
{
	int status,		/* result of last system call */
	 n, fd, fd2;
	void *align_buf;	/* aligned buffer */
	void *buf_start;	/* address to pass to free */

	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv) {		/* are we passing a directory? */
		sprintf(fn1, "%s/%s", *argv, TMPFILE1);	/* if so, build source file name */
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);	/* and destination file name */
	} else {		/* else build names in this directory */
		sprintf(fn1, "%s", TMPFILE1);	/* source file name */
		sprintf(fn2, "%s", TMPFILE2);	/* desination file nam */
	}
	aim_mktemp(fn2);	/* convert into unique temporary name */
	fd = open(fn1, O_DIRECT | O_RDONLY);	/* open the file */
	if (fd < 0) {		/* open source file */
		fprintf(stderr, "disk_dio_cp (1): cannot open %s\n", fn1);	/* handle error */
		perror(__FILE__);	/* print error */
		return (-1);	/* return error */
	}
	fd2 = open(fn2, O_DIRECT | O_WRONLY | O_CREAT, (S_IRWXU | S_IRWXG | S_IRWXO));	/* create the file */
	if (fd2 < 0) {		/* create output file */
		fprintf(stderr, "disk_dio_cp (2): cannot create %s\n", fn2);	/* talk to human on error */
		perror(__FILE__);
		close(fd);	/* close source file */
		return (-1);	/* return error */
	}
	DO_SYNC();
	status = lseek(fd, 0L, SEEK_SET);	/* move pointer to offset 0 (rewind) */
	if (status < 0) {	/* handle error case */
		fprintf(stderr, "disk_dio_cp (3): cannot lseek %s\n", fn1);	/* talk to human */
		perror(__FILE__);
		close(fd);	/* close source file */
		close(fd2);	/* close this file */
		return (-1);	/* return error */
	}
	if ((n = local_pma(&align_buf, 4096, sizeof nbuf, &buf_start))) {
		fprintf(stderr,
			"disk_dio_rw : can't allocated aligned memory %s\n",
			strerror(n));
		return (-1);
	}
	while (n--) {		/* while not done */
		status = read(fd, align_buf, sizeof nbuf);/* do the read */
		if (status != sizeof nbuf) {	/* return the status */
			fprintf(stderr,
				"disk_dio_cp (4): cannot read %s %d (status = %d)\n",
				fn1, fd, status);
			perror(__FILE__);
			close(fd);
			close(fd2);
			free(buf_start);
			return (-1);
		}
		status = write(fd2, align_buf, sizeof nbuf);/* do the write */
		if (status != sizeof nbuf) {	/* check for error */
			fprintf(stderr,
				"disk_dio_cp (5): cannot write %s\n", fn2);
			perror(__FILE__);
			close(fd);
			close(fd2);
			free(buf_start);
			return (-1);
		}
	}

	unlink(fn2);
	/*
	 * make it anonymous (and work NFS harder) 
	 */
	close(fd);		/* close input file */
	close(fd2);		/* close (and delete) output file */
	DO_SYNC();
	res->d = disk_iteration_count;	/* return number */
	free(buf_start);
	return (0);		/* show success */
}

static int disk_dio_wrt(int argc, char **argv, struct Result *res)
{
	int n, fd, i;
	void *align_buf;	/* aligned buffer */
	void *buf_start;	/* address to pass to free */
	int do_unlink = 1;
	struct stat s;

	n = disk_iteration_count;	/* user specified size */
	i = 0;
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv && stat(*argv, &s) == 0 && 
	    (S_ISCHR(s.st_mode)  || S_ISBLK(s.st_mode))) {
		/*
		 * Block or Char device.
		 */
		do_unlink = 0;	/* to skip unlink at end of test */
		sprintf(fn2, "%s", *argv);
		fd = open(fn2, O_DIRECT | O_WRONLY);
	} else {
		/*
		 * Old way -- use temporary file.
		 */
		if (**argv)
			sprintf(fn2, "%s/%s", *argv, TMPFILE3);
		else
			sprintf(fn2, "%s", TMPFILE3);
		aim_mktemp(fn2);
		fd = open(fn2, O_DIRECT | O_WRONLY | O_CREAT,
			  (S_IRWXU | S_IRWXG | S_IRWXO));
	}
	if (fd < 0) {
		fprintf(stderr, "disk_dio_wrt : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}

	DO_SYNC();
	if ((n = local_pma(&align_buf, 4096, sizeof nbuf, &buf_start))) {
		fprintf(stderr,
			"disk_dio_wrt : can't allocated aligned memory %s\n",
			strerror(n));
		return (-1);
	}
	while (n--) {
		if ((i = write(fd, align_buf, sizeof nbuf)) != sizeof nbuf) {
			fprintf(stderr, "disk_dio_wrt : cannot write %s\n",
				fn2);
			perror(__FILE__);
			if (do_unlink)
				unlink(fn2);
			close(fd);
			free(buf_start);
			return (-1);
		}
	}

	close(fd);
	res->d = disk_iteration_count;
	if (do_unlink)
		unlink(fn2);
	DO_SYNC();
	free(buf_start);

	return (0);
}

static int disk_brr(int argc, char **argv, struct Result *res)
{
	int i, fd, n;
	long sk;

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE3);
	else
		sprintf(fn2, "%s", TMPFILE3);
	aim_mktemp(fn2);	/* generate new file name */

	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if ((fd = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
		fprintf(stderr, "disk_brr : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}
	/*
	 * We do this to "encourage" the system to read from disk
	 * instead of the buffer cache.
	 * 12/12/89 TVL
	 */
	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_brr()");
			fprintf(stderr, "disk_brr : cannot write %s\n",
				fn2);
			close(fd);
			unlink(fn2);
			return (-1);
		}
	}
	close(fd);
	if ((fd = open(fn2, O_RDONLY)) < 0) {
		fprintf(stderr, "disk_brr : cannot open %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}


  /********** pseudo random read *************/
	for (i = 0; i < disk_iteration_count; i++) {
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    ((disk_iteration_count *
		      (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */
		/*
		 * sk specifies a specific block, multiply by blocksize to get offset in bytes 
		 */
		sk <<= SHIFT;
		if (lseek(fd, sk, 0) == -1) {
			perror("disk_brr()");
			fprintf(stderr, "disk_brr : can't lseek %s\n",
				fn2);
			close(fd);
			return (-1);
		}
		if ((n = read(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			perror("disk_brr()");
			fprintf(stderr, "disk_brr : can't read %s\n", fn2);
			close(fd);
			return (-1);
		}
	}
	unlink(fn2);
	close(fd);
	res->d = n;
	return (0);
}



/*
 * "Semi"-Random disk write
 */


static int disk_brw(int argc, char **argv, struct Result *res)
{
	int i, fd, n;

	long sk;

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE3);
	else
		sprintf(fn2, "%s", TMPFILE3);
	aim_mktemp(fn2);

	if ((fd = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
		fprintf(stderr, "disk_brw : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}

	while (n--) {
		if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			perror("disk_brw()");
			fprintf(stderr, "disk_brw : cannot write %s\n",
				fn2);
			close(fd);
			unlink(fn2);
			return (-1);
		}
	}
	close(fd);
	if ((fd = open(fn2, O_WRONLY)) < 0) {
		fprintf(stderr, "disk_brw : cannot open %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}


  /********** pseudo random write *************/

	for (i = 0; i < disk_iteration_count; i++) {
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    ((disk_iteration_count *
		      (long) sizeof(nbuf)) >> SHIFT);
		/* rand() % (filesize/blocksize) */
		/*
		 * sk specifies a specific block, multiply by blocksize to get offset in bytes 
		 */
		sk <<= SHIFT;
		if (lseek(fd, sk, 0) == -1) {
			perror("disk_brw()");
			fprintf(stderr, "disk_brw : can't lseek %s\n",
				fn2);
			close(fd);
			return (-1);
		}
		if ((n = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			perror("disk_brw()");
			fprintf(stderr, "disk_brw : can't read %s\n", fn2);
			close(fd);
			return (-1);
		}
	}
	unlink(fn2);
	close(fd);
	res->d = n;
	return (0);
}

static int disk_brd(int argc, char **argv, struct Result *res)
{
	int i, fd;

	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn1, "%s/%s", *argv, TMPFILE1);
	else
		sprintf(fn1, "%s", TMPFILE1);
	fd = open(fn1, O_RDONLY);
	if (fd < 0) {		/*  */
		fprintf(stderr, "disk_brd : cannot open %s\n", fn1);
		perror(__FILE__);
		return (-1);
	}
	/*
	 * forward sequential read only 
	 */
	if (lseek(fd, 0L, 0) < 0) {
		fprintf(stderr, "disk_brd : can't lseek %s\n", fn1);
		perror(__FILE__);
		close(fd);
		return (-1);
	}
	for (i = 0; i < disk_iteration_count; i++) {
		if (read(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
			fprintf(stderr, "disk_brd : can't read %s\n", fn1);
			perror(__FILE__);
			close(fd);
			return (-1);
		}
	}
	close(fd);
	res->d = i;
	return (0);
}

static int disk_bcp(int argc, char **argv, struct Result *res)
{
	int status,		/* result of last system call */
	 n, fd, fd2;

	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv) {		/* are we passing a directory? */
		sprintf(fn1, "%s/%s", *argv, TMPFILE1);	/* if so, build source file name */
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);	/* and destination file name */
	} else {		/* else build names in this directory */
		sprintf(fn1, "%s", TMPFILE1);	/* source file name */
		sprintf(fn2, "%s", TMPFILE2);	/* desination file nam */
	}
	aim_mktemp(fn2);	/* convert into unique temporary name */
	fd = open(fn1, O_RDONLY);	/* open the file */
	if (fd < 0) {		/* open source file */
		fprintf(stderr, "disk_bcp (1): cannot open %s\n", fn1);	/* handle error */
		perror(__FILE__);	/* print error */
		return (-1);	/* return error */
	}
	fd2 = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO));	/* create the file */
	if (fd2 < 0) {		/* create output file */
		fprintf(stderr, "disk_bcp (2): cannot create %s\n", fn2);	/* talk to human on error */
		perror(__FILE__);
		close(fd);	/* close source file */
		return (-1);	/* return error */
	}
	status = lseek(fd, 0L, SEEK_SET);	/* move pointer to offset 0 (rewind) */
	if (status < 0) {	/* handle error case */
		fprintf(stderr, "disk_bcp (3): cannot lseek %s\n", fn1);	/* talk to human */
		perror(__FILE__);
		close(fd);	/* close source file */
		close(fd2);	/* close this file */
		return (-1);	/* return error */
	}
	while (n--) {		/* while not done */
		status = read(fd, nbuf, sizeof nbuf);	/* do the read */
		if (status != sizeof nbuf) {	/* return the status */
			fprintf(stderr,
				"disk_bcp (4): cannot read %s %d (status = %d)\n",
				fn1, fd, status);
			perror(__FILE__);
			close(fd);
			close(fd2);
			return (-1);
		}
		status = write(fd2, nbuf, sizeof nbuf);	/* do the write */
		if (status != sizeof nbuf) {	/* check for error */
			fprintf(stderr, "disk_bcp (5): cannot write %s\n",
				fn2);
			perror(__FILE__);
			close(fd);
			close(fd2);
			return (-1);
		}
	}

	unlink(fn2);
	/*
	 * make it anonymous (and work NFS harder) 
	 */
	close(fd);		/* close input file */
	close(fd2);		/* close (and delete) output file */
	unlink(fn2);
	res->d = disk_iteration_count;	/* return number */
	return (0);		/* show success */
}

static int disk_bwrt(int argc, char **argv, struct Result *res)
{
	int n, fd, i;

	n = disk_iteration_count;	/* user specified size */
	i = 0;
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv)
		sprintf(fn2, "%s/%s", *argv, TMPFILE2);
	else
		sprintf(fn2, "%s", TMPFILE2);
	aim_mktemp(fn2);
	fd = creat(fn2, (S_IRWXU | S_IRWXG | S_IRWXO));
	if (fd < 0) {
		fprintf(stderr, "disk_bwrt : cannot create %s\n", fn2);
		perror(__FILE__);
		return (-1);
	}

	while (n--) {
		if ((i = write(fd, nbuf, sizeof nbuf)) != sizeof nbuf) {
			fprintf(stderr, "disk_bwrt : cannot write %s\n",
				fn2);
			perror(__FILE__);
			unlink(fn2);
			close(fd);
			return (-1);
		}
	}

	unlink(fn2);		/*
				 * unlink moved after write 10/17/95  
				 */
	close(fd);
	res->d = disk_iteration_count;
	return (0);
}

/*
 * adding AIO support - these variable and functions are used
 *	by all the aio tests.
 */
#ifdef MARCIA
#include <libaio.h>

static int aio_blksize;
static int aio_maxio;		/* max num of aio's in flight */

struct iocb **iocb_free;	/* array of pointers to iocb */
int iocb_free_count;		/* current free count */
int aio_alignment = 512; 	/* buffer alignment */
int aio_inflight;
io_context_t myctx;

int init_aio_bufs(int n, int iosize)
{
	void *buf;
	int i;
	void *buf_start;	/* address to pass to free */

	if (iocb_free)		/* already init'ed */
		return 0;
	aio_blksize = iosize;	/* set the max block size */
	aio_maxio = n;		/* set the max number of i/o */
	if ((iocb_free = malloc(n * sizeof(struct iocb *))) == 0) {
		return -1;
	}

	for (i = 0; i < n; i++) {
		if (!(iocb_free[i] = (struct iocb *) malloc(sizeof(struct iocb))))
			return -1;
		if (local_pma(&buf, aio_alignment, iosize, &buf_start))
			return -1;
		io_prep_pread(iocb_free[i], -1, buf, iosize, 0);
	}
	iocb_free_count = i;
	return 0;
}

struct iocb *alloc_iocb(int size)
{
	if (size > aio_blksize) {
		fprintf(stderr,
			"Attempting to allocate a buffer too large %d > %d\n",
			size, aio_blksize);
	}
	if (!iocb_free_count)
		return 0;
	return iocb_free[--iocb_free_count];
}

void free_iocb(struct iocb *io)
{
	iocb_free[iocb_free_count++] = io;
}

/*
 * aio_wait_for_ios() - wait for an io_event and free it.
 * returns the number of aio that completed
 * return -1 if an io error occurred.
 */

int aio_wait_for_ios(io_context_t ctx, struct timespec *to, char *string)
{
	struct io_event events[aio_maxio];
	struct io_event *ep;
	int ret, n;

	/*
	 * get up to aio_maxio events at a time.
	 */
	ret = n = io_getevents(ctx, 1, aio_maxio, events, to);

	/*
	 * Check if we got any io errors and transferred the data.
	 */
	for (ep = events; n-- > 0; ep++) {
		struct iocb *iocb = ep->obj;

		if (ep->res2 != 0) {
			fprintf(stderr, "%s: aio error on\n", string);
			ret = -1;
		}

		if (ep->res != iocb->u.c.nbytes) {
			fprintf(stderr, "%s: aio short transfer on\n", string);
			ret = -1;
		}

		aio_inflight--;
		free_iocb(iocb);
	}
	return ret;
}


#define AIO_NUM_IO	32
#define AIO_MAX_BLKSIZE	8192

/*
 * "Semi"-Random async disk i/o					 
 */
static int disk_aio_read_write(int argc, char **argv, struct Result *res,
	int open_flag, char *name)
{
	int i, fd, n;
	long sk;
	char myfn2[STRLEN];
	struct stat s;
	int do_unlink = 1;
	int io_size;

	sk = 0l;
	n = disk_iteration_count;	/* user specified size */
	if (argc != 1) {
		fprintf(stderr, "bad args\n");
	}
	if (**argv && stat(*argv, &s) == 0 && 
	    (S_ISCHR(s.st_mode)  || S_ISBLK(s.st_mode))) {
		/*
		 * Block or Char device.
		 */
		do_unlink = 0;	/* to skip unlink at end of test */
		sprintf(fn2, "%s", *argv);
		fd = open(fn2, O_WRONLY);
	} else {
		/*
		 * Old way -- use temporary file.
		 */
		if (**argv)
			sprintf(myfn2, "%s/%s", *argv, TMPFILE2);
		else
			sprintf(myfn2, "%s", TMPFILE2);
		aim_mktemp(myfn2);	/* generate new file name */

		if ((fd = creat(myfn2, (S_IRWXU | S_IRWXG | S_IRWXO))) < 0) {
			fprintf(stderr, "%s : cannot create %s\n", name, myfn2);
			perror(__FILE__);
			return (-1);
		}
		/*
		 * We do this to "encourage" the system to read from disk
		 * instead of the buffer cache.
		 * 12/12/89 TVL
		 */
		while (n--) {
			if (write(fd, nbuf, sizeof nbuf) != sizeof nbuf) {
				fprintf(stderr, "%s : cannot write %s\n",
					name, myfn2);
				perror("");
				close(fd);
				unlink(myfn2);
				return (-1);
			}
		}
		close(fd);
	}
	DO_SYNC();
	if ((fd = open(myfn2, open_flag)) < 0) {
		fprintf(stderr, "%s : cannot open %s\n", name, myfn2);
		perror(__FILE__);
		return (-1);
	}

	/* initialize AIO */
        memset(&myctx, 0, sizeof(myctx));
        io_queue_init(AIO_NUM_IO, &myctx);

	/*
	 * Init the aio buffer we want to use.
	 */
	if (init_aio_bufs(AIO_NUM_IO, AIO_MAX_BLKSIZE) < 0) {
                fprintf(stderr, "%s: Error allocating the aio buffers\n", name);
		io_queue_release(myctx);
		return(-1);
	}

  /********** pseudo random read *************/
	io_size = sizeof nbuf;
	for (i = 0; i < disk_iteration_count; i++) {
		struct iocb *iocb;
		/*
		 * get random block to read, making sure not to read past end of file 
		 */
		sk = aim_rand() %
		    ((disk_iteration_count *
		      (long)io_size) >> SHIFT);
		/* rand() % (filesize/blocksize) */
		/*
		 * sk specifies a specific block, multiply by blocksize to get offset in bytes 
		 */
		sk <<= SHIFT;

		/*
		 * Check if we need to wait for aio, before more can be issued
		 */
		if (aio_inflight >= AIO_NUM_IO) {
			/*
			 * wait for at least 1 io
			 * aio_wait_for_ios() - decrements aio_inflight.
			 */
			if (aio_wait_for_ios(myctx, 0, name) < 0) {
				while (aio_inflight > 0) {
					(void)aio_wait_for_ios(myctx, 0,
								name);
				}
				io_queue_release(myctx);
				close(fd);
				return (-1);
			}
		}

		
		/*
		 * get a AIO iocb and initialize it.
		 */
		iocb = alloc_iocb(io_size);
		/*
		 * If we opened the file for write only, do writes
		 */
		if ((open_flag & O_ACCMODE) == O_WRONLY) {
			io_prep_pwrite(iocb, fd, iocb->u.c.buf, io_size, sk);
		} else {
			io_prep_pread(iocb, fd, iocb->u.c.buf, io_size, sk);
		}

		if ((n = io_submit(myctx, 1, &iocb)) < 0) {
			fprintf(stderr,
				"%s : io_submit() failed on i/o to %s : %s\n",
				name, myfn2, strerror(-n));
			io_queue_release(myctx);
			close(fd);
			return (-1);
		}
		aio_inflight++;
	}
	/*
	 * wait for outstanding i/o to complete.
	 */
	while (aio_inflight > 0) {
		(void)aio_wait_for_ios(myctx, 0, name);
	}
	io_queue_release(myctx);
	close(fd);
	if (do_unlink)
		unlink(myfn2);
	DO_SYNC();
	res->d = n;
	return (0);
}

/*
 * "Semi"-Random async disk read
 */
static int disk_aio_rr(int argc, char **argv, struct Result *res)
{
	return disk_aio_read_write(argc, argv, res, O_RDONLY, "disk_aio_rr");
}

/*
 * "Semi"-Random async disk write
 */
static int disk_aio_rw(int argc, char **argv, struct Result *res)
{
	return disk_aio_read_write(argc, argv, res, O_WRONLY, "disk_aio_rw");
}

/*
 * "Semi"-Random async direct disk read
 */
static int disk_aiodio_rr(int argc, char **argv, struct Result *res)
{
	return disk_aio_read_write(argc, argv, res, O_RDONLY|O_DIRECT, "disk_aiodio_rr");
}

/*
 * "Semi"-Random async direct disk write
 */
static int disk_aiodio_rw(int argc, char **argv, struct Result *res)
{
	return disk_aio_read_write(argc, argv, res, O_WRONLY|O_DIRECT, "disk_aiodio_rw");
}

#endif /* MARCIA - get rid of async io */

