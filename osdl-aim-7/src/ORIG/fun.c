
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

#include <stdio.h>		/* declare print(), etc. */
#include "suite.h"
#include "funcal.h"

static int fun_cal(), fun_cal1(), fun_cal2(), fun_cal15();

int *p_i1;
long *p_fcount;

source_file *fun_c()
{
	static source_file s = { " @(#) fun.c:1.3 7/26/93 16:55:53",	/* SCCS info */
		__FILE__, __DATE__, __TIME__
	};

	register_test("fun_cal", 2, "1 0", fun_cal, 512000,
		      "Function Calls (no arguments)");
	register_test("fun_cal1", 2, "1 0", fun_cal1, 512000,
		      "Function Calls (1 argument)");
	register_test("fun_cal2", 2, "1 0", fun_cal2, 512000,
		      "Function Calls (2 arguments)");
	register_test("fun_cal15", 2, "1 0", fun_cal15, 512000,
		      "Function Calls (15 arguments)");

	return &s;
}

/*
 **  FUNCTION CALLER
 **		fcall0, fcal0 -> funcall 0 args
 **		fcall1, fcal1 -> funcall 1 arg
 **		fcall2, fcal2 -> funcall 2 args
 **		fcall15, fcal15 -> funcall 15 args
 **
 **	Functions are called with various permutations of no-, one-, two- 
 **	and thirty one-parameters.  Each function is called n*512 times
 **	for the drill.
 **
 ** 3/12/90 Tin Le
 **	- Feedbacks indicated that very few 'real world' programs out
 **	there really uses function calls with 31 args; the suggested
 **	maximum is between 10-15.  In looking over X source and a number
 **	of 'typical' SunView programs, I picked 15 as a reasonable 'max'
 **	number of args a function might use.
 **	- As a side note, this should make RISC people a little bit
 **	happier (especially the ones with register windows).
 */


/*
 *      fun_cal
 */
static int fun_cal(int c, char **argv, struct Result *res)
{
	int
	 (*p_fcal0) (),		/* function to call */
	 i,			/* inside loop variable */
	 n,			/* outside loo variable */
	 i1;			/* input parameter */

	long fcount;		/* number of iterations? */

	COUNT_START;

	if (sscanf(*argv, "%d %ld", &i1, &fcount) < 2) {	/* get args */
		fprintf(stderr, "fun_cal(): needs 2 arguments!\n");	/* handle error */
		return (-1);	/* return failure */
	}

	p_i1 = &i1;		/* point to input value */
	p_fcount = &fcount;	/* point to count */

	if (i1)			/* if calling fcal0 */
		p_fcal0 = fcal0;	/* point to it */
	else			/* (not taken) */
		p_fcal0 = fcalfake;	/* fool compilers */

	n = 1000;		/* initialize outside ocunt */
	while (n--)		/* loop this many times */
		for (i = 32; i--;) {	/* internal loop */
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
			(*p_fcal0) ();
			COUNT_BUMP;
		}
	res->l = fcount;	/* return results */
	COUNT_END("fun_cal");
	return (0);		/* return success */
}


/*
 *      fun_cal1
 */
static int fun_cal1(int c, char **argv, struct Result *res)
{
	int i,			/* internal loop variable */
	 n,			/* external loop variable */
	 i1,			/* argument */
	 (*p_fcal1) ();		/* function pointer */

	long fcount;		/* count */

	COUNT_START;

	if (sscanf(*argv, "%d %ld", &i1, &fcount) < 2) {	/* get args */
		fprintf(stderr, "fun_cal1(): needs 2 arguments!\n");	/* handle errors */
		return (-1);	/* return failure */
	}

	p_i1 = &i1;		/* point to arg */
	p_fcount = &fcount;	/* point to other arg */

	if (i1)
		p_fcal1 = fcal1;	/* if ok, point to function */
	else
		p_fcal1 = fcalfake;	/* should not happen */

	n = 1000;		/* initialize count */
	while (n--)		/* loop through it */
		for (i = 32; i--;) {	/* internal loop */
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
			(*p_fcal1) (n);
			COUNT_BUMP;
		}
	res->l = fcount;	/* return results */
	COUNT_END("fun_cal1");
	return (0);		/* return success */
}

/*
 *      fun_cal2
 */
static int fun_cal2(int c, char **argv, struct Result *res)
{
	int
	 (*p_fcal2) (),		/* pointer to function */
	 i,			/* internal loop count */
	 n,			/* external loop count */
	 i1;			/* argument */

	long fcount;		/* argument */

	COUNT_START;

	if (sscanf(*argv, "%d %ld", &i1, &fcount) < 2) {	/* get arguments */
		fprintf(stderr, "fun_cal2(): needs 2 arguments!\n");	/* handle errors */
		return (-1);	/* return failure */
	}

	p_i1 = &i1;		/* point to arguments */
	p_fcount = &fcount;

	if (i1)
		p_fcal2 = fcal2;	/* point to function */
	else
		p_fcal2 = fcalfake;	/* (shouldn't happen) */

	n = 1000;		/* initialize count */
	while (n--)		/* loop through it */
		for (i = 32; i--;) {	/* internal loop */
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
			(*p_fcal2) (&n, &i);
			COUNT_BUMP;
		}
	res->l = fcount;	/* assign results */
	COUNT_END("fun_cal2");
	return (0);		/* return success */
}

/*
 *      fun_cal15
 */
static int fun_cal15(int c, char **argv, struct Result *res)
{
	int
	 (*p_fcal15) (),	/* the function pointer */
	 i,			/* internal loop count */
	 n,			/* external loop count */
	 i1;			/* argument */

	long fcount;		/* argument */

	COUNT_START;

	if (sscanf(*argv, "%d %ld", &i1, &fcount) < 2) {	/* get args */
		fprintf(stderr, "fun_cal15(): needs 2 arguments!\n");	/* handle errors */
		return (-1);	/* return failure */
	}

	p_i1 = &i1;		/* point to args */
	p_fcount = &fcount;

	if (i1)
		p_fcal15 = fcal15;	/* point to function */
	else
		p_fcal15 = fcalfake;	/* (shouldn't happen) */

	n = 1000;		/* initialize count */
	while (n--) {		/* loop through it */
		for (i = 512; i--;) {	/* internal counter */
			(*p_fcal15) (&i, &i, &i, &i, &i, &i, &i, &i, &i, &i, &i, &i, &i, &i, &i);	/* call the funciton */
			COUNT_BUMP;
		}
	}			/* end of loop */
	res->l = fcount;	/* return value */
	COUNT_END("fun_cal15");
	return (0);		/* return success */
}
