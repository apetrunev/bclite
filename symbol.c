#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "symbol.h"
#include "macros.h"
#include "umalloc.h"

#define DIR_LEN 1024

static struct symbol_table *global;
static struct symbol_table *top;

static struct symbol *ans;

static void symbol_init_ans(void);

static int
symbol_strcmp(void *a, void *b)
{
	int ret;

	return_val_if_fail(a != NULL, ret_invalid);
	return_val_if_fail(b != NULL, ret_invalid);

	ret = strcmp((char *)a, (char *)b);

	return ret;
}

static int
symbol_strhash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

static struct symbol_table*
create_table(void)
{
	struct symbol_table *table;
	
	table = umalloc0(sizeof(*table));
	
	table->scope =  hash_table_new(0, (hash_callback_t)symbol_strhash, (hash_compare_t)symbol_strcmp);

	if (table->scope == NULL)
		error(1, "can't create a scope");
	
	return table;
}

void
symbol_table_create_global(void)
{
	struct symbol_table *table;
	
	table = create_table();
	
	global = top = table;
	/* init special variable */
	symbol_init_ans();
}

void
symbol_table_global_put_symbol(struct symbol *symbol)
{
	ret_t ret;
	
	return_if_fail(symbol != NULL);

	ret = hash_table_insert_unique(global->scope, symbol->name, symbol);
	
	if (ret != ret_ok)
		error(1, "symbol insert fail");
}

struct symbol*
symbol_table_lookup_top(char *name)
{
	struct symbol *symbol;
	ret_t ret;

	return_val_if_fail(name != NULL, NULL);
	
	if (!global)
		return NULL;
	
	ret = hash_table_lookup(top->scope, (void *)name, (void **)&symbol);
	
	if (ret != ret_ok)
		return NULL;
	
	return symbol;
}

struct symbol*
symbol_table_lookup_all(char *name)
{
	struct symbol *symbol;
	struct symbol_table *table;
	ret_t ret;
	
	return_val_if_fail(name != NULL, NULL);
	
	if (!global)
		return NULL;
		
	for (table = top; table != NULL; table = table->prev) {
		ret = hash_table_lookup(table->scope, (void *)name, (void **)&symbol);
	
		if (ret == ret_ok)
			return symbol;
	}
	
	return NULL;
}

struct symbol_table*
symbol_table_get_current_table(void)
{
	return_val_if_fail(top != NULL, NULL);
	return_val_if_fail(global != NULL, NULL);
	
	return top;
}

void
symbol_table_push(void)
{
	struct symbol_table *table;
	
	return_if_fail(global != NULL);

	table = create_table();	
	
	table->prev = top;
	
	top = table;
}

void
symbol_table_pop(void)
{
	return_if_fail(global != NULL);
	
	if (!top->prev)
		error(1, "pop global table");
	
	top = top->prev;
}

void
symbol_table_put_symbol(struct symbol *symbol)
{
	ret_t ret;
	
	return_if_fail(symbol != NULL);

	ret = hash_table_insert_unique(top->scope, symbol->name, symbol);
	
	if (ret != ret_ok)
		error(1, "symbol insert fail");
}

void
symbol_table_set_scope(struct symbol_table *scope)
{
	struct symbol_table *prev;

	return_if_fail(scope != NULL);
	
	prev = top;
	
	top  = scope;
	
	top->prev = prev;
}

void
symbol_table_destroy(struct symbol_table **table)
{
	struct symbol *symbol;
	struct hash_table_iter *iter;
	char *name;

	return_if_fail(table != NULL);
	
	iter = hash_table_iterate_init((*table)->scope);

	if (!iter)
		error(1, "hash iterator");
	
	while (hash_table_iterate(iter, (void **)&name, (void **)&symbol))
		symbol_destroy(symbol);
	
	hash_table_iterate_deinit(&iter);
	hash_table_destroy(&(*table)->scope);

	ufree(*table);
	(*table) = NULL;		
}

static void
symbol_free(struct symbol *symbol)
{
	return_if_fail(symbol != NULL);

	ufree(symbol->name);

	switch(symbol->v_type) {
	case VALUE_TYPE_STRING:
		ufree(symbol->string);
		break;
	case VALUE_TYPE_VECTOR:
		gsl_vector_free(symbol->vector);
		break;
	case VALUE_TYPE_MATRIX:
		gsl_matrix_free(symbol->matrix);
		break;
	default:
		break;
	}
	
	ufree(symbol);
}

struct symbol*
symbol_new(char *name, value_t v_type)
{
	struct symbol *sym;

	return_val_if_fail(name != NULL, NULL);
	
	sym = umalloc0(sizeof(*sym));
	
	switch(v_type) {
	case VALUE_TYPE_UNKNOWN:
	case VALUE_TYPE_DIGIT:
	case VALUE_TYPE_STRING:
	case VALUE_TYPE_VECTOR:
	case VALUE_TYPE_MATRIX:
		sym->name       = ustrdup(name);
		sym->v_type     = v_type;
		sym->destructor = symbol_free;
		break;
	default:
		error(1, "wrong value type");
	}
	
	return sym;
}

static void
symbol_clean_val(struct symbol *symbol)
{
	return_if_fail(symbol != NULL);
	
	switch(symbol->v_type) {
	case VALUE_TYPE_STRING:
		ufree(symbol->string);
		break;
	case VALUE_TYPE_VECTOR:
		gsl_vector_free(symbol->vector);
		break;
	case VALUE_TYPE_MATRIX:
		gsl_matrix_free(symbol->matrix);
		break;
	default:
		break;
	}
}

void
symbol_set_val(struct symbol *symbol, value_t v_type, void *val)
{
	gsl_vector *vc;
	gsl_matrix *mx;

	return_if_fail(symbol != NULL);
	return_if_fail(val != NULL);
	/* clean previous value*/
	symbol_clean_val(symbol);
	
	switch(v_type) {
	case VALUE_TYPE_DIGIT:
		symbol->v_type = v_type;
		symbol->digit  = *(double *)val;
		break;
	case VALUE_TYPE_STRING:
		symbol->v_type = v_type;
		symbol->string = ustrdup((char *)val);
		break;
	case VALUE_TYPE_VECTOR:
		symbol->v_type = v_type;
		vc = (gsl_vector *)val;
		symbol->vector = gsl_vector_alloc(vc->size);
		gsl_vector_memcpy(symbol->vector, vc);
		break;
	case VALUE_TYPE_MATRIX:
		symbol->v_type = v_type;
		mx = (gsl_matrix *)val;
		symbol->matrix = gsl_matrix_alloc(mx->size1, mx->size2);
		gsl_matrix_memcpy(symbol->matrix, mx);
		break;
	default:
		error(1, "wrong value type");
	}
}

static void
symbol_init_ans(void)
{
	char dname[DIR_LEN];

	ans = symbol_new("ans", VALUE_TYPE_UNKNOWN);
	
	getcwd(dname, DIR_LEN);
	
	symbol_set_val(ans, VALUE_TYPE_STRING, dname);
	
	symbol_table_global_put_symbol(ans);
}

struct symbol*
symbol_get_ans(void)
{
	return ans;
}

void
symbol_destroy(struct symbol *symbol)
{
	return_if_fail(symbol != NULL);
	
	symbol->destructor(symbol);
}


