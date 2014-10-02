// DAN 07/14/2009

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
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>		/* for stat call */
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>


#include "suite.h"
#include "files.h"
#include "aimlist.h"
#include "include/test.h"
#include "include/usctest.h"

/* TODO - make this cleaner */
char logfile[] = "multiuser.ss";
char csvfile[] = "multiuser.csv";
/* Cheap hack for STP, bwa-ha-ha-ha */
char stpmdata[] = "stp_mdata.txt";
char stpsdata[] = "stp_sdata.html";
char stphdata[] = "stp_hdata.html";
char maxjobs[] = "maxjobs.txt";
char *logfile_prefix_g;

/* separate function so we can call it various places
 */
long get_mhertz()
{
	long mhertz;
#ifndef HZ
	mhertz = sysconf(_SC_CLK_TCK);
	if (mhertz == -1) {
		fprintf(stderr, "Cannot get HZ\n");
		exit(1);
	}
#else
	mhertz = HZ;
#endif
	return mhertz;
}

/* create a linked list */
void aiml_init(struct _aimList *list)
{
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
	return;
}

/* linked list insert */
int aiml_insert(struct _aimList *list, struct Cargs *myargs,
		struct Result *res, int hits)
{
	struct _aimlItem *new_item;
	struct _aimlItem *tmp;
	if ((new_item =
	     (struct _aimlItem *) malloc(sizeof(struct _aimlItem))) ==
	    NULL)
		return -1;
	new_item->myargs = myargs;
	new_item->res = res;
	new_item->tot_time = 0;
	new_item->max_time = 0;
	new_item->count = 0;
	new_item->hits = hits;
	new_item->next = NULL;
	if (list->head == NULL) {	/* insert at head */
		list->head = new_item;
		list->tail = new_item;
	} else {
		tmp = list->tail;
		tmp->next = new_item;
		list->tail = new_item;
	}


	list->size++;
	return 0;
}

/* linked list removal */
int aiml_remove(struct _aimList *list, struct _aimlItem *item, int debug)
{
	struct _aimlItem *tmpi;
	struct _aimlItem *tmpb;
	struct _aimlItem *prev;
	double avg;
	char dbug[256];

	tmpi = list_head(list);
	tmpb = list_tail(list);

	prev = NULL;
	for (tmpi = list_head(list); tmpi != NULL; tmpi = tmpi->next) {
		if (tmpi == item) {
			if (prev == NULL) {
				list->head = tmpi->next;
			} else {
				prev->next = tmpi->next;
			}
			if (tmpb == item) {
				list->tail = tmpi;
			}
			if ((tmpi->count > 0) && (debug)) {
				avg =
				    (double) tmpi->tot_time / tmpi->count;
				sprintf(dbug,
					"Func %s called %d times avg: %6.3f max %ld",
					tmpi->myargs->name, tmpi->count,
					avg, tmpi->max_time);
				write_debug_file(dbug);
			}
			free(tmpi->myargs);
			free(tmpi->res);

			free(tmpi);
			list->size--;
			return 0;
		}
		prev = tmpi;
	}
	fprintf(stderr, "failed to remove item");
	return 1;
}
int write_debug_file(char *debug_msg)
{
	FILE *dptr;

	if ((dptr = fopen("./reaim.debuglog", "a")) == NULL) {
		perror("debug log");
		fprintf(stderr, "Failed to open logfile ./reaim.debuglog\n");
		return 1;
	}
	fprintf(dptr, "%s\n", debug_msg);
	fflush(NULL);
	fclose(dptr);
	return 0;
}

char *ext_strcat(char *s1, char *s2)
{
	char *stmp = (char*)malloc(strlen(s1)+strlen(s2));
	stmp[0] = '\0';
	strcat(stmp,s1);
	strcat(stmp,s2);

	return stmp;
}

void write_csv_header()
{
	FILE *cptr;
	char *s;

	s = ext_strcat(logfile_prefix_g,".csv");
	
	if ((cptr = fopen(s, "w")) == NULL) {
		perror("driver");
		fprintf(stderr, "Failed to open csvfile %s\n", s);
		exit(1);
	}
	fprintf(cptr,
		"Forks,JPM,JPM_C,JPS_C,parent_tm,childU_tm,childS_tm,std_dev,JTI,max_c,min_c\n");
	fflush(NULL);
	fclose(cptr);
	free(s);
}
void write_csv_out(int forks, double jpm, double jps, double ptime,
		   double cutck, double cstck, double std_def, double jti,
		   double max, double min)
{
	FILE *cptr;
	double tpmu = jps * 60.0;
	char *s;
	
	s = ext_strcat(logfile_prefix_g,".csv");
	
	if ((cptr = fopen(s, "a")) == NULL) {
		perror("driver");
		fprintf(stderr, "Failed to open csvfile %s\n", s);
		exit(1);
	}
	fprintf(cptr,
		"%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
		forks, jpm, tpmu, jps, ptime, cutck, cstck, std_def, jti,
		max, min);
	fflush(NULL);
	fclose(cptr);
	free(s);

}
void write_file_header(char *suite_time)
{
	FILE *lptr;

		char *s;
	
	s = ext_strcat(logfile_prefix_g,".log");
	
	if ((lptr = fopen(s, "w")) == NULL) {
  		perror("driver");
		fprintf(stderr, "Failed to open logfile %s\n", s);
		exit(1);
	}
	fprintf(lptr, "Re-AIM Version 0.1.0 Run Date %s\n", suite_time);
	fprintf(lptr, "Times are in seconds. \n");
	fprintf(lptr, "ChildU time is cutime from the parent\n");
	fprintf(lptr, "ChildS time is cstime from the parent\n");
	fprintf(lptr, "Send me a patch for a nicer header\n\n");
	fprintf(lptr,
		"                 Jobs/min/ Jobs/sec/ Time:   Time:  Time:   Time:           Running child time\n");
	fprintf(lptr,
		"Forks  Jobs/min   child     child parent  childU childS  std_dev   JTI   :max  :min\n");
	fflush(NULL);
	fclose(lptr);
	free(s);
}
void write_stp_sheader(char *stime, char *etime)
{
	FILE *stp;

	if ((stp = fopen(stpsdata, "w")) == NULL) {
		perror("stpsdata");
		fprintf(stderr, "Failed to open stp data file %s\n",
			stpsdata);
		exit(1);
	}
	fprintf(stp,
		"<table cellpadding=\"2\" cellspacing=\"2\" border=\"1\" width=\"100%%\">\n<tbody>");
	fprintf(stp,
		"<tr><th colspan=\"6\">Re-aim SingleUser Run Report</th>\n");
	fprintf(stp, "<tr><th colspan=\"6\">Starting time: %s</th>\n",
		stime);
	fprintf(stp,
		"<tr><th colspan=\"6\">Projected finish time: %s</th>\n",
		etime);
	fclose(stp);
}
void close_stp_single(char *stime, char *etime)
{
	FILE *stp;

	if ((stp = fopen(stpsdata, "a")) == NULL) {
		perror("stpsdata");
		fprintf(stderr, "Failed to open stp data file %s\n",
			stpsdata);
		exit(1);
	}
	fprintf(stp,
		"<tr><th colspan=\"6\">Projected finish time: %s</th>\n",
		stime);
	fprintf(stp, "<tr><th colspan=\"6\">Actual finish time: %s</th>\n",
		etime);
	fprintf(stp, "</tbody></table>");
	fclose(stp);
}

void write_stp_mheader()
{
	FILE *stp;
	FILE *stph;

	if ((stph = fopen(stphdata, "w")) == NULL) {
		perror("stphdata");
		fprintf(stderr, "Failed to open stp html file %s\n",
			stphdata);
		exit(1);
	}
	fprintf(stph,
		"<table cellpadding=\"2\" cellspacing=\"2\" border=\"1\" width=\"100%%\">\n<tbody>");
	fprintf(stph,
		"<tr><th colspan=\"11\">Re-aim Multiuser Run Report</th>\n");
	fprintf(stph,
		"<tr><td>Children</td><td>Jobs per Minute</td><td>JPM per Child</td><td>Jobs/sec/child</td><td>Run Time</td><td>Child User Time</td><td>Child System Time</td><td>std dev</td><td>JTI</td><td>Max Child Time</td><td>Min Child Time</td></tr>\n");
	fclose(stph);

	if ((stp = fopen(stpmdata, "w")) == NULL) {
		perror("stpdmata");
		fprintf(stderr, "Failed to open stp data file %s\n",
			stpmdata);
		exit(1);
	}
	fprintf(stp, "Re-aim Multiuser Test Results\n");
	fclose(stp);
}
void write_stp_hout(int forks, double jpm, double jps, double ptime,
		    double cutck, double cstck, double std_def, double jti,
		    double max, double min)
{
	FILE *stph;
	double tpmu = jps * 60.0;

	if ((stph = fopen(stphdata, "a")) == NULL) {
		perror("stphdata");
		fprintf(stderr, "Failed to open stphdata %s\n", stphdata);
		exit(1);
	}
	fprintf(stph,
		"<tr><td>%-5d</td><td>%8.2f</td><td>%9.2f</td><td>%9.2f</td><td>%8.2f</td><td>%6.2f</td><td>%6.2f</td><td>%8.2f</td><td>%6.2f</td><td>%5.2f</td><td>%5.2f</td></tr>\n",
		forks, jpm, tpmu, jps, ptime, cutck, cstck, std_def, jti,
		max, min);
	fflush(NULL);
	fclose(stph);


}
void close_stp_hout(double jmax)
{
	FILE *stph;
	FILE *maxj;

	if ((maxj = fopen(maxjobs, "w")) == NULL) {
		perror("maxjobs");
		fprintf(stderr, "Failed to open maxjobs %s\n", maxjobs);
		exit(1);
	}
	if ((stph = fopen(stphdata, "a")) == NULL) {
		perror("stphdata");
		fprintf(stderr, "Failed to open stphdata %s\n", stphdata);
		exit(1);
	}
	fprintf(stph,
		"<tr><th colspan=\"11\">Peak Jobs per Minute: %.2f</th></tr>\n",
		jmax);
	fprintf(stph, "</tbody></table>\n");
	fprintf(maxj, "%.2f\n", jmax);
	fclose(stph);
	fclose(maxj);
}


void write_file_out(int forks, double jpm, double jps, double ptime,
		    double cutck, double cstck, double std_def, double jti,
		    double max, double min)
{
	FILE *lptr;
	double tpmu = jps * 60.0;

	if ((lptr = fopen(logfile, "a")) == NULL) {
		perror("driver");
		fprintf(stderr, "Failed to open logfile %s\n", logfile);
		exit(1);
	}
	fprintf(lptr,
		"%-5d %8.2f %9.2f %9.2f %8.2f  %6.2f %6.2f%8.2f  %6.2f %5.2f %5.2f\n",
		forks, jpm, tpmu, jps, ptime, cutck, cstck, std_def, jti,
		max, min);
	fflush(NULL);
	fclose(lptr);


}

void write_stp_mout(int num_children, double real_time, double tpm,
		    double tpm_per_user, int jti)
{
	FILE *stp;

	if ((stp = fopen(stpmdata, "a")) == NULL) {
		perror("stpmdata");
		fprintf(stderr, "Failed to open stpmdata %s\n", stpmdata);
		exit(1);
	}
	fprintf(stp, "%d %.2f %.2f %.2f %d\n", num_children, real_time,
		tpm, tpm_per_user, jti);
	fflush(NULL);
	fclose(stp);

}

void print_usage()
{
	printf("\tREAIM Workload\n");
	printf("\tOptions:\n");
	printf
	    ("\t-d<x>, --debug<x>\tTurns on debugging output - 1 is default\n");
	printf("\t-v, --verbose\t\tProduces more output\n");
	printf("\t-s<x>,--startusers<x>\tNumber of users to start with\n");
	printf("\t-e<x>,--endusers<x>\tNumber of users to end with\n");
	printf
	    ("\t-i<x>, --increment<x>\tNumber of users to increment by\n");
	printf
	    ("\t-f<s>, --file<s>\tWorkfile name (default '/usr/local/share/reaim/workfile')\n");
	printf("\t-l<s>, --logprefix<s>\tPrefix for logfile (default 'reaim')\n");
	printf
	    ("\t-c<s>, --config<s>\tDisk Config file name (default '/usr/local/share/reaim.config')\n");
	printf
	    ("\t-x, --crossover\t\tRun to crossover (jpm/user < 1.0) \n");
	printf
	    ("\t-q,--quick\t\tRun to quick crossover (jpm/user < 60.0) \n");
	printf
	    ("\t-j<x>,--jobs<x>\tNumber of jobs in tasklist (min is workfile size)\n");
	printf("\t-m, --multiuser\t\tAIM7 style - default\n");
	printf
	    ("\t-t, --timeroff\t\tDefeats adaptive timer for crossover run\n");
	printf
	    ("\t-o, --oneuser\t\tRuns AIM9 style - single thread, each test runs for 10 seconds\n");
	printf
	    ("\t-r<x>, --repeat<x>\t\tRepeats the test <x> number of times, default 1\n");
	printf
	    ("\t-p<x>, --period<x>\t\tNumber of seconds to run each test in singleuser mode\n");
	printf("\t-g , --guesspeak\t\t Run until cheezy maxjobs algorhythm detects JPM rate dropping\n");
	printf("\t-Z\t\t Specify an external monitoring tool to run in 4 phases (setup, begin, halt, report)\n");
	printf("\t-z\t\t Specify a command line for the external monitoring tool. It will be called as [tool] begin [cmd line]\n");
	printf("\t-h, --help\tThis message\n");
	printf("Bug reports to cliffw@osdl.org\n");
}


void print_header_line()
{
	/* this is in a separate function for maintainability */
	(void)
	    printf("REAIM Workload\n");
	printf
	    ("Times are in seconds - Child times from tms.cstime and tms.cutime\n\n");
	printf
	    ("Num     Parent   Child   Child  Jobs per   Jobs/min/  Std_dev  Std_dev  JTI\n");
	printf
	    ("Forked  Time     SysTime UTime   Minute     Child      Time     Percent \n");
}


void error_exit(int problem)
{
	if (problem == ERR_BADLINE) {
		(void) fprintf(stderr, "Error in Workfile\n");
		(void) fprintf(stderr,
			       "Specify either a weighted test line:\n");
		(void) fprintf(stderr, "<weight> <test name>\n");
		(void) fprintf(stderr,
			       "Or the file or pool size in kbytes or megabytes:\n");
		(void) fprintf(stderr, "FILESIZE: <integer><KkMm>\n");
		(void) fprintf(stderr, "POOLSIZE: <integer><KkMm>\n");
		exit(1);	/* die */
	}
	if (problem == ERR_BADSIZE) {
		(void) fprintf(stderr, "Error in workfile \n");
		(void) fprintf(stderr,
			       "Specify the file or pool size in kbytes or megabytes:\n");
		(void) fprintf(stderr, "FILESIZE: <integer><KkMm>\n");
		(void) fprintf(stderr, "POOLSIZE: <integer><KkMm>\n");
		exit(1);	/* die */
	}
	if (problem == ERR_BADFILE) {
		(void) fprintf(stderr,
			       "Missing or unreadable workfile, exiting\n");
		exit(1);
	}
	if (problem == ERR_BADCONFIG) {
		(void) fprintf(stderr, "Error in config file, exiting\n");
		exit(1);
	}
}

/* test to see if the workfile item is legal
 * we return false, true to the found_test arg
 */
int check_name(char *s, int hits)
{
	struct _aimlItem *tmp;

	for (tmp = global_list->head; tmp != NULL; tmp = tmp->next) {
		if (strcmp(s, tmp->myargs->name) == 0) {
			tmp->hits = hits;
			return 1;
		}
	}
	(void) fprintf(stderr,
		       "find_arguments: can't find <%s> in testlist, check workfile\n",
		       s);
	return 0;
}

int adjust_adaptive_timer(struct runloop_input *rl,
			  struct input_params *inv, int cnt)
{
	double tmp, tmp2, a_tn_avg;


	tmp2 = (double) inv->incr;
	/* TODO add Newton switch, minimun number of samples */
	if (cnt > 8) {
		if (((double) (rl->tpm - rl->runnum) /
		     (double) rl->runnum) <= 0.10) {
			tmp2 = (double) (rl->tpm - rl->runnum) / 2.0;
			if (tmp2 < 1.0)
				tmp2 = 1.0;
		} else {
			a_tn_avg = (rl->a_tn + rl->a_tn1) / 2.0;
			if (a_tn_avg == 0.0) {
				a_tn_avg = 1.0;
			}
			if (rl->a_tn == 0.0) {
				rl->a_tn = 1.0;
			}
			tmp = rl->a_tn1 / rl->a_tn;
			if (tmp < THRESHOLD_MIN) {
				tmp2 *= (2.0 / (rl->a_tn / a_tn_avg));
			} else if (tmp > THRESHOLD_MAX) {
				tmp2 /= (2.0 / (rl->a_tn / a_tn_avg));
			}
			if (tmp2 < 1.0) {
				tmp2 = (double) inv->incr;
			}
		}
	}
	return ((int) tmp2);
}

/* delta is in milliseconds */
int output_stp(struct _aimlItem *item, long delta, int count, int testnum)
{
	double rate, ops;
	char buffer[200];
	FILE *stp;

	if ((stp = fopen(stpsdata, "a")) == NULL) {
		perror("stpsdata");
		fprintf(stderr, "Failed to open stp data file %s\n",
			stpsdata);
		exit(1);
	}
	rate = 1000.0 * (double) count / delta;
	ops = rate * (double) item->myargs->factor;

	if (item->myargs->factor >= 0) {
		sprintf(buffer, "%15.2f %s/second", ops,
			item->myargs->units);
	} else {
		sprintf(buffer, "<Uncalibrated> %s/second",
			item->myargs->units);
	}


	fprintf(stp,
		"<tr><td>%d</td><td>%s</td><td>%.2f</td><td>%d</td><td>%.2f</td><td>%s</td></tr>\n",
		testnum, item->myargs->name, delta / 1000.0, count, rate,
		buffer);
	fclose(stp);	/* MARCIA: changed close to fclose - wrong calling
			 * sequence for close. */

	return 0;
}
int output_result(struct _aimlItem *item, long delta, int count,
		  int testnum, FILE * ss)
{
	double rate, ops;
	char buffer[200];

	rate = 1000.0 * (double) count / delta;
	ops = rate * (double) item->myargs->factor;

	if (item->myargs->factor >= 0) {
		sprintf(buffer, "%15.2f %s/second", ops,
			item->myargs->units);
	} else {
		sprintf(buffer, "<Uncalibrated> %s/second",
			item->myargs->units);
	}

	/* TODO
	   if (!no_logfile) {
	   fprintf(logfile,"%s %g %g %s\n", test, delta, rate, buffer);        
	   fflush(logfile);                                            
	   }
	 */

	printf("%-6d %-10s %6.2f %8d   %10.2f     %s\n",
	       testnum, item->myargs->name, delta / 1000.0, count, rate,
	       buffer);
	if (ss != NULL) {
		fprintf(ss, "%-6d %-10s %6.2f %8d  %10.2f     %s\n",
			testnum, item->myargs->name, delta / 1000.0, count,
			rate, buffer);
	}
	fflush(ss);


	return 0;
}

/* sthis is a total, brutal hack, but i'm in a hurry 
 * type one is singleuser 
 * type two is multiuser 
 *  Files to clean up 
 *  multiuser.ss
 *  multiuser.csv
 *  singleuser.ss
 *  */
void repeat_fix_results(int repeat, int type, int brief)
{
	char m1_buf[256];

	if (type == 1) {
		sprintf(m1_buf, "mv singleuser.ss singleuser.%d.ss",
			repeat);
		system(m1_buf);
		if (brief) {
			sprintf(m1_buf,
				"mv stp_sdata.html stp_sdata.%d.html",
				repeat);
			system(m1_buf);
		}
	} else {
		sprintf(m1_buf, "mv multiuser.ss multiuser.%d.ss", repeat);
		system(m1_buf);
		sprintf(m1_buf, "mv multiuser.csv multiuser.%d.csv",
			repeat);
		system(m1_buf);
		if (brief) {
			sprintf(m1_buf,
				"mv stp_mdata.txt stp_mdata.%d.txt",
				repeat);
			system(m1_buf);
			sprintf(m1_buf,
				"mv stp_hdata.html stp_hdata.%d.html",
				repeat);
			system(m1_buf);
			sprintf(m1_buf, "mv maxjobs.txt maxjobs.%d.txt",
				repeat);
			system(m1_buf);
		}
	}

}
