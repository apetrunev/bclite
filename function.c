#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "function.h"
#include "macros.h"
#include "common.h"
#include "hash.h"
#include "umalloc.h"
#include "libcall.h"
#include "misc.h"

#define err_msg_ret(ret, fmt, arg...) \
do { \
	messsge(fmt, ##arg); \
	return(ret); \
} while(0)

#define err_msg(fmt, arg...) \
do { \
	message(fmt, ##arg); \
} while(0)
	
static struct hash_table *function_table;

static void function_init_lib(void);

/* names for functions arguments */
static char *names[] = {
	"x1",
	"x2",
	"x3",	
	"x4",	
	"x5",
	"x6",
	"x7",
	"x8",
	"x9",
	"x10",
	NULL
};

static struct {
	char			*name;
	unsigned int		nargs;
	lib_handler_type_t	handler;
} functions[] = {
	{ "sin",	1,	libcall_sin  },
	{ "cos",	1,	libcall_cos  },
	{ "exp",	1,	libcall_exp  },
	{ "ln",		1,	libcall_ln  },
	{ "tan",	1,	libcall_tan },
	{ "matrix",	2,	libcall_matrix },
	{ "vector",	1,	libcall_vector },
	{ "sqrt",	1,	libcall_sqrt },
	{ NULL,		-1,	NULL }
};

static int
function_hash(unsigned char *name)
{
	unsigned long hash = 5381;
	int c;
		
	while ((c = *name++) != 0)
		hash = ((hash << 5) + hash) + c;
	
	return hash;
}

static int
function_strcmp(const void *a, const void *b)
{
	int ret;
		
	ret = strcmp((char *)a, (char *)b);
	
	return ret;
}

void
function_table_create(void)
{
	function_table = hash_table_new(0,(hash_callback_t)function_hash,
					(hash_compare_t)function_strcmp);
	if (function_table == NULL)
		error(1, "can't create function table");
	
	function_init_lib();
}

struct function*
function_table_lookup(char *name)
{
	struct function *func = NULL;
	ret_t ret;

	return_val_if_fail(name != NULL, NULL);

	ret = hash_table_lookup(function_table, name, (void **)&func);
	
	return func;
}

int
function_table_insert(struct function *function)
{
	ret_t ret;

	return_val_if_fail(function_table != NULL, -1);
	return_val_if_fail(function != NULL, -1);
	
	ret = hash_table_insert_unique(function_table, function->name, function);
	
	if (ret != ret_ok)
		error(1, "insert in function table fail");	
	
	return ret;	
}

struct function*
function_new(char *name)
{
	struct function *func;

	return_val_if_fail(name != NULL, NULL);
	
	func = umalloc0(sizeof(*func));
	
	func->name   = ustrdup(name);
	func->is_lib = FALSE;

	return func;	
}

void
function_destroy(struct function *func)
{
	return_if_fail(func != NULL);
	
	ufree(func->name);

	if (func->args)
		ufree(func->args);

	if (func->scope)
		symbol_table_destroy(&func->scope);
	
	if (func->body)
		ast_node_unref(func->body);

	ufree(func);
}

void
function_add_arg(struct function *function, struct symbol *arg)
{
	int i;

	return_if_fail(function != NULL);	
	
	i = function->nargs++;
	
	function->args = urealloc(function->args,
		function->nargs * sizeof(*function->args));

	function->args[i] = arg;	
}

void
function_table_delete_function(char *name)
{
	struct function *func;
	ret_t ok;

	return_if_fail(name != NULL);

	func = function_table_lookup(name);
	
	if (!func) {
		err_msg("not entry for this function");
		return;
	}

	ok = hash_table_remove(function_table, func->name);

	if (!ok) {
		err_msg("no entry for this function");
		return;
	}

	function_destroy(func);
}

static void
function_init_args(struct function *lib, int nargs)
{
	struct symbol *symbol;
	int i;
	
	return_if_fail(lib != NULL);

	for (i = 0; i < nargs; i++) {
		symbol = symbol_new(names[i], VALUE_TYPE_UNKNOWN);
	
		function_add_arg(lib, symbol);
	}	
}

static void
function_init_lib(void)
{	
	struct function *lib;
	ret_t ret;
	int i;

	for (i = 0; functions[i].name != NULL; i++) {
		lib = umalloc0(sizeof(*lib));
		
		lib->is_lib  = TRUE;
		lib->name    = ustrdup(functions[i].name);
		lib->handler = functions[i].handler;
		
		function_init_args(lib, functions[i].nargs);

		ret = hash_table_insert_unique(function_table, lib->name, lib);
	
		if (ret != ret_ok)
			error(1, "init lib failed");		
	}	
}	
