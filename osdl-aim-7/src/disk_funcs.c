
/****************************************************************
**                                                             **
**    Copyright (c) 2003 Open Source Development Lab           **
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
/* disk_funcs.c - moved the disk load create/destroy functions to a 
 * separate file, so they can be replaced by something better 
 */

#include <stdio.h>		/* enable printf(), etc. */
#include <stdlib.h>		/* enable exit(), etc. */
#include <math.h>
#include <fcntl.h>
#ifndef hpux
#include <getopt.h>
#endif  /* hpux */
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>		/* for stat call */
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>

#include "suite.h"



void disk_unlink_all_test_files(int which_files)
{
	int j;			/* loop variable */
	char cmd[1024];
	int numdir = my_disk->numdirs;
	if (numdir > 0)		/* if using directories */
		for (j = 0; j < numdir; j++)	/* loop through them all */
			(void) unlink(my_disk->fn1arr[j]);	/* unlink each file */
	else			/* else no directories */
		(void) unlink(my_disk->fn1arr[0]);	/* so unlink the file */


	if (which_files == ALL_FILES) {
		if (numdir > 0)	/* if using directories */
			for (j = 0; j < numdir; j++) {	/* loop through them all */
				sprintf(cmd, "rm -fr %s", my_disk->fn2arr[j]);	/* remove fakeh directory */
				system(cmd);
				sprintf(cmd, "rm -f %s/link* 2> /dev/null", my_disk->dkarr[j]);	/* remove old link_test files */
				system(cmd);
		} else {	/* else no directories */
			system("rm -rf ./fakeh");	/* remove fakeh directory */
			system("rm -f ./link* 2> /dev/null");	/* remove link_test files */
		}
	}
}


 /* write out the work files that disk_rd and disk_cp use.  instead of having
  * each write out its own, we'll write out one copy ... in each directory
  * specified in the config file ... and let them all share it as input.  This
  * has two benefits, the first being that the test results are no longer
  * obfuscated with the disk write times, the second is that less overall
  * disk space is required
  * The disk files are filled with semi-random goop 
  * File names for tmpfiles are #defined in suite.h 
  */


/* global goop we rely on:
 * dkarr[] - list of disk directories from config file
 * fn1arr - names of diretories
 * fn2arr - names of files - used in create/destroy
 * inputs: 
 * numdir - number of disk directories available 
 * all this data is now contained in a struct, which is
 * located by following the global pointer my_disk
 */
int disk_create_all_files()
{
	int fd1, j, k;
	char cwd[256];		/* working directory for tar */
	char file_buffer[KILO];	/* tmp buffer for files */
	char fn1[STRLEN];
	/* tmp for naming */
	int numdir = my_disk->numdirs;

	if (getcwd(cwd, 256) == NULL) {
		fprintf(stderr,
			"disk_create_all_files(): can't get current working directory\n");
		return (-1);
	}
	/* fill with goop */
	for (j = 0; j < (int) sizeof(file_buffer); j++) {
		file_buffer[j] = ' ' + (j % 95);
	}

	if (verbose) {
		printf("Number of dirs is %d\n", numdir);
	}

	if (numdir > 0) {
		for (j = 0; j < numdir; j++) {
			(void) sprintf(fn1, "%s/%s", my_disk->dkarr[j],
				       TMPFILE1);
			fd1 = creat(fn1, S_IRWXU | S_IRWXG | S_IRWXO);
			/* error exit */
			if (fd1 < 0) {
				(void) fprintf(stderr,
					       "disk_create_all_files: cannot create %s\n",
					       fn1);
				return (-1);
			}
			/* funny names dept: disk_iteration_count is the number
			 * of 1K blocks we are creating. Same value is re-used inside
			 * disk_rr disk_rw, etc 
			 */
			k = disk_iteration_count;
			if (verbose) {
				printf("size in kbytes is %d\n", k);
			}

			while (k--) {
				if (write
				    (fd1, file_buffer,
				     sizeof(file_buffer)) !=
				    sizeof(file_buffer)) {
					(void) fprintf(stderr,
						       "disk_create_all_files: cannot create %s\n",
						       fn1);
					(void) close(fd1);
					return (-1);
				}
			}

			/* Save the generated file name for destruction */
			strcpy(my_disk->fn1arr[j], fn1);
			close(fd1);
		}
	} else {
		/* if no directories listed in the config file, die TODO - make this more graceful */
		fprintf(stderr,
			"No directories in config file, exitiing. \n");
		return 1;
	}
	system("sync");
	return (0);
}

/* The fakeh unpack function is only used by disk_src */
int create_fakeh()
{
	int fd1, j;
	struct stat stbuf;
	char buf[256];		/* file name */
	char dbuf[256];		/* string for location of fakeh.tgz */
	char cmd[1024];		/* string for system function -holds tar */
	char cwd[256];		/* working directory for tar */
	char file_buffer[KILO];	/* tmp buffer for files */
	char *default_fakeh = "/usr/local/share/reaim";
	char fn1[STRLEN];
	/* tmp for naming */
	int numdir = my_disk->numdirs;
	j = 0;

	if (getcwd(cwd, 256) == NULL) {
		fprintf(stderr,
			"create_fakeh(): can't get current working directory\n");
		return (-1);
	}

	if (numdir > 0) {
		/* First, check the current directory for fakeh.tgz 
	 	* then	 check the install default
	 	* then die
	 	*/
#ifdef hpux
		/* HPUX doesn't support the tar -z option.
		 * gzcat fakeh.tgz > fakeh.tar
		 */
		sprintf( dbuf, "%s/data/fakeh.tar", cwd );
		if ( stat( dbuf, &stbuf ) == -1 )  {
				sprintf( dbuf, "%s/fakeh.tar", default_fakeh );
				if ( stat( dbuf, &stbuf ) == -1 ) {
					fprintf(stderr, "create_fakeh(): can't find fakeh.tar\n");
					fprintf(stderr, "create_fakeh(): Default location is %s\n", default_fakeh);
					return (-1);
				}
		}
#else
		sprintf( dbuf, "%s/data/fakeh.tgz", cwd );
		if ( stat( dbuf, &stbuf ) == -1 )  {
				sprintf( dbuf, "%s/fakeh.tgz", default_fakeh );
				if ( stat( dbuf, &stbuf ) == -1 ) {
					fprintf(stderr, "create_fakeh(): can't find fakeh.tgz\n");
					fprintf(stderr, "create_fakeh(): Default location is %s\n", default_fakeh);
					return (-1);
				}
		}
#endif

		for (j = 0; j < numdir; j++) {
			sprintf(buf, "%s/fakeh", my_disk->dkarr[j]);
			(void) strcpy(my_disk->fn2arr[j], buf);
			if (stat(buf, &stbuf) < 0) {
#ifdef hpux
				sprintf(cmd,
					"cd %s; tar xf %s ",
					my_disk->dkarr[j], dbuf);
#else
				sprintf(cmd,
					"cd %s; tar xzf %s ",
					my_disk->dkarr[j], dbuf);
#endif
				system(cmd);
				if (stat(buf, &stbuf) < 0) {
					fprintf(stderr,
						"create_fakeh(): cannot create %s/fakeh\n",
						my_disk->dkarr[j]);
					return (-1);
				}
			}
		}
	}
	system("sync");
	return (0);

}
