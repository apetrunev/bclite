#ifndef LIST_H_
#define LIST_H_

struct list_of_val {
	struct list_of_val *prev;
	void	*val;
};

extern struct list_of_val *list;

void
push(void *arg);

void*
pop(void);

void
purge(void);

#endif /*LIST_H_*/
