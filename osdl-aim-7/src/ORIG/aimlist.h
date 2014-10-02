/* header file to create linked list stuff 
 * I am stealing this straight from the book */
#ifndef AIMLIST_H
#define AIMLIST_H

#include <stdlib.h>

#define MAXFUNCNAME (50)	/* fear magic numbers */

struct _aimlItem {
	struct _aimlItem *next;
	struct Cargs *myargs;
	struct Result *res;
	long tot_time;
	long max_time;
	int count;
	int hits;
};

struct _aimList {
	struct _aimlItem *head;
	struct _aimlItem *tail;
	int size;
	int (*match) (const void *key1, const void *key2);
};

void aiml_init(struct _aimList *list);
int aiml_insert(struct _aimList *list, struct Cargs *cmdargs,
		struct Result *res, int hits);
int aiml_remove(struct _aimList *list, struct _aimlItem *item, int debug);


#define list_size(list) ((list)->size)
#define list_head(list) ((list)->head)
#define list_tail(list) ((list)->tail)
#define list_is_head(list, element) ((element) == (list)->head ? 1 : 0)

#define list_is_tail(element) ((element)->next == NULL ? 1 : 0)

#define get_Cargs(element) ((element)->myargs)
#define get_Result(element) ((element)->res)

#define get_next(element) ((element)->next)


#endif
