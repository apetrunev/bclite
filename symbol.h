#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

#include "hash.h"
#include "common.h"

struct symbol;

struct symbol_table {
	struct symbol_table *prev;
	struct hash_table *scope;
	int count;	
};

typedef void (*release_t)(struct symbol* );

struct symbol {
	value_t			v_type;
	char			*name;
	union {
		double		digit;
		char		*string;
		gsl_vector	*vector;
		gsl_matrix	*matrix;		
	};

	release_t		destructor;
};

void
symbol_table_create_global(void);

void
symbol_table_global_put_symbol(struct symbol *symbol);

struct symbol*
symbol_table_lookup_top(char *name);

struct symbol*
symbol_table_lookup_all(char *name);

struct symbol_table*
symbol_table_get_current_table(void);

void
symbol_table_push(void);

void
symbol_table_pop(void);

void
symbol_table_put_symbol(struct symbol *symbol);

void
symbol_table_set_scope(struct symbol_table *scope);

void
symbol_table_destroy(struct symbol_table **table);


struct symbol*
symbol_new(char *name, value_t v_type);

void
symbol_set_val(struct symbol *symbol, value_t v_type, void *val);

struct symbol*
symbol_get_ans(void);

void
symbol_destroy(struct symbol *symbol);

#endif /* SYMBOL_H_ */
