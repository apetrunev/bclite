#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

#include "traverse.h"
#include "macros.h"
#include "list.h"
#include "symbol.h"
#include "umalloc.h"
#include "function.h"
#include "misc.h"
#include "eval.h"
#include "lex.h"
#include "syntax.h"

typedef enum {
	RES_OK,
	RES_BREAK,
	RES_CONTINUE,
	RES_RETURN,
	RES_ERROR
} res_type_t;

struct loop_ctx {
	int is_break;
	int is_continue;
	int is_return;
};

extern struct list_of_val *list;

static int errors;

#define err_msg_ret(ret, fmt, arg...) \
do { \
	errors++; \
	message(fmt, ##arg); \
	return (ret); \
} while(0)
		
#define err_msg(fmt, arg...) \
do { \
	errors++; \
	message(fmt, ##arg); \
	return; \
} while(0)

static struct loop_ctx helper;

typedef void (* handler_type_t)(struct ast_node *);

static void traverse_op(struct ast_node *node);
static void traverse_const(struct ast_node *node);
static void traverse_id(struct ast_node *node);
static void traverse_assign(struct ast_node *node);
static void traverse_func_call(struct ast_node *node);
static void traverse_return(struct ast_node *node);
static void traverse_if(struct ast_node *node);
static void traverse_for(struct ast_node *node);
static void traverse_while(struct ast_node *node);
static void traverse_break(struct ast_node *node);
static void traverse_continue(struct ast_node *node);
static void traverse_empty(struct ast_node *node);
static void traverse_vector(struct ast_node *node);
static void traverse_matrix(struct ast_node *node);
static void traverse_access(struct ast_node *node);
static void traverse_root(struct ast_node *node);

struct {
	node_type_t	type;
	handler_type_t	handler;
} traverse_nodes[] = {
	{ NODE_TYPE_ADD_OP,	traverse_op },
	{ NODE_TYPE_MULT_OP,	traverse_op },
	{ NODE_TYPE_REL_OP,	traverse_op },
	{ NODE_TYPE_AND_OP,	traverse_op },
	{ NODE_TYPE_OR_OP,	traverse_op },
	{ NODE_TYPE_EXP_OP,	traverse_op },	
	{ NODE_TYPE_IF,		traverse_if },
	{ NODE_TYPE_FOR,	traverse_for },
	{ NODE_TYPE_WHILE,	traverse_while },
	{ NODE_TYPE_BREAK,	traverse_break },
	{ NODE_TYPE_CONTINUE,	traverse_continue },
	{ NODE_TYPE_ASSIGN,	traverse_assign },
	{ NODE_TYPE_CONST,	traverse_const },
	{ NODE_TYPE_ID,		traverse_id },
	{ NODE_TYPE_FUNC_CALL,	traverse_func_call },
	{ NODE_TYPE_RETURN,	traverse_return },
	{ NODE_TYPE_END_SCOPE,	traverse_empty },
	{ NODE_TYPE_STUB,	traverse_empty },
	{ NODE_TYPE_VECTOR,	traverse_vector },
	{ NODE_TYPE_MATRIX,	traverse_matrix },
	{ NODE_TYPE_ACCESS,	traverse_access },
	{ NODE_TYPE_ROOT,	traverse_root },
	{ NODE_TYPE_UNKNOWN,	NULL }		
};

static void
error_checking(void)
{	
	struct eval *eval;

	if (!errors)
		return;
	
	errors = 0;

	while (list) {
		eval = pop();
		eval_free(eval);
	}	
}

/* set new symbol value */
static void
set_value(struct symbol *sym, struct eval *eval)
{
	value_t v_type;

	return_if_fail(sym != NULL);
	return_if_fail(eval != NULL);

	v_type = eval->v_type;
 
	switch(v_type) {
	case VALUE_TYPE_UNKNOWN:
		symbol_set_val(sym, v_type, NULL);
		break;
	case VALUE_TYPE_DIGIT:
		symbol_set_val(sym, v_type, &eval->digit);
		break;
	case VALUE_TYPE_STRING:
		symbol_set_val(sym, v_type, eval->string);
		break;
	case VALUE_TYPE_VECTOR:
		symbol_set_val(sym, v_type, eval->vector);
		break;
	case VALUE_TYPE_MATRIX:
		symbol_set_val(sym, v_type, eval->matrix);
		break;
	case VALUE_TYPE_VOID:
		break;
	default:
		error(1, "error: unknown value type");
	}	
}

static int
is_true(struct eval *expr)
{
	if (expr->v_type != VALUE_TYPE_DIGIT)                         
		err_msg_ret(FALSE, "error: `expr' must be a digit"); 
                                               
        if (!expr->digit)
		return FALSE;
	
	return TRUE;                      
}

static void
traverse_root(struct ast_node *node)
{
	return_if_fail(node != NULL);
	
	if (node->child)
		traversal(node->child);
}

static res_type_t
traverse_body(struct ast_node *node)
{
	res_type_t res;

	return_val_if_fail(node != NULL, -1);
	
	traversal(node);

	if (errors)
		return RES_ERROR;
	
	if (helper.is_continue) {
		--helper.is_continue;
		res = RES_CONTINUE;

	} else if (helper.is_break) {
		--helper.is_break;
		res = RES_BREAK;

	} else if (helper.is_return) {
		--helper.is_return;
		res = RES_RETURN;

	} else 
		res = RES_OK;
	
	return res;
}

static int
init_dims(struct ast_node **dims, int *dim, int ndims)
{
	struct eval *idx;
	int i;

	for (i = 0; i < ndims; i++) {
		traversal(dims[i]);

		idx = pop();
	
		if (idx->v_type != VALUE_TYPE_DIGIT) 
			err_msg_ret(FALSE, "error: incompatible type for index");
	
		dim[i] = idx->digit;
		eval_free(idx);
	}
	
	return TRUE;	
}

static void
traverse_empty(struct ast_node *node)
{
	return_if_fail(node != NULL);
}

static void
traverse_break(struct ast_node *node)
{
	return_if_fail(node != NULL);

	helper.is_break++;
}

static void
traverse_continue(struct ast_node *node)
{
	return_if_fail(node != NULL);
	
	helper.is_continue++;
}

static void
traverse_return(struct ast_node *node)
{
	struct ast_node_return *_return;

	return_if_fail(node != NULL);	
		
	_return = (struct ast_node_return *)node;
	
	helper.is_return++;
	
	if (_return->ret_val)
		traversal(_return->ret_val);
}

static void
traverse_while(struct ast_node *node)
{
	struct ast_node_while *while_node;
	struct ast_node *stmt, *next;
	struct eval *expr;
	res_type_t res;

	return_if_fail(node != NULL);
	
	while_node = (struct ast_node_while *)node;

	stmt = while_node->stmt;
	
	while (TRUE) {
		traversal(while_node->expr);	
		
		expr = pop();	
	
		if (!is_true(expr))	
			goto exit_while;
	
		/* continue cycle if end is reached */
		if (stmt == NULL || stmt->type == NODE_TYPE_END_SCOPE) {
			stmt = while_node->stmt;
			continue;
		}
	
		next = stmt->next;
	
		res = traverse_body(stmt);
		
		switch(res) {
		case RES_ERROR:
		case RES_BREAK:
		case RES_RETURN:
			goto exit_while;
		default:
			eval_free(expr);
			break;
		}
	
		stmt = next;		
	}

exit_while:
	eval_free(expr);	
}

static void
traverse_for(struct ast_node *node)
{
	struct ast_node_for *for_node;
	struct ast_node *stmt, *next;
	struct eval *expr;
	res_type_t res;

	return_if_fail(node != NULL);
	
	for_node = (struct ast_node_for *)node;
	
	/*init cycle*/
	traversal(for_node->expr1);
	
	stmt = for_node->stmt; 
	
	while (TRUE) {
		traversal(for_node->expr2);
		
		expr = pop();
	
		if (!is_true(expr))
			goto exit_for;

		if (stmt == NULL || stmt->type == NODE_TYPE_END_SCOPE) {
			stmt = for_node->stmt;
			traversal(for_node->expr3);	
			continue;	
		}

		next = stmt->next;

		res = traverse_body(stmt);
		
		switch(res) {
		case RES_ERROR:
		case RES_BREAK:
		case RES_RETURN:
			goto exit_for;
		default:
			eval_free(expr);
			break;
		}
		
		stmt = next;
	}
	
exit_for:
	eval_free(expr);	
}

static void
traverse_if(struct ast_node *node)
{
	struct ast_node_if *if_node;
	struct eval *expr;
	struct ast_node *stmt, *next;

	return_if_fail(node != NULL);
	
	if_node = (struct ast_node_if *)node;

	traversal(if_node->expr);
	
	expr = pop();

	if (expr->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: `expr' must be a digit");
		eval_free(expr);
		return;
	}
	
	if (expr->digit) 
		stmt = if_node->stmt;
	else
		stmt = if_node->_else;
	
	while (stmt != NULL) {
		if (stmt->type == NODE_TYPE_END_SCOPE)
			break;

		next = stmt->next;
		
		traversal(stmt);

		stmt = next;	
	}
}

static void
perform_init_args(struct function *func, struct ast_node **args)
{
	struct eval *eval;
	int i;

	for (i = 0; i < func->nargs; i++) {
		traversal(args[i]);
	
		eval = pop();
	
		set_value(func->args[i], eval);

		eval_free(eval);
	}
}

static void
perform_lib_function(struct function *func, struct ast_node **args)
{
	struct eval *eval;
	struct symbol *sym;
	value_t v_type;
	void *result;
	int ok;

	/* initialize function args */	
	perform_init_args(func, args);
	
	ok = func->handler(func, &v_type, &result);

	if (!ok) {
		err_msg("error: in the function `%s'", func->name);
		return;
	}

	eval = eval_new(TAG_CONST, v_type, result);

	push(eval);

	sym = symbol_get_ans();

	set_value(sym, eval);
}			

static void
perform_custom_function(struct function *func, struct ast_node **args)
{
	struct ast_node *node, *next;
	res_type_t res;

	return_if_fail(func != NULL);
	return_if_fail(args != NULL);
	
	perform_init_args(func, args);
		
	for (node = func->body; node->type != NODE_TYPE_END_SCOPE; node = next) {
		
		next = node->next;
	
		res = traverse_body(node);
	
		switch(res) {
		case RES_ERROR:
		case RES_RETURN:
			break;
		default:
			continue;
		}

		break;			
	}
}

static void
traverse_func_call(struct ast_node *node)
{
	struct function *function;
	struct ast_node_func_call *func_node;
	
	return_if_fail(node != NULL);
	
	func_node = (struct ast_node_func_call *)node;
	
	function = function_table_lookup(func_node->name);
	
	if (function->is_lib) {

		perform_lib_function(function, func_node->args);

	} else {
		symbol_table_set_scope(function->scope);
		
		perform_custom_function(function, func_node->args);
	
		symbol_table_pop();
	}				
}

static void
set_access_value(struct ast_node_access *ac, struct eval *eval)
{
	struct symbol *sym;
	unsigned int row, col, idx;
	int ndims, ok;
	int *dims;

	if (eval->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: non-numerical value");	
		return;
	}
	
	sym = symbol_table_lookup_all(ac->name);
	
	ndims = ac->ndims;
	dims  = umalloc(sizeof(int) * ndims);

	ok = init_dims(ac->dims, dims, ndims);
	
	if (!ok)
		return;
		
	switch(ac->v_type) {
	case VALUE_TYPE_MATRIX:
		if (ndims != 2) {
			err_msg("error: invalid dimention");
			return;
		}
		row = dims[0];
		col = dims[1];
		gsl_matrix_set(sym->matrix, row, col, eval->digit);
		break;
	case VALUE_TYPE_VECTOR:
		if (ndims != 1) {
			err_msg("error: invalid dimention");
			return;
		}
		idx = dims[0];
		gsl_vector_set(sym->vector, idx, eval->digit);
		break;
	default:
		break;
	}		
}

static void
traverse_assign(struct ast_node *node)
{
	struct ast_node_assign *assign;
	struct ast_node *left;
	struct ast_node *right;
	struct ast_node_id *id;
	struct ast_node_access *ac;
	struct eval *eval;
	struct symbol *sym;

	return_if_fail(node != NULL);

	assign = (struct ast_node_assign *)node;
		
	left  = assign->left;
	right = assign->right;
	
	traversal(right);
	
	switch(left->type) {
	case NODE_TYPE_ID:
		id = (struct ast_node_id *)left;
		eval = pop();	
		sym = symbol_table_lookup_all(id->name);
		set_value(sym, eval);	
		break;
	case NODE_TYPE_ACCESS:
		ac = (struct ast_node_access *)left;
		eval = pop();
		set_access_value(ac, eval);
		break;
	default:
		break;
	}

	eval_free(eval);
}

static void
traverse_op(struct ast_node *node)
{
	struct ast_node_op *op;
	struct eval *a,*b, *c;
	struct symbol *sym;

	return_if_fail(node != NULL);
	
	op = (struct ast_node_op *)node;

	traversal(op->left);
	traversal(op->right);
		
	b = pop();
	a = pop();
	
	switch(op->opcode) {
	case OPCODE_ADD:
	case OPCODE_SUB:
		c = eval_add_op(a, b, op->opcode);
		break;	
	case OPCODE_MULT:
	case OPCODE_DIV:
		c = eval_mult_op(a, b, op->opcode);
		break;
	case OPCODE_AND:
	case OPCODE_OR:
		c = eval_logic_op(a, b, op->opcode);
		break;
	case OPCODE_LT:
	case OPCODE_LE:
	case OPCODE_GT:
	case OPCODE_GE:
	case OPCODE_EQ:
	case OPCODE_NE:
		c = eval_rel_op(a, b, op->opcode);
		break;
	case OPCODE_EXP:
		c = eval_exp_op(a, b, op->opcode);
		break;
	default:
		error(1, "error: unknown operation");
	}

	if (c != NULL)
		push(c);

	eval_free(a);
	eval_free(b);
	
	sym = symbol_get_ans();
	set_value(sym, c);
}

static void
traverse_const(struct ast_node *node)
{
	struct ast_node_const *_const;
	struct symbol *sym;
	struct eval *eval;
	tag_type_t tag;
	value_t	v_type;
	
	return_if_fail(node != NULL);
	
	_const = (struct ast_node_const *)node;
	
	tag    = TAG_CONST;
	v_type = _const->v_type;

	switch(v_type) {	
	case VALUE_TYPE_DIGIT:
		eval = eval_new(tag, v_type, &_const->digit);
		break;
	case VALUE_TYPE_STRING:
		eval = eval_new(tag, v_type, _const->string);
		break;
	default:
		break;
	}	
	
	push(eval);
	
	sym = symbol_get_ans();
	set_value(sym, eval);
}	

static void
traverse_id(struct ast_node *node)
{
	struct ast_node_id *id;
	struct symbol *symbol;
	struct eval *eval;
	value_t v_type;
	tag_type_t tag;

	return_if_fail(node != NULL);
	
	id = (struct ast_node_id *)node;
	
	symbol = symbol_table_lookup_all(id->name);
	
	tag    = TAG_SYMBOL;
	v_type = symbol->v_type;
 
	switch(v_type) {
	case VALUE_TYPE_DIGIT:	
		eval = eval_new(tag, v_type, &symbol->digit);
		break;	
	case VALUE_TYPE_STRING:
		eval = eval_new(tag, v_type, symbol->string);
		break;
	case VALUE_TYPE_VECTOR:
		eval = eval_new(tag, v_type, symbol->vector);
		break;
	case VALUE_TYPE_MATRIX:
		eval = eval_new(tag, v_type, symbol->matrix);
		break;
	default:
		err_msg("error: unknown variable\n");	
		break;
	}	
	
	push(eval);	
}

static void
traverse_vector(struct ast_node *node)
{
	struct ast_node_vector *vc_node;
	struct eval *eval;
	struct symbol *sym;
	gsl_vector *vc;
	int i;

	return_if_fail(node != NULL);
	
	vc_node = (struct ast_node_vector *)node;
	
	vc = gsl_vector_alloc(vc_node->size);

	for (i = 0; i < vc_node->size; i++) {
		traversal(vc_node->elem[i]);
	
		eval = pop();	
	
		if (eval->v_type != VALUE_TYPE_DIGIT) {
			err_msg("error: nonnumberical value");
			goto err_vc;
		}
		
		gsl_vector_set(vc, i, eval->digit);
		
		eval_free(eval);
	}
	
	eval = eval_new(TAG_CONST, VALUE_TYPE_VECTOR, vc);
	push(eval);

	sym = symbol_get_ans();
	set_value(sym, eval);
	return;
err_vc:
	eval_free(eval);
	gsl_vector_free(vc);
}

static void 
traverse_matrix(struct ast_node *node)
{
	struct ast_node_matrix *mx_node;
	struct symbol *sym;
	struct eval *eval;
	gsl_matrix *mx;
	int i, j, idx;

	return_if_fail(node != NULL);
	
	mx_node = (struct ast_node_matrix *)node;
	
	mx = gsl_matrix_alloc(mx_node->size1, mx_node->size2);
	
	for (i = 0; i < mx_node->size1; i++) {
	
		idx = i * mx_node->size2;

		for (j = 0; j < mx_node->size2; j++) {
			traversal(mx_node->elem[idx + j]);

			eval = pop();
	
			if (eval->v_type != VALUE_TYPE_DIGIT) {
				err_msg("error: nonnumerical value");
				goto err_mx;
			}
			
			gsl_matrix_set(mx, i, j, eval->digit);
	
			eval_free(eval);
		}
	}

	eval = eval_new(TAG_CONST, VALUE_TYPE_MATRIX, mx);
	push(eval);

	sym = symbol_get_ans();
	set_value(sym, eval);
	return;
err_mx:
	eval_free(eval);
	gsl_matrix_free(mx);		
}

static void
traverse_access(struct ast_node *node)
{
	struct ast_node_access *ac_node;
	struct symbol *sym;
	struct eval *eval;
	double dg;
	int ndims;
	int *dims;

	return_if_fail(node != NULL);	
		
	ac_node = (struct ast_node_access *)node;

	sym = symbol_table_lookup_all(ac_node->name);
	 
	ndims = ac_node->ndims;	
	dims  = umalloc(sizeof(int) * ndims);

	init_dims(ac_node->dims, dims, ndims);

	switch(sym->v_type) {
	case VALUE_TYPE_VECTOR:	
		if (ndims != 1) {
			err_msg("error: invalid dimention");
			return;
		}
		dg   = gsl_vector_get(sym->vector, dims[0]);
		eval = eval_new(TAG_CONST, VALUE_TYPE_DIGIT, &dg);
		push(eval);
		break;
	case VALUE_TYPE_MATRIX:
		if (ndims != 2) {
			err_msg("error: invalid dimention");
			return;	
		}
		dg   = gsl_matrix_get(sym->matrix, dims[0], dims[1]);
		eval = eval_new(TAG_CONST, VALUE_TYPE_DIGIT, &dg);
		push(eval);
		break;		
	default:
		err_msg("error: id is not a vector or a matrix");
		return;
	}
	
	
}

static void
traverse_tree(struct ast_node *tree)
{
	int i;

	for (i = 0; traverse_nodes[i].type != NODE_TYPE_UNKNOWN; i++) {
		if (tree->type == traverse_nodes[i].type) {
			traverse_nodes[i].handler(tree);
			return;
		}
	}	
}

void
traversal_print_result(void)
{
	struct eval *res;
	
	error_checking();

	if (list != NULL) {
		res = pop();
		eval_print(res);
		eval_free(res);	
	}		
}

void
traversal(struct ast_node *tree)
{
	if (!errors)
		traverse_tree(tree);			
}
