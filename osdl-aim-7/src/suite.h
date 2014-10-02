#ifndef SUITE_H
#define SUITE_H
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
#define suite_h " @(#) suite.h:1.9 1/22/96 00:00:00"

/*
 ** suite.h
 **
 **	Common include file for the benchmark.
 **
 */
#define TRUE (1)		/* define these for everyone */
#define FALSE (0)
/* This is an old debug function, soon to be removed
 * if you wish see exactly how many times each sub loop runs,
 * #define COUNT 1
 * otherwise, fugeddaboutit  */

#ifdef COUNT
#define COUNT_START static int aim_iteration_test_count = 0, caim_iteration_test_count = 0;
#define COUNT_ZERO aim_iteration_test_count = 0; caim_iteration_test_count = 0
#define COUNT_BUMP  { aim_iteration_test_count++; }
#define COUNT_END(a) if (caim_iteration_test_count++ == 0) printf("Count = %d for test %s in file %s at line %d\n", aim_iteration_test_count, a, __FILE__, __LINE__);
#else
#define COUNT_START
#define COUNT_BUMP
#define COUNT_ZERO
#define COUNT_END(a)
#endif

#define WORKLD		100
#define MAX_WORK	1024
#define SCRIPTLOCATION  "/usr/local/share/reaim/osdl-aim-7/scripts"
#define WORKFILE	"/usr/local/share/reaim/workfile.short"
#define CONFIGFILE	"/usr/local/share/reaim/reaim.config"
#define LOGFILEPREFIX	"reaim"
#define STROKES		50	/* baud rate per user typing */
#define MAXITR		10	/* max number of iteration */
#define MAXDRIVES	255	/* max number of HD drives to test */
#define LIST		100	/* results array size */
#define LOADNUM		100	/* number of procs to do per user */
#define CONTINUE	1
#define STRLEN		80
#define Members(x)	(sizeof(x)/sizeof(x[0]))	/* number items in array */
#define JUST_TEMP_FILES	0
#define ALL_FILES 	1
#define THRESHOLD_MIN   1.20
#define THRESHOLD_MAX   1.30

/*
 * NBUFSIZE is the smallest possible test file size (1k==1024 bytes).
 * The filesize is NBUFSIZE * disk_iteration_count (normally 1 megabyte).
 * disk_iteration_count is set by the user at the input prompt
 */
#define NBUFSIZE        (2*512)	/* size of disk read/write buffer */
#define KILO		1024	/* multiplier for config file */

void killall();			/* for signal handling */
void letgo();			/*  */
void dead_kid();		/* for signal handling */
void math_err();		/*  */
int vmctl();			/* rec */
int ttyctl();			/* rec */
int tp_ctl();			/* rec */
void foo();			/* rec */

/*----------------------------------------------
 ** Declared GLOBALs for use in other files
 **----------------------------------------------
 */
extern int *p_i1;
extern long *p_fcount;
extern long debug;

struct Result {
	char c;
	short s;
	int i;
	long l;
	float f;
	double d;
};

double next_value();		/* next value from floating point array */
long rtmsec(int);		/* return real time in ms */
long child_ticks();		/* return time in child from times() */

unsigned int aim_rand(), aim_rand2();

void aim_srand(), aim_srand2();

#define WAITING_FOR_DISK_COUNT -99	/* marker value */
void register_test(char *name,	/* register test to be run */
		   int argc,	/* added to match ltp */
		   char *args,	/* name, args */
		   int (*f) (),	/* pointer to function */
		   int factor,	/* factor (or -1 for uncalibrated) */
		   char *units);	/* units for factor */

struct Cargs {
	char *name;
	char *args;
	int (*f) ();
	int factor;		/* 5/27/93 -- REC used for Suite IX's operation count */
	char *units;		/* 5/28/93 -- REC used for Suite IX's operation count */
	int argc;		/* arg count added to make LTP test useable */
	int run;
};


/*
 ** Defines for disk tests
 */
#define TMPFILE1 "tmpa.common"
#define TMPFILE2 "tmpb.XXXXXXXXXX"
#define TMPFILE3 "tmpc.XXXXXXXXXX"
#define TMPFILE4 "tmpd.XXXXXXXXXX"
#define TMPFILE5 "tmpe.XXXXXXXXXX"
#define TMPFILE6 "tmpf.XXXXXXXXXX"

extern int disk_iteration_count;	/* specified by user, test file size in kbytes */

#undef EXTERN


typedef struct {		/* useful for declarations */
	char *sccs;		/* contains SCCS info */
	char *filename;		/* filename */
	char *date;		/* date of compilation */
	char *time;		/* time of compilation */
} source_file;			/* make it a typedef */

/* TODO - replace this with something less global */
struct disk_data {
	int numdirs;
	char fn1arr[MAXDRIVES][STRLEN];
	char fn2arr[MAXDRIVES][STRLEN];
	char dkarr[MAXDRIVES][STRLEN];
};
struct runloop_input {
	int total_hits;
	int procs_per_user;
	double tpm;
	double tpm_per_user;
	long mhertz;
	int runnum;
	double a_tn;
	double a_tn1;
	int brief;
};

/* This struct is used to pass params around. 
 * Some of these items have global equivalents, but
 * i'm duplicating here for neatness
 */
struct input_params {
	int debug;
	int verbose;
	int maxusers;
	int minusers;
	int crossover;
	int poolsize;
	int filesize;
	int incr;
	int jobs;
	int test_period;
	int maxjobs;
	int brief;
};

/* global poiners */
extern struct _aimList *global_list;
/* helper functions - declarations */
int read_config_file(char *cfile, struct input_params *inv);
void print_header_line();
void print_usage();
void cleanup(void);
void error_exit(int problem);
void write_file_header(char *suite_time);
void repeat_fix_results(int repeat, int type, int brief);
void write_csv_header();
void write_stp_sheader(char *nowtime, char *endtime);
void write_stp_mheader();
void close_stp_hout(double jmax);
void close_stp_single(char *etime, char *fintime);
void write_stp_hout(int forks, double jpm, double jps, double ptime,
		    double cutck, double cstck, double std_def, double jti,
		    double max, double min);

void write_stp_mout(int forks, double ptim, double jpm, double jpmc,
		    int jti);

void write_csv_out(int forks, double jpm, double jps, double ptime,
		   double ctime, double ctcks, double std_def, double jti,
		   double max, double min);
void write_file_out(int forks, double jpm, double jps, double ptime,
		    double ctime, double ctcks, double std_def, double jti,
		    double max, double min);

int write_debug_file(char *msg);


int disk_create_all_files();
int create_fakeh();
void disk_unlink_all_test_files(int which_files);
int reduce_list(int wlist[], int work);
long rtmsec(int reset);
long child_uticks();
long child_sticks();
long get_mhertz();
void clear_ipc();

int check_name(char *cmd, int hits);
int multiuser(struct input_params *inv, int totalhits);
int singleuser(struct input_params *invars, int work);
int adjust_adaptive_timer(struct runloop_input *rl,
			  struct input_params *inv, int cnt);
void kill_all_child_processes(int sig);
extern struct disk_data *my_disk;
/* globals defined for getopts */
extern int flag;
extern int opt_num;
extern int debug_l;
extern int verbose;
extern int timeron;
extern double xover_threshold;



/* Error exit conditions */
#define ERR_BADLINE	1	/* line in workfile not filesize or test call */
#define ERR_BADFILE	2	/* missing workfile */
#define ERR_BADSIZE	3	/* file/pool size not specified correctly */
#define ERR_BADCONFIG	4	/* config file wrong */
#endif
