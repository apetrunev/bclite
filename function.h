#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "symbol.h"
#include "as_tree.h"

struct function;

typedef int (*lib_handler_type_t)(struct function *, value_t *, void **);

struct function {
	char 			*name;
	unsigned int 		is_lib;
	unsigned int		nargs;
	struct symbol		**args;
	struct symbol_table	*scope;
	struct ast_node		*body;
	lib_handler_type_t 	handler;
};

void
function_table_create(void);

struct function*
function_table_lookup(char *name);

int
function_table_insert(struct function *function);

struct function*
function_new(char *name);

void
function_destroy(struct function *func);

void
function_add_arg(struct function *function, struct symbol *arg);
	
void
function_table_delete_function(char *name);

#endif /*FUNCTION_H_*/
