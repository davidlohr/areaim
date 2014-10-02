
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

#include <stdio.h>		/* enable printf(), etc. */
#include <stdlib.h>		/* enable exit(), etc. */
#include <math.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>		/* for stat call */
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>


/* local includes */
#include "suite.h"
#include "files.h"
#include "aimlist.h"
#include "../include/test.h"
#include "../include/usctest.h"

/* defines */
#define INFINITE 999999999	/* if we are running to crossover */
#define SLEEP 1
#define PER_CHILD_ALARM 1200

/* to make libltp happy */
char *TCID = "driver";		/* Test program identifier.    */
int TST_TOTAL = 1;		/* Total number of test cases. */
extern int Tst_count;		/* Test Case counter for tst_* routines */

/* helper functions */

/* main functions */
int runloop(struct _aimList *tlist, struct runloop_input *rl);
void child_sighandler(int sig);
void parent_sighandler(int sig);
void alarm_handler(int sig);
int output_result(struct _aimlItem *item, long time, int count,
		  int testnum, FILE * ss);
int output_stp(struct _aimlItem *item, long time, int count, int testnum);



/* timing functions */
long timestamp();
long timedelta(long start_time);


/* libltp functions */
extern int forker();
extern int Forker_npids;	/* num of forked pid, defined in forker.c */


/* Ugly Global Goop */
int disk_iteration_count;	/* this is really the 'filesize' parameter *
				 * it's declared extern in suite.h and used *
				 * by the test function */
/* static int filesize, poolsize;	 read from workload file */
/* global data to avoid changing register_test */
struct runloop_input *rl_vars;
struct disk_data *my_disk;
struct _aimList *global_list;

int flag = 0;
/* for getopt */
int opt_num = 0;
int debug_l = 0;
int verbose = 0;
/* global defaults */
int timeron = 1;
double xover_threshold = 10.0;
/* for single loop */
/* the global logfile prefix (ugly, but at least configurable */
extern char* logfile_prefix_g;

int main(int argc, char **argv)
{
	struct input_params invars;
	int i, n, c;		/* counters */
	int found_test;		/* for arguments */
	int total_work = 0;	/* holds total number of tests */
	source_file *(*s) ();	/* used to get function pointer */
	char buf[132];
	int total_hits = 0;	/* total of all hits */
	/* switches for aim 7 vs aim 9 */
	int multi = 0;
	int single = 0;

	FILE *fp;		/* for the workload file */
	char *wfname = NULL;	/* workload file name */
	char *cfname = NULL;    /* config file name */
	pid_t parent_pid;
	void (*sigvalu) ();
	source_file *p;
	/* tmp variables for command line configurations 
	 * command line overriddes the config file
	 * So we need temporaries even for our globals.
	 * 
	 */
	int t_xover = 0;
	int t_maxjobs = 0;
	int t_minu = 0;
	int t_maxu = 0;
	int t_incr = 0;
	int t_jobs = 0;
	int t_filesize = 0;
	int t_poolsize = 0;
	int config_res = 0;
	int t_test_p = 0;
	/* Always run once */
	int repeat = 1;
	int run_num = 0;
	int t_brief = 0;

	struct _aimList tlist;	/* linked list of tests to be run */

	global_list = &tlist;
	/* Current outputs 
	 * Console - happens by default, can't be switched off 
	 * logfile - name and path are fixed, can be switched off
	 * debug - name and path are fixed, can be switched on
	 */
	/* Parent signal handling. ( magic from Caldera version ) */
	/* If we are already ignoring SIGUSR1 setpgid 
	 * Otherwise, setpgrp() so signals don't mess with run scripts
	 */

	parent_pid = getpid();
	sigvalu = signal(SIGUSR1, SIG_IGN);
	if (sigvalu == SIG_IGN) {
		(void) setpgid(0, parent_pid);
	} else {
		(void) signal(SIGUSR1, sigvalu);
		(void) setpgrp();
	}
	(void) signal(SIGUSR1, SIG_IGN);
	/* setup global kill in case child dies bad */
	(void) signal(SIGTERM, kill_all_child_processes);
	/* setup of needed global data structs */
	if ((my_disk =
	     (struct disk_data *) malloc(sizeof(struct disk_data))) ==
	    NULL) {
		(void) fprintf(stderr, "Failed to malloc disk_data\n");
		exit(1);
	}


	/* Setup input defaults, default is 5 user run for now */
	invars.minusers = 1;
	invars.maxusers = 5;
	invars.incr = 1;
	invars.jobs = 100;
	invars.crossover = 0;
	invars.filesize = 0;
	invars.poolsize = 0;
	invars.test_period = 10;
	invars.verbose = 0;
	invars.maxjobs = 0;
	invars.brief = 0;
	invars.debug = 1;

	logfile_prefix_g = NULL;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose", 0, NULL, 'v'},
			{"startusers", 1, &opt_num, 1},
			{"endusers", 1, &opt_num, 2},
			{"increment", 1, &opt_num, 3},
			{"jobs", 1, &opt_num, 4},
			{"debug", 2, &opt_num, 5},
			{"logprefix", 1, NULL, 'l'},
			{"file", 1, NULL, 'f'},
			{"crossover", 0, NULL, 'x'},
			{"multiuser", 0, NULL, 'm'},
			{"brief", 0, NULL, 'b'},
			{"quick", 0, NULL, 'q'},
			{"oneuser", 0, NULL, 'o'},
			{"timeroff", 0, NULL, 't'},
			{"repeat", 1, NULL, 'r'},
			{"period", 1, NULL, 'p'},
			{"help", 0, NULL, 'h'},
			{"config", 1, NULL, 'c'},
			{"guesspeak", 0, NULL, 'g'}, /* terrible, but we've exhausted the alphabet */
 			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "bvs:e:i:j:d::f:l:p:r:c:mqothxg",
				long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (opt_num == 1) {
				t_minu = atoi(optarg);
			} else if (opt_num == 2) {
				t_maxu = atoi(optarg);
			} else if (opt_num == 3) {
				t_incr = atoi(optarg);
			} else if (opt_num == 4) {
				t_jobs = atoi(optarg);
			} else if (opt_num == 5) {
				if (optarg != 0) {
					debug_l = atoi(optarg);
				} else {
					debug_l = 1;
				}
			} else {
				fprintf(stderr, "bad option\n");
				exit(1);
			}
			break;
		case 'f':
			wfname = optarg;
			break;
		case 'l':
			logfile_prefix_g = optarg;
			break;
		case 'j':
			t_jobs = atoi(optarg);
			break;
		case 's':
			t_minu = atoi(optarg);
			break;
		case 'e':
			t_maxu = atoi(optarg);
			break;
		case 'i':
			t_incr = atoi(optarg);
			break;
		case 'd':
			if (optarg != 0) {
				debug_l = atoi(optarg);
			} else {
				debug_l = 1;
			}
			break;
		case 'x':
			t_xover = 1;
			t_maxu = INFINITE;
			break;
		case 'b':
			t_brief = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'm':
			multi = 1;
			break;
		case 'o':
			single = 1;
			break;
		case 'p':
			t_test_p = atoi(optarg);
			break;
		case 'q':
			xover_threshold = 60.0;
			t_xover = 1;
			t_maxu = INFINITE;
			break;
		case 'r':
			repeat = atoi(optarg);
			break;
		case 't':
			timeron = 0;
			break;
		case 'c':
		        cfname = optarg;
			break;
		case 'g':
			t_maxjobs = 1;
			t_maxu = INFINITE;
			break;
		case 'h':
			print_usage();
			exit(1);
			break;
		case '?':
			break;
		default:
			print_usage();
			exit(1);
			break;
		}
	}
	if (optind < argc) {
		printf("bad options: ");
		while (optind < argc) {
			printf("%s ", argv[optind++]);
		}
		printf("\n");
		exit(1);
	}
	if(cfname == NULL) {
		cfname = CONFIGFILE;
		fprintf(stderr, "Using default config file 'reaim.config'\n");
 	}
	config_res = read_config_file(cfname, &invars);

	/* at this moment we've read the config file and parsed all command line options 
	 * command line options over-rule config file */
	if (t_minu) {
		invars.minusers = t_minu;
	}
	if (t_maxu) {
		invars.maxusers = t_maxu;
	}
	if (t_xover) {
		invars.crossover = t_xover;
	}
	if (t_incr) {
		invars.incr = t_incr;
	}
	if (t_jobs) {
		invars.jobs = t_jobs;
	}
	if (t_test_p) {
		invars.test_period = t_test_p;
	}
	if (invars.verbose) {
		verbose = invars.verbose;
	}
	if (t_maxjobs) {
		invars.maxjobs = t_maxjobs;
	}
	if (t_brief) {
		invars.brief = t_brief;
	}
	if (debug_l) {
	         invars.debug = debug_l;
	}


	if (invars.minusers > invars.maxusers) {
		fprintf(stderr, "Cannot run backwards\n");
		exit(1);
	}
	if (invars.crossover && invars.maxjobs) {
		fprintf(stderr, "Cannot run both crossover and maxjobs\n");
		exit(1);
	}
	if ((multi == 1) && (single == 1)) {
		fprintf(stderr,
			" Cannot run both multiuser and singleuser - choose one\n");
	} else if ((multi == 0) && (single == 0)) {
		multi = 1;
		if (verbose) {
			fprintf(stderr, "Defaulting to multiuser run\n");
		}
	}
	if (wfname == NULL) {
		wfname = WORKFILE;
		fprintf(stderr,
			"No workfile specified, using default 'workfile'\n");
	}
	if(logfile_prefix_g == NULL) {
		logfile_prefix_g = LOGFILEPREFIX;
		fprintf(stderr,"No logfile prefix specified, using default 'reaim'\n");
	}

	/* TODO -> more option tests */

	aiml_init(global_list);
	/* old header */
	if (verbose) {
		printf("\nFile\t\tDate\t\tTime\t\tSCCS\n");
		printf
		    ("---------------------------------------------------------\n");
		fflush(NULL);
	}

	/* set up the linked list 
	 * The linked list holds - 
	 * pointer to cmdargs array for the test
	 * the number of hits (test weight) 
	 * a pointer to a Result array */

	/* at the end of this loop our linked list will be built
	 * global_list->size will hold our total number of known tests 
	 * the p = s(); statement will actually call register_test */

	for (i = 0; i < (int) Members(source_files); i++) {
		s = source_files[i];
		p = s();
		if (verbose) {
			printf("%-15s\t%s\t%s\t%s\n", p->filename, p->date,
			       p->time, p->sccs);
			fflush(NULL);
		}
	}
	/* free(p); */

	/* A proper workfile should have:
	 * # comments or headeris
	 * FILESIZE: <SIZE><K|M>
	 * POOLSIZE: <SIZE><K|M>
	 * <hits> <test_name>
	 */

	if ((fp = fopen(wfname, "r")) == NULL) {
		perror(argv[0]);
		error_exit(ERR_BADFILE);
	}
	/* read the workfile and build a list of tests
	 * get filesize and poolsize
	 */
	while (!feof(fp) && !(ferror(fp))) {
		char label[32], dim;
		char tmpcmd[40];
		int size, tmphits;

		if (fgets(buf, sizeof(buf) - 1, fp) != NULL) {

			if (*buf == '#') {
				continue;
			}
			/* skip comment */
			n = sscanf(buf, "%d %s", &tmphits, tmpcmd);	/* read line and hope it's a tuple */

			if ((n < 2) && (strlen(buf) > (size_t) 0)) {	/* bad read, not blank line */

				n = sscanf(buf, "%s %d%c", label, &size, &dim);	/* format: FILESIZE: 10K */

				if ((n < 3) && (strlen(buf) > (size_t) 0)) {
					error_exit(ERR_BADLINE);
				}

				if ((dim == 'M') || (dim == 'm')) {
					size *= KILO;
				}
				/* how many 1k buffers */
				else if ((dim != 'K') && (dim != 'k')) {
					error_exit(ERR_BADSIZE);
				}

				if ((*label == 'F') || (*label == 'f')) {
					t_filesize = size;
				} else if ((*label == 'P')
					   || (*label == 'p')) {
					t_poolsize = size;
				} else {
					error_exit(ERR_BADSIZE);
				}
				continue;
			}	/* end of bad read and not blank line */
		} /* fgets returned NULL */
		else if (!feof(fp)) {
			perror(argv[0]);
			error_exit(ERR_BADFILE);
		} else {
			continue;
		}

		found_test = check_name(tmpcmd, tmphits);
		if (!found_test) {
			(void) fprintf(stderr,
				       "Test '%s' has not been registered.\n",
				       tmpcmd);
			exit(1);
		}

		total_hits += tmphits;
		total_work++;

	}
	(void) fclose(fp);	/* close the file (we're done w/ it) */
	if (t_filesize) {
		invars.filesize = t_filesize;
	}
	if (t_poolsize) {
		invars.poolsize = t_poolsize;
	}
	if ((invars.filesize == 0) && (invars.poolsize == 0)) {
		error_exit(ERR_BADSIZE);
	}
	/* write the log files in order */
	run_num = 1;
	while (repeat > 0) {
		if (multi == 1) {
			multiuser(&invars, total_hits);
			if (repeat > 1) {
				repeat_fix_results(run_num, 2,
						   invars.brief);
			}
		} else if (single == 1) {
			singleuser(&invars, total_work);
			if (repeat > 1) {
				repeat_fix_results(run_num, 1,
						   invars.brief);
			}
		}
		repeat--;
		run_num++;
		system("sync");
		sleep(SLEEP);
	}
	cleanup();
	return 0;
}
int multiuser(struct input_params *invars, int totalhits)
{
	time_t vtime;
	char sdate[25];
	int ndir;
	char *ldate;
	int loop_result;	/* error return for test */
	int adapt_cnt = 0;
	double mjobs[5];
	double jstdev, jtot, jsumsq, javg, jpct, jmax;

	jstdev = jtot = jsumsq = jmax = mjobs[0] = mjobs[1] = mjobs[2] =
	    mjobs[4] = mjobs[3] = mjobs[5] = 0.0;

	if ((rl_vars =
	     (struct runloop_input *) malloc(sizeof(struct runloop_input)))
	    == NULL) {
		(void) fprintf(stderr, "Failed to malloc runloop_input\n");
		exit(1);
	}
	rl_vars->total_hits = totalhits;
	rl_vars->procs_per_user = invars->jobs;
	rl_vars->tpm = 0.0;
	rl_vars->tpm_per_user = 0.0;
	rl_vars->mhertz = get_mhertz();
	rl_vars->runnum = 0;
	rl_vars->a_tn = 0.0;
	rl_vars->a_tn1 = 0.0;
	rl_vars->brief = invars->brief;

	/* get current time for suite header */
	time(&vtime);
	ldate = ctime(&vtime);
	sscanf(ldate, "%*s %21c", sdate);
	sdate[20] = '\0';
	/* output_suite_header(mname, sdate); */
	write_file_header(sdate);
	write_csv_header();
	/* TODO check if using datapoints file */
	/* TODO do proper numdirs */
	ndir = my_disk->numdirs;
	if (invars->debug) {
		fprintf(stderr, "DEBUG: Number of directories is %d\n",
			ndir);
		fflush(stderr);
	}
	/*
	 * if disk file size doesn't change w/ userload, create disk files to be 
	 * used by disk tests ; otherwise, must do below on a per-userload basis 
	 */
	/* for now fakeh.tar is always created. */
	if (create_fakeh() == -1) {
		perror("create_fakeh()");
		(void) fprintf(stderr, "fakeh tar file creation failed\n");
		kill_all_child_processes(0);
	}

	if (invars->poolsize == 0) {
		disk_iteration_count = invars->filesize;	/* set size global, poolsize 0 here */
		if (disk_create_all_files() == -1) {
			perror("disk_create_all_files()");
			(void) fprintf(stderr,
				       "disk work file creation failed\n");
			kill_all_child_processes(0);
		}
	}
	if (invars->brief) {
		write_stp_mheader();
	} else {
		print_header_line();
	}
	/* loop through the users */
	for (rl_vars->runnum = invars->minusers;
	     rl_vars->runnum <= invars->maxusers;) {
		time_t my_time;
		if (invars->verbose) {
			printf("Loop number %d\n", rl_vars->runnum);
			fflush(NULL);
		}

		/*
		 * if poolsize non-zero, disk filesize changes with userload, must re-create disk files
		 */
		if (invars->poolsize != 0) {
			/*
			 * filesize & poolsize are the "number of NBUFSIZE buffers" needed to 
			 * get the disk file sizes specified at the user prompts.
			 * we truncate instead of round so never go over poolsize
			 */
			disk_iteration_count =
			    invars->filesize +
			    (invars->poolsize / rl_vars->runnum);
			if (disk_create_all_files() == -1) {
				perror("disk_create_all_files()");
				(void) fprintf(stderr,
					       "disk work file creation failed\n");
				kill_all_child_processes(0);
			}
		}
		/*
		   (void) printf("%5d", rl_vars->runnum);
		 */
		fflush(stdout);
		system("sync;sync;sync");	/* clean out the cache, boosts performance */
		loop_result = runloop(global_list, rl_vars);
		time(&my_time);

		/* reset to correct for rounding in child loop */
		rl_vars->procs_per_user = invars->jobs;

		if (rl_vars->tpm > jmax) {
			jmax = rl_vars->tpm;
		}
		if (rl_vars->runnum == invars->maxusers) {
			break;
		}
		if ((invars->crossover)
		    && (rl_vars->tpm_per_user <= xover_threshold)) {
			printf("Crossover achieved\n");
			break;
		}
		if (invars->maxjobs) {
			mjobs[4] = mjobs[3];
			mjobs[3] = mjobs[2];
			mjobs[2] = mjobs[1];
			mjobs[1] = mjobs[0];
			mjobs[0] = rl_vars->tpm;
			jtot =
			    mjobs[0] + mjobs[1] + mjobs[2] + mjobs[3] +
			    mjobs[4];
			javg = jtot / 5;
			if (mjobs[4] > 0.0) {
				if (mjobs[0] < mjobs[1]) {
					if ((((javg -
					       mjobs[0]) / javg) * 100) >
					    1.00) {
						printf
						    (" Job rate dropping avg: %3.2f loss pct: %3.2f\n",
						     javg,
						     ((javg -
						       mjobs[0]) / javg) *
						     100);
						break;
					} else {
						if (invars->verbose) {
							printf
							    ("Job drop pct %3.2f\n",
							     ((javg -
							       mjobs[0]) /
							      javg) * 100);
						}
					}
				}
			}
			jsumsq =
			    (mjobs[0] * mjobs[0]) + (mjobs[1] * mjobs[1]) +
			    (mjobs[2] * mjobs[2])
			    + (mjobs[3] * mjobs[3]) +
			    (mjobs[4] * mjobs[4]);
			jstdev = sqrt((jsumsq - jtot * jtot / 5) / 5);
			jpct = (jstdev / javg) * 100.0;
			if (invars->verbose) {
				printf
				    ("Current std %3.2f avg %3.2f pct %3.2f\n",
				     jstdev, javg, jpct);
			}
			if (jpct < 1.00) {
				printf("Max sustained jobs reached\n");
				break;
			}

		}
		/* loop exit */
		/* TODO  finish the loop code */
		if (timeron) {
			invars->incr =
			    adjust_adaptive_timer(rl_vars, invars,
						  adapt_cnt);
			adapt_cnt++;
		}
		rl_vars->runnum = rl_vars->runnum + invars->incr;
		if (invars->poolsize != 0) {
			disk_unlink_all_test_files(JUST_TEMP_FILES);
		}

	}
	printf("Max Jobs per Minute %.2f\n", jmax);
	if (invars->brief) {
		close_stp_hout(jmax);
	}
	/* free(rl_vars); */
	return 0;


}


/* One loop of the AIM7 run
 * Walk the list of tests
 * hits per test = workfile info/total hits * number of processes per user
 */

int runloop(struct _aimList *tlist, struct runloop_input *rl)
{
	/* DEBUG - temp log file */

	/* for forker section */
	int kids_forked = 0;
	int arbitrary_msg = 1;
	int num_children = 0;
	int umbilical[2];

	int i, j, k, tmp_indx, jti = 0;
	int fork_ret, forker_inc;
	int func_ret, status;
	struct _aimList wlist;
	double child_utcks;
	double child_stcks;

	int numdir = my_disk->numdirs;

	int (*func) ();

	struct _aimlItem *item;
	struct Cargs *myargs;
	pid_t my_pid, pa_pid, big_kid, tiny_kid, dead_kid;

	char child_string[STRLEN];	/* for error message */
	char err_string[STRLEN];	/* param string for func */
	char dbug_msg[STRLEN];
	char *ptr = NULL;
	/* timing variables */
	long rt1, cut1, cst1;
	int tmp_load, tmp_total;
	long mhertz = rl->mhertz;


	/* initalize */
	/* timing data */
	double processes_per_second, real_time, new_load, per_proc_avg,	/* temporary variable */
	 elapsed_seconds = 0.0,
	    sum_elapsed_times = 0.0,
	    sumsq_elapsed_times = 0.0, std_dev = 0.0, cov = 0.0, dbmax =
	    0.0, dbmin = 9990.0;
	big_kid = tiny_kid = 0;


	/* create our temp list for the work load */
	aiml_init(&wlist);

	/* walk the list and figure what percentage 
	 * of the specified # of user processes goes to this test
	 */
	i = tmp_total = 0;
	item = list_head(tlist);
	while (1) {
		new_load =
		    ((double) item->hits / rl->total_hits) *
		    rl->procs_per_user;
		tmp_load = (int) new_load;
		/* round off to int, run each test at least once */
		if ((new_load - (double) tmp_load) >= 0.5) {
			tmp_load += 1;
		} else if ((new_load - (double) tmp_load) != 0.0) {
			if (tmp_load == 0) {
				tmp_load = 1;
			}
		}
		if (aiml_insert(&wlist, item->myargs, item->res, tmp_load)
		    != 0) {
			exit(1);
		}
		tmp_total += tmp_load;
		if (debug_l >= 3) {
			myargs = get_Cargs(item);
			sprintf(dbug_msg,
				"tst name %s hits:%d NewLd:%f rnded:%d totHits:%d ppU:%d\n",
				myargs->name, item->hits, new_load,
				tmp_load, rl->total_hits,
				rl->procs_per_user);
			write_debug_file(dbug_msg);
		}
		if (list_is_tail(item)) {
			break;
		} else {
			item = item->next;
			i++;
		}
	}
	/*
	 * recompute the number of jobs for the selected mix
	 */
	rl->procs_per_user = tmp_total;
	/*
	 * open pipe for communication with children, they will send a message up when they are
	 * forked and ready to run, otherwise we may start the tests before all kids are ready
	 */
	if (pipe(umbilical) < 0) {	/* problem making a pipe */
		close(umbilical[0]);
		close(umbilical[1]);
		fprintf(stderr, "cannot create pipe to children\n");
		return 1;
	}

	processes_per_second = real_time = 0.0;
	/* we use the ltpltp forker() function to spawn all the 
	 * children. See forker.c 
	 * forker spawns n - 1 copies 
	 * parent gets a count
	 * TODO Think more about using mode 1 
	 */
	pa_pid = getpid();
	if (debug_l) {
		sprintf(dbug_msg, "Parent pid %d\n", pa_pid);
		write_debug_file(dbug_msg);
	}

	forker_inc = rl->runnum + 1;
	fork_ret = forker(forker_inc, 0, "aim_fork");
	my_pid = getpid();

	if (my_pid != pa_pid) {

		/* Child code */
		int dj, randnum;
		long start_tick;
		long delta = 0;
		int chld_alrm = 0;


		close(umbilical[0]);
		/* Step 1: seed random number generators
		 * We use a small integer which increments 
		 * for each child, the child pid looks handy
		 * CHANGE: old AIM used the array index, a slightly
		 * smaller integer
		 */
		aim_srand(my_pid);
		aim_srand2(my_pid);
		/* Initalize signal handlers
		 *
		 */
		tst_sig(FORK, child_sighandler, NULL);

		/* msg to parent */
		if (write
		    (umbilical[1], &arbitrary_msg,
		     sizeof arbitrary_msg) < 0) {
			fprintf(stderr,
				"Child pid %d cannot write to parent\n",
				my_pid);
			exit(-1);
		}
		close(umbilical[1]);
		/* TODO see about - TEST_PAUSE; here */
		/* TEST_PAUSE; -didn't work realy well */
		pause();

		if (disk_iteration_count > 9) {
			chld_alrm = disk_iteration_count;
		} else {
			chld_alrm = 10;
		}
		/* now we set a timeout alarm */
		alarm(rl->runnum * chld_alrm);
		/*
		 * Step 4: Set up mechanism for random 
		 * selection of directory for writes during tests
		 */
		dj = (numdir > 0 ? (aim_rand2() % numdir) : 0);
		/* Step 5 run the loads 
		 * reduce_list returns an array index
		 * known to have work */
		item = list_head(&wlist);
		for (j = 0; j < rl->procs_per_user; j++) {
			tmp_indx = 0;
			while (tmp_indx == 0) {
				randnum = aim_rand();
				k = randnum % wlist.size;
				while (k > 0) {
					if (item->next == NULL) {
						item = list_head(&wlist);
					} else {
						item = item->next;
					}
					k--;
				}
				if (item->hits > 0) {
					item->hits--;
					tmp_indx = 1;
				} else {
					if (debug_l) {
						sprintf(dbug_msg,
							"removing %s\n",
							item->myargs->
							name);
						write_debug_file(dbug_msg);
					}
					aiml_remove(&wlist, item, debug_l);
					item = list_head(&wlist);
				}
			}

			/* get the args for the tast 
			 * we have to walk the list here
			 * replaces the call to find_arguments */
			tmp_indx = k;
			myargs = get_Cargs(item);
			func = myargs->f;
			(void) sprintf(err_string, "%s  ", myargs->name);
			/* The disk tests use a single arg, 'DISKS' 
			 * this is replaced by the disk directory name
			 * from the config file 
			 */
			if (strcmp(myargs->args, "DISKS") == 0) {
				if (numdir > 1) {
					ptr = my_disk->dkarr[dj % numdir];
					dj++;
					(void) strcat(err_string, ptr);
				} else if (numdir == 1) {
					ptr = my_disk->dkarr[0];
					(void) strcat(err_string, ptr);
				}
				myargs->args = ptr;

			}
			errno = 0;
			(void) sprintf(child_string, "\nChild #%d: ",
				       fork_ret);
			if (debug_l >= 2) {
				sprintf(dbug_msg, "pid:%d: Name: %s\n",
					fork_ret, myargs->name);
				write_debug_file(dbug_msg);
			}
			start_tick = timestamp();
			func_ret =
			    (*func) (myargs->argc, &myargs->args,
				     item->res);
			if (func_ret < 0) {
				perror(child_string);
				(void) fprintf(stderr,
					       "\nFailed to execute\n\t%s\n",
					       err_string);
				(void) kill(pa_pid, SIGTERM);
				exit(1);
			}
			delta = timedelta(start_tick);
			item->tot_time += delta;
			item->count++;
			if (delta > item->max_time) {
				item->max_time = delta;
			}
			/* to catch the last item on the list */
			if (item->hits == 0) {
				if (debug_l) {
					sprintf(dbug_msg,
						"removing at eol %s\n",
						item->myargs->name);
					write_debug_file(dbug_msg);
				}
				aiml_remove(&wlist, item, debug_l);
				item = list_head(&wlist);
			}

			/* TODO - aim9 style reporting should be collected here */
			if (debug_l >= 2) {
				sprintf(dbug_msg,
					"pid %d CUload:%d Test:%s delta:%ld st:%ld seconds %ld\n",
					my_pid, rl->runnum, myargs->name,
					delta, start_tick,
					delta * 1000 / mhertz);
				write_debug_file(dbug_msg);
			}

		}
		/* TODO logfile
		 * if (!no_logfile) {
		 * fclose(logfile); }
		 */
		free(item);
		exit(0);
		/* end of child code */
	} else {
		/* in parent */
		/* This did not work, but it's not dead yet */
		/* tst_sig(FORK, parent_sighandler, NULL); */
		if (fork_ret != rl->runnum) {
			(void) fprintf(stderr,
				       "Wanted %d kids, current count %d\n",
				       rl->runnum, fork_ret);
			/* TODO error exit here ? */
		}
		/* this is the AIM7 wait method - may need to change */
		while (kids_forked < rl->runnum) {
			int msg;
			if (read(umbilical[0], &msg, sizeof(msg)) < 0) {
				fprintf(stderr,
					"cannot read message from child\n");
				return 1;
			}
			kids_forked++;
		}
		close(umbilical[1]);
		close(umbilical[0]);
		sleep(SLEEP);
		/* the children are started here */
		rt1 = timestamp();
		cut1 = child_uticks();
		cst1 = child_sticks();
		(void) kill(0, SIGUSR1);

		/* Catching the children's exit */
		while (1) {
			dead_kid = wait(&status);
			if ((dead_kid > 0) && (kids_forked > 0)) {
				if (WIFEXITED(status)) {
					/*
					   (void) fprintf(stderr,
					   "Child called exit(), status = %d\n",
					   WEXITSTATUS
					   (status)); */
					/* calculate time child took to run
					 * store sum of elapsed times
					 * store square of elapsed times for std-deviation
					 */
					elapsed_seconds =
					    (double) timedelta(rt1) /
					    1000.0;
					/*temp hack for stats 
					   sprintf(dbug_msg,
					   "%d,%f", num_children, elapsed_seconds);
					   write_debug_file(dbug_msg);
					 */
					sum_elapsed_times +=
					    elapsed_seconds;
					sumsq_elapsed_times +=
					    (elapsed_seconds *
					     elapsed_seconds);
					if (elapsed_seconds > dbmax) {
						dbmax = elapsed_seconds;
						big_kid = dead_kid;
					}
					if (elapsed_seconds < dbmin) {
						dbmin = elapsed_seconds;
						tiny_kid = dead_kid;
					}
					status++;

					num_children++;
					if (debug_l) {
						sprintf(dbug_msg,
							" pid:%d elapSec:%g SES: %g SET: %g\n",
							my_pid,
							elapsed_seconds,
							sum_elapsed_times,
							sumsq_elapsed_times);
						write_debug_file(dbug_msg);
					}
					/* TODO add log file output */

				} else if (WIFSIGNALED(status)) {
					(void) fprintf(stderr,
						       "Child exited with uncaught signal # %d\n",
						       WTERMSIG(status));
					if (WCOREDUMP(status)) {
						(void) fprintf(stderr,
							       "Child dumped core\n");
					}
				} else if (WIFSTOPPED(status)) {
					(void) fprintf(stderr,
						       "Child stopeed by signal #%d\n",
						       WSTOPSIG(status));
				}
				--kids_forked;

				if (kids_forked == 0) {
					break;
				}
				/* wait(status) <= 0 */
			} else {
				if (errno == ECHILD) {
					/* TODO - kill children? */
					fprintf(stderr, "caught ECHILD\n");
					break;
				}
			}
		}		/* end while(1) */
		sprintf(dbug_msg, "loop %d", rl->runnum);
		write_debug_file(dbug_msg);
		/* Testing over - results calculation */
		real_time = (double) timedelta(rt1) / 1000.0;
		/* procs/second */
		processes_per_second =
		    (double) rl->procs_per_user / real_time;
		/* jobs per minute */
		rl->tpm =
		    processes_per_second * 60.0 * (double) rl->runnum;
		/* calculate the Job Timing index 
		 * Requires more that one child */
		if (num_children == 1) {
			std_dev = cov = 0.0;
			jti = 100.0;
		} else if (num_children != 0) {
			std_dev =
			    sqrt((sumsq_elapsed_times -
				  sum_elapsed_times * sum_elapsed_times /
				  num_children) / num_children);
			if (sum_elapsed_times == 0) {
				cov = 0.0;
			} else {
				cov =
				    std_dev / (sum_elapsed_times /
					       num_children);
			}
			jti = (int) cov > 1.0 ? 0.0 : (1 - cov) * 100.0;
		}
		/* update adaptive timers */
		if (rl->a_tn1 > 0.0) {
			rl->a_tn = rl->a_tn1;
		}
		rl->a_tn1 = real_time;
		child_stcks = (double) (child_sticks() - cst1) / mhertz;
		child_utcks = (double) (child_uticks() - cut1) / mhertz;
		/* output realtime to screen */
		rl->tpm_per_user = processes_per_second * 60.0;
		per_proc_avg = (double) real_time / num_children;
		if (rl->brief) {
			write_stp_mout(num_children, real_time, rl->tpm,
				       rl->tpm_per_user, jti);
			write_stp_hout(num_children, rl->tpm,
				       processes_per_second, real_time,
				       child_utcks, child_stcks, std_dev,
				       jti, dbmax, dbmin);
		} else {
			(void)
			    printf
			    ("%-8d%-8.2f %-7.2f %-7.2f %-10.2f %-10.2f %-8.2f %-8.2f %-4d \n",
			     num_children, real_time, child_stcks,
			     child_utcks, rl->tpm, rl->tpm_per_user,
			     std_dev, (cov * 100.0), jti);
			fflush(stdout);
		}
		write_file_out(num_children, rl->tpm, processes_per_second,
			       real_time, child_utcks, child_stcks,
			       std_dev, jti, dbmax, dbmin);
		write_csv_out(num_children, rl->tpm, processes_per_second,
			      real_time, child_utcks, child_stcks,
			      std_dev, jti, dbmax, dbmin);

		if (debug_l) {
			sprintf(dbug_msg,
				"Big kid pid %d tiny kid pid %d\n",
				big_kid, tiny_kid);
			write_debug_file(dbug_msg);
		}
		/* TODO if debug create cov.ss */


	}			/* end of parent */
	/* free(&wlist); */
	return 0;
}


void register_test(char *name, int argc, char *args, int (*f) (),
		   int factor, char *units)
{
	struct Cargs *tmpargs;
	struct _aimlItem *new;
	struct _aimlItem *tmp;
	struct Result *res;

	if ((res =
	     (struct Result *) malloc(sizeof(struct Result))) == NULL) {
		fprintf(stderr, "Failed to malloc Results ");
		exit(1);
	}
	if ((tmpargs =
	     (struct Cargs *) malloc(sizeof(struct Cargs))) == NULL) {
		fprintf(stderr, "Failed to malloc Cargs ");
		exit(1);
	}

	if ((new =
	     (struct _aimlItem *) malloc(sizeof(struct _aimlItem))) ==
	    NULL) {
		fprintf(stderr, "Failed to malloc aimlist ");
		exit(1);
	}
	tmpargs->name = name;
	tmpargs->args = args;
	tmpargs->f = f;
	tmpargs->factor = factor;
	tmpargs->units = units;
	tmpargs->argc = argc;

	new->myargs = tmpargs;
	new->res = res;
	new->hits = 0;
	new->tot_time = 0;
	new->max_time = 0;
	new->count = 0;
	new->next = NULL;
	if (global_list->head == NULL) {
		global_list->head = new;
		global_list->tail = new;
	} else {
		tmp = global_list->tail;
		tmp->next = new;
		global_list->tail = new;
	}
	global_list->size++;

}


/* This routine fills in a struct of disk name goop
 * for the test routines
 * The struct is found at the global pointer my_disk
 * it's not great but what can ya do
 * The number of directories is returned and used by the invars struct
 */

int read_config_file(char *cfname, struct input_params *inv)
{
	int numd;
	FILE *fp;
	char rstring[STRLEN];
	char label[32];
	char dim;
	int size;
	int brf;
	int n;
	unsigned int i;

	fp = fopen(cfname, "r");	/* open config file */
	if (fp == NULL) {	/* if error, stop here */
		(void) fprintf(stderr, "No config file: %s\n", cfname);
		exit(1);
	}
	numd = 0;		/* start with no directories */
	while (fgets(rstring, STRLEN, fp) != NULL) {	/* read 1 line from file */
		/* make string upper case */
		for(i=0;i<strlen(rstring);i++) {
			if(rstring[i] == ' ')
				break;
			rstring[i] = toupper(rstring[i]);
		}

		switch (rstring[0]) {	/* parse from first character */

		case '#':	/* comment */
			break;
		case ' ':
			break;	/* blank line */
		case 'B':
			n = sscanf(rstring, "%s %d", label, &brf);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADLINE);
			}
			inv->brief = brf;
			break;
		case 'C':
			n = sscanf(rstring, "%s %d", label, &size);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADCONFIG);
			}
			if (size == 1) {
				inv->crossover = 1;
			} else {
				inv->crossover = 0;
			}
			break;
		case 'D':	/* Directories for disk exerciser */
			n = sscanf(rstring, "%s %s", label,
				   my_disk->dkarr[numd]);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADLINE);
			}
			if (inv->verbose) {
				printf("\nUsing disk directory <%s>",
				       my_disk->dkarr[numd]);
			}
			numd++;
			break;
		case 'E':
			n = sscanf(rstring, "%s %d", label, &size);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADCONFIG);
			}
			inv->maxusers = size;
			break;
		case 'F':
			n = sscanf(rstring, "%s %d%c", label, &size, &dim);
			if ((n < 3) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADLINE);
			}
			if ((dim == 'M') || (dim == 'm')) {
				size *= KILO;
			} else if ((dim != 'K') && (dim != 'k')) {
				error_exit(ERR_BADSIZE);
			}
			inv->filesize = size;
			break;
		case 'I':
			n = sscanf(rstring, "%s %d", label, &size);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADCONFIG);
			}
			inv->incr = size;
			break;
		case 'J':
			n = sscanf(rstring, "%s %d", label, &size);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADCONFIG);
			}
			inv->jobs = size;
			break;
		case 'P':
			n = sscanf(rstring, "%s %d%c", label, &size, &dim);
			if ((n < 3) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADLINE);
			}
			if ((dim == 'M') || (dim == 'm')) {
				size *= KILO;
			} else if ((dim != 'K') && (dim != 'k')) {
				error_exit(ERR_BADSIZE);
			}
			inv->poolsize = size;
			break;
		case 'V':
			n = sscanf(rstring, "%s %d", label, &size);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADCONFIG);
			}
			if (size == 1) {
				inv->verbose = 1;
			} else {
				inv->verbose = 0;
			}
			break;
		case 'S':
			n = sscanf(rstring, "%s %d", label, &size);
			if ((n < 2) && (strlen(rstring) > (size_t) 0)) {
				error_exit(ERR_BADCONFIG);
			}
			inv->minusers = size;
			break;
		default:
			fprintf(stderr, "unknown line in config file %s\n",
				cfname);
			break;
		}		/* end of switch */
	}			/* end of loop */
	printf("\n");
	(void) fclose(fp);	/* end of file */

	if (numd)		/* directory test? */
		if ((strcmp(my_disk->dkarr[0], "OFF")) == 0)	/* if 1st one is OFF */
			numd = 0;	/* ALL is OFF */
	my_disk->numdirs = numd;
	return 0;
}

/*
 * reduce_list()
 *      Used in sampling without replacement in runtap().
 *      Contributed by Jim Summerall of DEC.
 * 12/13/89 TVL
 */
int reduce_list(int wlist[], int work)
{
	register int i, total;
	int rlist[WORKLD];

	for (i = 0, total = 0; i < work; i++) {
		if (wlist[i] == 0)
			continue;
		else
			rlist[total++] = i;
	}
	if (total)
		i = aim_rand() % total;
	else {
		(void) fprintf(stderr, "FATAL ERROR - DIVIDE BY ZERO\n");
		(void) fprintf(stderr, "reduce_list(): total = 0\n");
		exit(1);
	}
	return (rlist[i]);
}


/* simple idea for a timestamp function set
 * first function returns a time stamp. 
 * We really don't care what the timestamp means
 */
long timestamp()
{				/* before each test */
	long period;

	period = times(NULL);
	return period;
}

/* second function returns the time in 
 * milliseconds since the first timestamp
 * This version is based on the times() call
 */
long timedelta(long start_time)
{
	long stamp;
	long delta;
	long hz = get_mhertz();
	stamp = times(NULL);
	if (hz == 1000) {
		delta = stamp - start_time;
	} else {
		delta = ((stamp - start_time) * 1000) / hz;
	}
	return delta;
}



/* We are now using the libltp signal handler 
 * so we have one routine for children, one for parent.
 * tests are now gunned with SIGUSR1, which should be okay
 */
void parent_sighandler(int sig)
{
	int dummy = 0;
	if (sig == SIGUSR1) {
		/* printf("Parent caught SIGUSR1\n");
		 */
		dummy++;
	} else if (sig == SIGUSR2) {
		fprintf(stderr, "Parent caught SIGUSR2\n");
	} else if (sig == SIGCHLD) {
		fprintf(stderr, "Parent caught SIGCHLD\n");
	} else {
		fprintf(stderr, "Parent caught raw signal %d\n", sig);

		dummy++;
	}
}

void child_sighandler(int sig)
{
	int status;
	switch (sig) {
	case SIGTERM:
		printf("sigterm\n");
	case SIGQUIT:
		printf("sigquit\n");
	case SIGSEGV:
		signal(sig, SIG_IGN);
		cleanup();
		break;
	case SIGFPE:
	case SIGHUP:		/* this is the death_of_a_chile */
		printf("sighup\n");
		/* TODO - is this still okay? */
		signal(sig, SIG_IGN);
		(void) fprintf(stderr,
			       "\ndeath_of_child() received signal SIGHUP (%d)\n",
			       sig);
		if (wait(&status) > 0) {	/* harvesting children */
			if (WIFEXITED(status)) {
				(void) fprintf(stderr,
					       "Child exited normally, exit = %d\n",
					       WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				(void) fprintf(stderr,
					       "Child process exited on uncaught signal = %d\n",
					       WTERMSIG(status));
				if (WCOREDUMP(status)) {
					(void) fprintf(stderr,
						       "core dumped\n");
				}
			} else if (WIFSTOPPED(status)) {
				(void) fprintf(stderr,
					       "Child stopped by signal = %d\n",
					       WSTOPSIG(status));
			}
		}
		(void) kill(getppid(), SIGTERM);
		exit(1);
		break;		/* should never reach here */

		/* TODO - do we get many SIGFPE's these days? */
	case SIGALRM:
		(void) fprintf(stderr, "Child exceeded alarm timeout\n");
		exit(1);
		break;
	case SIGUSR1:
		/* printf("gun\n"); */
		break;
	case SIGUSR2:
		printf("sigusr2\n");
		break;
	default:
		fprintf(stderr, "Child caught raw signal %d\n", sig);
		break;
	}
}
void cleanup(void)
{
	/* TODO - this is clean up for pipe_test only */
	clear_ipc();

	disk_unlink_all_test_files(ALL_FILES);
}
int singleuser(struct input_params *invars, int work)
{
	/* do delta */
	time_t now, then, delta, finish;
	long start, endtime;

	int hr, mn, sc, trouble, count;
	int test_period;
	int testnum = 1;
	struct _aimlItem *item;
	struct Cargs *myargs;
	int (*func) ();
	char sign;
	int nd;
	FILE *ss;
	char *report_file = "singleuser.ss";
	char nowtime[STRLEN];
	char etime[STRLEN];
	char fintime[STRLEN];
	nd = my_disk->numdirs;
	test_period = invars->test_period;

	time(&now);
	delta = work * test_period;
	disk_iteration_count = invars->filesize;	/* set size global, poolsize 0 here */

	hr = delta / 3600;
	mn = (delta % 3600) / 60;
	sc = delta % 60;
	then = now + delta;
	sprintf(nowtime, "%s", ctime(&now));
	sprintf(etime, "%s", ctime(&then));

	ss = fopen(report_file, "w");
	if (ss == NULL) {
		fprintf(stderr, "Unable to open report file, exiting\n");
		exit(1);
	}
	if (invars->brief) {
		write_stp_sheader(nowtime, etime);
	} else {
		printf("\n\n");	/* tell user all of this */
		printf("Starting time:      %s", nowtime);	/* starting time */
		printf("Projected Run Time: %d:%02d:%02d\n", hr, mn, sc);
		printf("Projected finish:   %s", etime);
		printf("\n\n\n");
		printf
		    ("Test   Test        Elapsed   Iteration Iteration      Operation\n");
		printf
		    ("Number Name        Time(sec) Count     Rate(loops/s)  Rate (ops/sec)\n");
	}

	fprintf(ss, "Re-aim Workload - Single user test \n");
	fprintf(ss, "Disk iterations %d\n", disk_iteration_count);
	fprintf(ss, "Starting time:      %s", nowtime);	/* starting time */
	fprintf(ss, "Projected Run Time: %d:%02d:%02d\n", hr, mn, sc);
	fprintf(ss, "Projected finish:   %s\n\n", etime);

	fprintf
	    (ss,
	     "Test   Test        Elapsed   Iteration Iteration     Operation\n");
	fprintf(ss,
		"Number Name        Time(sec) Count     Rate(loops/s) Rate (ops/sec)\n");

	/* for now fakeh.tar is always created. */
	if (create_fakeh() == -1) {
		perror("create_fakeh()");
		(void) fprintf(stderr, "fakeh tar file creation failed\n");
		kill_all_child_processes(0);
	}
	item = list_head(global_list);
	while (1) {
		if (item->hits > 0) {
			myargs = get_Cargs(item);
			func = myargs->f;
			if (strcmp(myargs->args, "DISKS") == 0) {
				myargs->args = my_disk->dkarr[0];
				disk_create_all_files();
			}
			if (myargs->factor == WAITING_FOR_DISK_COUNT) {
				if (strncmp(myargs->name, "sync", 4) == 0) {
					myargs->factor =
					    invars->filesize / 2;
				} else {
					myargs->factor = invars->filesize;
				}
			}
			signal(SIGALRM, alarm_handler);
			flag = 1;
			alarm(test_period);
			start = timestamp();
			trouble = count = 0;
			while (flag) {
				if ((*func)
				    (myargs->argc, &myargs->args,
				     item->res) < 0) {
					perror(myargs->name);
					fprintf(stderr,
						"failed to execute %s\n",
						myargs->name);
					trouble = 1;
					break;
				}
				count++;
			}
			if (trouble == 0) {
				endtime = timedelta(start);
				if (invars->brief) {
					output_stp(item, endtime, count,
						   testnum);
				} 
				output_result(item, endtime, count,
						      testnum, ss);
				fflush(NULL);

				system("sync");
				testnum++;
			} else {
				fprintf(stderr, "skipping to next test\n");
			}
		}
		if (list_is_tail(item)) {
			break;
		} else {
			item = item->next;
		}
	}
	/* done with test loop */
	time(&finish);
	/* TODO prinf(line) */
	delta = finish - then;
	sign = ' ';
	if (delta < 0) {
		sign = '-';
		delta = -delta;
	}
	hr = delta / 3600;
	mn = (delta % 3600) / 60;
	sc = delta % 60;
	sprintf(fintime, "%s", ctime(&finish));
	printf("Projected Completion time:  %s", etime);
	printf("Actual Completion time:     %s", fintime);
	printf("Difference:                %c%d:%02d:%02d\n", sign, hr, mn,
	       sc);
	fprintf(ss, "\n\nProjected Completion time:  %s", etime);
	fprintf(ss, "Actual Completion time:     %s", fintime);
	fprintf(ss, "Difference:                %c%d:%02d:%02d\n", sign,
		hr, mn, sc);
	fclose(ss);
	if (invars->brief) {
		close_stp_single(etime, fintime);
	}
	return 0;

}
void alarm_handler(int sig)
{
	sig++;			/* compiler warning */
	flag = 0;
}

void kill_all_child_processes(int sig)
{
	/*
	 *  kill_all_child_processes sends signals to all child process and waits
	 *  for their death
	 */
	signal(SIGTERM, SIG_IGN);	/* ignore SIGTERM */
	disk_unlink_all_test_files(ALL_FILES);
	if (sig == 0)		/* if signal 0, problem */
		(void) fprintf(stderr,
			       "Fatal Error! SIGTERM (#%d) received!\n\n\n",
			       sig);
	(void) fprintf(stderr, "\n Testing over....\n\n");

	(void) kill(0, SIGTERM);
	while (wait((int *) 0) != -1);
	exit(0);
}
