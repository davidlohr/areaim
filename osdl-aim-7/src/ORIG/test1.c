/*  A first test file 
 * @(#) $Id$ $Date$ $Author$
 */


#include <stdio.h>		/* enable printf(), etc. */
#include <stdlib.h>		/* enable exit(), etc. */
#include <unistd.h>

#include "suite.h"		/* standard includes */

static int test1(int argc, char **argv, struct Result *res);

source_file *test1_c()
{
	static source_file s = { "@(#) test1.c:1.0 2/15/03 23:00:00",
		__FILE__, __DATE__, __TIME__
	};
	register_test("test1", 1, "Hi_There", test1, 100, "Dummy test");
	return &s;
}


static int test1(int argc, char **argv, struct Result *res)
{
	/* printf("Input was %s \n", argv[0]);
	 */
	res->d++;
	return 0;


}
