#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keyword.h"
#include "hash.h"
#include "macros.h"
#include "common.h"

static struct hash_table *keyword_table;

struct keyword keywords[] = {
	{ "function",	TOKEN_FUNCTION },
	{ "if",		TOKEN_IF },
	{ "else",	TOKEN_ELSE },
	{ "for",	TOKEN_FOR },
	{ "while",	TOKEN_WHILE },
	{ "return",	TOKEN_RETURN },
	{ "break",	TOKEN_BREAK },
	{ "continue",	TOKEN_CONTINUE },
	{ "local",	TOKEN_LOCAL },
	{ "include",	TOKEN_INCLUDE },
	{ NULL,		TOKEN_UNKNOWN }
};

static int
keyword_hash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;

	return hash;
}

static int
keyword_strcmp(void *a, void *b)
{
	int ret;
	
	return_val_if_fail(a, ret_invalid);
	return_val_if_fail(b, ret_invalid);

	ret = strcmp((char *)a, (char *)b);

	return ret;
}

void
keyword_table_create(void)
{
	int i;
	keyword_table = hash_table_new(0, (hash_callback_t)keyword_hash,
					(hash_compare_t)keyword_strcmp);
	if (keyword_table == NULL)
		error(1, "no memory for keyword table");
	
	for (i = 0; keywords[i].token != TOKEN_UNKNOWN; i++)
		hash_table_insert_unique(keyword_table, keywords[i].name,
							&keywords[i]);
	
}

void
keyword_table_destroy(void)
{
	return_if_fail(keyword_table != NULL);
	
	hash_table_clean(keyword_table);
	hash_table_destroy(&keyword_table);
}

struct keyword*
keyword_table_lookup(char *name)
{
	struct keyword *kw;
	ret_t ret;
	
	return_val_if_fail(keyword_table != NULL, NULL);
	
	ret = hash_table_lookup(keyword_table, name, (void**)&kw);
	if (ret != ret_ok)
		return NULL;

	return kw;
}
