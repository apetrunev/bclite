#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "macros.h"
#include "umalloc.h"

struct list_of_val *list;

void
push(void *arg)
{
	struct list_of_val *tmp;
		
	tmp = umalloc0(sizeof(*tmp));

	tmp->val  = arg;
	tmp->prev = list;
	list      = tmp;	
}

void*
pop(void)
{
	void *ret;
	struct list_of_val *tmp;

	return_val_if_fail(list != NULL, NULL);
	
	ret   = list->val;
	tmp   = list;
	list  = list->prev;
	
	ufree(tmp);

	return ret;	
}

void
purge(void)
{
	struct list_of_val *prev;
	
	for  (;list != NULL; list = prev) {
		prev = list->prev;
		ufree(list);
	}
}
