#ifndef KEYWORD_H_
#define KEYWORD_H_

#include "lex.h"

struct keyword {
	char	*name;
	token_t	token;
};

void
keyword_table_create(void);
	
void
keyword_table_destroy(void);
	
struct keyword*
keyword_table_lookup(char *name);

#endif /*KEYWORD_H_*/ 
