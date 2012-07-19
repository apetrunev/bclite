#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "lex.h"
#include "syntax.h"
#include "symbol.h"
#include "umalloc.h"
#include "function.h"

extern struct lex lex;

static int errors;

#define error_msg(message) \
do { \
	errors++; \
	fprintf(stderr, "%s\n", (message)); \
} while(0)

struct scope_ctx {
	int is_func;
	int is_cycle;
	int is_cond;
};

static struct lex lex_prev;
static token_t current_token;

static struct ast_node *or_expr(void);
static struct ast_node *and_expr(void);
static struct ast_node *rest_or(struct ast_node *node);
static struct ast_node *rest_and(struct ast_node *node);
static struct ast_node *rel_expr(void);
static struct ast_node *sum_expr(void);
static struct ast_node *mult_expr(void);
static struct ast_node *rest_mult(struct ast_node *node);
static struct ast_node *rest_sum(struct ast_node *node);
static struct ast_node *exp_expr(void);
static struct ast_node *rest_exp(struct ast_node *node);
static struct ast_node *token_id(char *name);
static struct ast_node *function_call(char *name);
static struct ast_node *stmt(void *opaque);
static struct ast_node *stmts(void *opaque);
static struct ast_node *process_matrix(void);


static void
sync_stream(void)
{
	while (current_token != TOKEN_EOL)
		current_token = get_next_token();
}

static void
lex_dup(void)
{
	switch (lex.token) {
	case TOKEN_DOUBLE:
		lex_prev.real = lex.real;
		break;
	case TOKEN_ID:
		lex_prev.id = ustrdup(lex.id);
		break;
	case TOKEN_STRING:
		lex_prev.string = ustrdup(lex.string);
		break;
	default:
		break;
	
	}

	lex_prev.token = current_token;
}

static void
lex_prev_free(void)
{
	switch(lex_prev.token) {
	case TOKEN_ID:
		ufree(lex_prev.id);
		break;
	case TOKEN_STRING:
		ufree(lex_prev.string);
		break;
	default:
		break;	
	}
}

static void
consume_token(void)
{
	lex_prev_free();
	lex_dup();
	current_token = get_next_token();
}

static int
match(token_t token)
{
	if (current_token == token) {
		lex_prev_free();
		lex_dup();
		current_token = get_next_token();
		return 1;	
	}

	return 0;
}

static struct ast_node*
term(void)
{
	struct ast_node_const *_const;
	struct ast_node *node;

	if (match(TOKEN_ID)) {
		node = token_id(lex_prev.id);
		return node;
	} else if (match(TOKEN_DOUBLE)) {
		_const = ast_node_const(VALUE_TYPE_DIGIT, &lex_prev.real);
		return AST_NODE(_const);
	} else if (match(TOKEN_STRING)) {
		_const = ast_node_const(VALUE_TYPE_STRING, lex_prev.string);
		return AST_NODE(_const);
	} else if (match(TOKEN_LBRACKET)) {
		node = process_matrix();
		return node;
	}

	return NULL;
}

static struct ast_node*
process_matrix(void)
{	
	struct ast_node_stub *stub_node;
	struct ast_node **elem;
	struct ast_node *node;	
	int row, col, len;
	int prev_col;
	int i;

	elem = NULL;
	node = NULL;

	row = col = 1;
	len = prev_col = 0;

	while (TRUE) {
		node = or_expr();

		if (node == NULL) {
			error_msg("expr expected");
			sync_stream();
			goto err;
		}		
		
		elem = urealloc(elem, ++len*sizeof(struct ast_node *));
		elem[len - 1] = node;

		switch(current_token) {
		case TOKEN_COMMA:
			consume_token();
			col++;
			continue;
		case TOKEN_SEMICOLON:
			consume_token();
			if (prev_col == 0)
				prev_col = col;
			else if (prev_col != col) {
				error_msg("incompatible column count");
				sync_stream();
				goto err;
			}
			col = 1;
			row++;
			continue;
		case TOKEN_RBRACKET:
			consume_token();
			break;
		default:
			error_msg("syntax error");
			sync_stream();
			goto err;
		} 
		break;							
	}

	if (row == 1 || col == 1)
		node = (struct ast_node *)ast_node_vector(elem, len);
	else
		node = (struct ast_node *) ast_node_matrix(elem, row, col);

	return node;

err:
	if (node != NULL)
		ast_node_unref(node);
	
	if (elem != NULL) 
		for (i = 0; i < len; i++)
			ast_node_unref(elem[i]);

	stub_node = ast_node_stub();

	return AST_NODE(stub_node);	
}

static struct ast_node*
function_call(char *name)
{
	struct function *func_ctx;
	struct ast_node_func_call *func_call;
	struct ast_node_stub *stub_node;	
	struct ast_node *arg;
	int nargs = 0;

	func_ctx = function_table_lookup(name);

	consume_token();
	
	if (func_ctx == NULL) {
		error_msg("unknown function");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);	
	}

	func_call = ast_node_func_call(func_ctx->name);

	while (!match(TOKEN_RPARENTH)) {
		if (match(TOKEN_EOL)) {
			error_msg("EOL in function call");
			sync_stream();
			goto free;
			
		}
		
		arg = or_expr();

		if (arg == NULL) {
			error_msg("error: function argument expected");
			sync_stream();
			goto free;
		}
			
		nargs++;

		ast_node_func_call_add_arg(func_call, arg);
		
		if (current_token != TOKEN_RPARENTH && !match(TOKEN_COMMA)) {
			error_msg("error: missed comma");
			sync_stream();
			goto free;
		}				
	}
	
	if (func_ctx->nargs != nargs) {
		error_msg("error: unmatched arguments count");
		sync_stream();
		goto free;
	}

	return AST_NODE(func_call);	
free:
	ast_node_unref(AST_NODE(func_call));	
	stub_node = ast_node_stub();
	return	AST_NODE(stub_node);
}

/*static int
verify_index_type(struct ast_node *node)
{
	struct ast_node_const *_const;
	struct ast_node_id *id;
	struct symbol *sym;

	switch(node->type) {
	case NODE_TYPE_CONST:
		_const = (struct ast_node_const *)node;
		if (_const->v_type != VALUE_TYPE_DIGIT)
			return FALSE;
		break; 
	case NODE_TYPE_ID:
		id  = (struct ast_node_id *)node;
		sym = symbol_table_lookup_all(id->name);
		if (sym->v_type != VALUE_TYPE_DIGIT)
			return FALSE;
		break;
	default:
		error_msg("error: bad index");
		return FALSE;
	}
	
	return TRUE;
}	
*/

static struct ast_node*
access_node(char *name)
{
	struct ast_node_stub *stub_node;
	struct ast_node_access *ac_node;
	struct ast_node *idx;
	struct symbol *sym;

	sym = symbol_table_lookup_all(name);
	
	if (sym == NULL) {
		error_msg("error: no such symbol");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	/* `[' */
	consume_token();
	
	ac_node = ast_node_access(sym->v_type, sym->name);

	do {
		idx = or_expr();
			
		if (idx == NULL) {	
			error_msg("error: empty index");
			sync_stream();
			goto ac_error;
		}
		
		ast_node_access_add(ac_node, idx);

		if (current_token != TOKEN_RBRACKET) {
			error_msg("error: missed `]'");	
			sync_stream();
			goto ac_error;
		}
		/* `]' */
		consume_token();

	} while (match(TOKEN_LBRACKET));

	return AST_NODE(ac_node);

ac_error:
	ast_node_unref(AST_NODE(ac_node));
	stub_node = ast_node_stub();

	return AST_NODE(stub_node);		
}

struct ast_node*
token_id(char *name)
{
	struct ast_node *node;
	struct symbol *sym;

	switch (current_token) {
	case TOKEN_LPARENTH :
		node = function_call(name);
		break;
	case TOKEN_LBRACKET:
		node = access_node(name);
		break;
	default:		
		sym = symbol_table_lookup_all(name);
		
		if (sym == NULL) {
			sym = symbol_new(name, VALUE_TYPE_UNKNOWN);
			symbol_table_global_put_symbol(sym);
		}

		node = (struct ast_node *)ast_node_id(name);
		break;
	}

	return AST_NODE(node);	
}

static struct ast_node*
term_expr(void)
{
	struct ast_node *node;

	if (!match(TOKEN_LPARENTH))
		return term();

	node = or_expr();
	
	if (!match(TOKEN_RPARENTH)) 
		error_msg("error: missed `)'");	

	return node;	
}

static struct ast_node*
exp_expr(void)
{
	struct ast_node *node;
	struct ast_node *rest_node;
	
	node = term_expr();
		
	if (node == NULL)
		return NULL;
	
	rest_node = rest_exp(node);
	
	return rest_node;
}

static struct ast_node*
rest_exp(struct ast_node *node)
{
	struct ast_node *expr_node;
	struct ast_node *prev_node;
	struct ast_node *ret_node;
	char op;

	prev_node = node;

	while(TRUE) {
		switch(current_token) {
		case TOKEN_CARET:
			op = '^';
			break;
		default:
			goto exit_exp;			
		}

		consume_token();
		expr_node = term_expr();
	
		if (expr_node == NULL) {
			error_msg("error: syntax error");
			expr_node = (struct ast_node *)ast_node_stub();
			sync_stream();
			goto exit_exp;
		}
	
		prev_node = (struct ast_node *)ast_node_op(op, prev_node, expr_node);
	}
	
exit_exp:
	ret_node = prev_node;
	return ret_node;	
}

static struct ast_node*
mult_expr(void)
{
	struct ast_node *node;
	struct ast_node *rest_node;
	
	node = exp_expr();

	if (node == NULL)
		return NULL;

	rest_node = rest_mult(node);
	
	return rest_node;	
}

static struct ast_node*
rest_mult(struct ast_node *node)
{
	struct ast_node *expr_node;
	struct ast_node *prev_node;
	struct ast_node *ret_node;
	char op;

	prev_node = node;
	
	while (TRUE) {
		switch(current_token) {	
		case TOKEN_ASTERIK:
			op = '*';
			break;
		case TOKEN_SLASH:
			op = '/';
			break;
		default:
			goto exit_mult;
		}

		consume_token();
		expr_node = exp_expr();

		if (expr_node == NULL) {
			error_msg("error: syntax error");
			expr_node = (struct ast_node *)ast_node_stub();
		}
	
		prev_node = (struct ast_node *)ast_node_op(op, prev_node, expr_node);
	}

exit_mult:	
	ret_node = prev_node;
	return ret_node;
	
}

static struct ast_node*
sum_expr(void) 
{
	struct ast_node *node;
	struct ast_node *rest_node;

	node = mult_expr();

	if (node == NULL)
		return NULL;

	rest_node = rest_sum(node);

	return rest_node;
}

static struct ast_node*
rest_sum(struct ast_node *node)
{
	struct ast_node *prev_node;
	struct ast_node *ret_node;
	struct ast_node *mult_node;
	char op;
	
	prev_node = node;
	
	while (TRUE) {
		switch(current_token) {
		case TOKEN_PLUS:
			op = '+';
			break;
		case TOKEN_MINUS:
			op = '-';
			break;
		default:
			goto exit_sum;
		}

		consume_token();
		mult_node = mult_expr();
		
		if (mult_node == NULL) {
			error_msg("error: syntax error");
			mult_node = (struct ast_node *)ast_node_stub();
		}
		
		prev_node = (struct ast_node *)ast_node_op(op, prev_node, mult_node);	
	}

exit_sum:
	ret_node = prev_node;
	return ret_node;
}

static struct ast_node*
or_expr(void)
{
	struct ast_node *node;
	struct ast_node *rest_node;
	
	node = and_expr();
	
	if (node == NULL)
		return NULL;
		
	rest_node = rest_or(node);
	
	return rest_node;	
}

static struct ast_node*
rest_or(struct ast_node *node)
{
	struct ast_node *ret_node;	
	struct ast_node *left;
	struct ast_node *right;

	ret_node = node;
	
	while (TRUE) {
		switch(current_token) {
		case TOKEN_OR :
			consume_token();

			left  = ret_node;
			right = and_expr();
			
			if (right == NULL) {
				error_msg("error: expression expected after ||");	
				right = (struct ast_node *)ast_node_stub();
				sync_stream();
			}
			ret_node = (struct ast_node *)ast_node_logic_op(OPCODE_OR, left, right);
			continue;
		default:
			goto exit_or;
		}
	}

exit_or:	
	return ret_node;
}

static struct ast_node*
and_expr(void)
{
	struct ast_node *node;
	struct ast_node *rest_node;

	node = rel_expr();
	
	if (node == NULL)
		return NULL;
	
	rest_node = rest_and(node);
	
	return	rest_node;
}

static struct ast_node*
rest_and(struct ast_node *node)
{
	struct ast_node *ret_node;
	struct ast_node *left;
	struct ast_node *right;

	ret_node = node;
	
	while (TRUE) {
		switch (current_token) {
		case TOKEN_AND:
			consume_token();

			left  = ret_node;
			right = rel_expr();

			if (right == NULL) {
				error_msg("error: missed expression");
				right = (struct ast_node *)ast_node_stub();
				sync_stream();
			}	
			ret_node = (struct ast_node *)ast_node_logic_op(OPCODE_AND, left, right);
			continue;
		default:
			goto exit_and;
		}
	}

exit_and:
	return ret_node;
}

static struct ast_node*
rel_expr(void)
{
	struct ast_node *left;
	struct ast_node *right;
	struct ast_node_op *op_node;
	opcode_type_t opcode;

	left = sum_expr();

	if (left == NULL)
		return NULL;

	switch(current_token) {
	case TOKEN_EQ:
		consume_token();
		opcode = OPCODE_EQ;
		break;	
	case TOKEN_LT:
		consume_token();
		opcode = OPCODE_LT;
		break; 
	case TOKEN_GT:
		consume_token();
		opcode = OPCODE_GT;
		break;
	case TOKEN_LE:
		consume_token();
		opcode = OPCODE_LE;
		break;
	case TOKEN_GE:
		consume_token();
		opcode = OPCODE_GE;
		break;
	case TOKEN_NE:
		consume_token();
		opcode = OPCODE_NE;
		break;
	default:
		return left;
	}
	
	right = sum_expr();

	if (right == NULL) {
		error_msg("error: expression expected");
		right = (struct ast_node *)ast_node_stub();
		sync_stream();
	}
		
	op_node = ast_node_rel_op(opcode, left, right);
	
	return AST_NODE(op_node);		
}


static struct ast_node*
expr(void)
{
	struct ast_node *lvalue;
	struct ast_node *rvalue;
	struct ast_node_assign *assign;

	lvalue = or_expr();
	
	if (current_token != TOKEN_EQUALITY)
		return lvalue;	
	/* `=' */
	consume_token();
	
	switch(lvalue->type) {
	case NODE_TYPE_ID:
	case NODE_TYPE_ACCESS:
		break;
	default:	
		error_msg("error: rvalue assignmet");
		sync_stream();
		return lvalue;
	}
	
	rvalue = or_expr();

	if (!rvalue) {
		error_msg("error: expression expected `='");
		sync_stream();
		rvalue = (struct ast_node *)ast_node_stub();
	}
	
	assign = ast_node_assign(lvalue, rvalue);

	return AST_NODE(assign);
}

struct ast_node*
process_return_node(void *opaque)
{
	struct ast_node *ret_val;
	struct ast_node_return *_return;
	
	if (!opaque) {
		error_msg("error: return outside function");
		sync_stream();
		ret_val = (struct ast_node *)ast_node_stub();
		return AST_NODE(ret_val);
	}
	
	ret_val = sum_expr();
	
	_return = ast_node_return(ret_val);

	return AST_NODE(_return);
}

static void
process_function_body(char *name)
{
	struct scope_ctx helper;
	struct ast_node *body;
	struct function *func_ctx;
	struct symbol_table *scope;
	int i;

	consume_token();
	
	func_ctx = function_table_lookup(name);

	symbol_table_push();
	/* insert arguments in function scope */	
	for (i = 0; i < func_ctx->nargs; i++)
		symbol_table_put_symbol(func_ctx->args[i]);
	
	
	memset(&helper, 0, sizeof(helper));
	
	helper.is_func = 1;
	
	body  = stmts(&helper);
	
	scope = symbol_table_get_current_table();
	
	func_ctx->body  = body;		
	func_ctx->scope = scope;
	
	symbol_table_pop();	

	if(errors) {
		error_msg("->redefine your function");
		function_table_delete_function(name);
	}	
}

static void
process_args(char *name)
{
	struct function *func;
	struct symbol *arg;

	/*(*/
	consume_token();

	func = function_table_lookup(name);

	/* Delete an old function and make a new one */
	if (func != NULL) 
		function_table_delete_function(name);	

	func = function_new(name);
	
	while (!match(TOKEN_RPARENTH)) {
		if (match(TOKEN_EOL)) {
			error_msg("error: new line in function definition");
			return;	
		}
		
		if (!match(TOKEN_ID)) {
			error_msg("error: unexpected symbol in definition");
			sync_stream();
			ufree(func);
			return;
		}
	
		arg = symbol_new(lex_prev.id, VALUE_TYPE_UNKNOWN);
		
		function_add_arg(func, arg);
		
		if (current_token != TOKEN_RPARENTH && !match(TOKEN_COMMA)) {
			error_msg("error: comma expected");
			sync_stream();
			return;
		}			
	}

	switch(current_token) {
	case TOKEN_LBRACE:
		function_table_insert(func);
		process_function_body(name);
		break;
	default:
		error_msg("error: `{' expected");
		sync_stream();
		break;
	}	
}

struct ast_node*
process_function(void *opaque)
{
	char *name;
	struct ast_node_stub *stub_node;

	if (!match(TOKEN_ID)) {
		error_msg("error: function name expected after `function'");	
		sync_stream();
		goto exit;	
	}
	
	name = ustrdup(lex_prev.id);

	if (current_token != TOKEN_LPARENTH) {
		error_msg("error: `(' expected");
		sync_stream();
		goto exit;
	}
	
	process_args(name);
exit:	
	stub_node = ast_node_stub();

	return AST_NODE(stub_node);
}

static struct ast_node*
if_expr(void *opaque)
{
	struct ast_node_if *if_node;
	struct ast_node_stub *stub_node;
	struct ast_node *expr;
	struct ast_node *stmt_node;
	struct ast_node *_else;
	struct scope_ctx helper;

	if (!match(TOKEN_LPARENTH)) {
		error_msg("error: `(' expected after if");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	expr = or_expr();
	
	if (expr == NULL) {
		error_msg("error: expression expected after `('");
		sync_stream(); 
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	if (!match(TOKEN_RPARENTH)) {	
		error_msg("error: `)' expected after exprssion");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	if (opaque == NULL) {
		memset(&helper, 0, sizeof(helper));
		helper.is_cond++;
	} else {
		helper = *(struct scope_ctx *)opaque;
		helper.is_cond++;
	}	
		
	stmt_node = stmt(&helper);
		
	if (stmt_node == NULL) {
		error_msg("error: statment expected after `)'");
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}

	_else = NULL;

	if (match(TOKEN_ELSE))
		_else = stmt(&helper);
	
	if_node = ast_node_if(expr, stmt_node, _else);
		
	helper.is_cond--;
	
	return AST_NODE(if_node);
}

static struct ast_node*
process_scope(void *opaque)
{
	struct ast_node *stmt;
	struct ast_node_stub *stub_node;
	struct scope_ctx *helper;

	if (opaque == NULL) {
		error_msg("error: unexpected symbol");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}	
	
	helper = (struct scope_ctx *)opaque;

	if (!helper->is_cycle && !helper->is_cond) {
		error_msg("error: scope outside context");	
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
		
	stmt = stmts(opaque);	
		
	return stmt;
}

static struct ast_node*
unknown_expr(void *opaque)
{
	struct ast_node_stub *node;

	error_msg("error: unknown expression");

	sync_stream();	

	node = ast_node_stub();

	return AST_NODE(node);
}

static struct ast_node*
other_expr(void *opaque)
{
	struct ast_node *node;
	struct ast_node_stub *stub_node;

	node = expr();	
		
	if (node == NULL) {
		if (current_token != TOKEN_EOL) {
			error_msg("error: unexpected symbol");
			sync_stream();
			stub_node = ast_node_stub();
			return AST_NODE(stub_node);
		}
	}
	
	return node;	
}

static struct ast_node*
for_expr(void *opaque)
{
	struct ast_node_for *for_node;
	struct ast_node_stub *stub_node;
	struct ast_node *expr1;
	struct ast_node *expr2;
	struct ast_node *expr3;
	struct ast_node *_stmt;
	struct scope_ctx helper;

	if (!match(TOKEN_LPARENTH)) {
		error_msg("error: `(' expected after for");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	expr1 = expr();
	
	if (!match(TOKEN_SEMICOLON)) {
		error_msg("error: `;' expected after for(");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	expr2 = or_expr();

	if (!match(TOKEN_SEMICOLON)) {
		error_msg("error: `;' expected after for(;");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}	
	
	expr3 = expr();
		
	if (!match(TOKEN_RPARENTH)) {
		error_msg("error: `)' expected after for(;;");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	if (opaque) {
		helper = *(struct scope_ctx *)opaque;
		helper.is_cycle++;
	} else {	
		memset(&helper, 0, sizeof(helper));
		helper.is_cycle++;
	}

	_stmt = stmt(&helper);
	
	if (_stmt == NULL) {
		error_msg("error: stmt expected after for(;;)");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	for_node = ast_node_for(expr1, expr2, expr3, _stmt);
	
	helper.is_cycle--;

	return AST_NODE(for_node);
}

static struct ast_node*
while_expr(void *opaque)
{
	struct ast_node_stub *stub_node;
	struct ast_node_while *while_node;
	struct ast_node *expr;
	struct ast_node *_stmt;
	struct scope_ctx helper;

	if (!match(TOKEN_LPARENTH)) {		
		error_msg("errorr: `(' expected after while");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	expr = or_expr();
	
	if (expr == NULL) {
		error_msg("error: NULL expression in while()");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	if (!match(TOKEN_RPARENTH)) {
		error_msg("error: `)' expected after while(");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	if (opaque) {
		helper = *(struct scope_ctx *)opaque;
		helper.is_cycle++;
	} else {
		memset(&helper, 0, sizeof(helper));
		helper.is_cycle++;
	}
		
	_stmt = stmt(&helper);
	
	if (_stmt == NULL) {
		error_msg("error: statmet expected after while()");
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}

	while_node = ast_node_while(expr, _stmt);
	
	helper.is_cycle--;

	return AST_NODE(while_node);
}

static struct ast_node*
break_expr(void *opaque)
{
	struct ast_node_stub *stub_node;
	struct ast_node_break *break_node;
	struct scope_ctx *helper;

	if (opaque == NULL) {
		error_msg("error: `break' outside loop");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);	
	}
	
	helper = (struct scope_ctx *)opaque;
	
	if (!helper->is_cycle) {
		error_msg("error: `break' outside loop");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	break_node = ast_node_break();
	
	return AST_NODE(break_node);
}

static struct ast_node*
continue_expr(void *opaque)
{
	struct ast_node_continue *continue_node;
	struct ast_node_stub *stub_node;
	struct scope_ctx helper;

	if (opaque == NULL) {
		error_msg("error: `continue' outside loop");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	helper = *(struct scope_ctx *)opaque;
	
	if (!helper.is_cycle) {
		error_msg("error: `continue' outside loop`");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	continue_node = ast_node_continue();
	
	return AST_NODE(continue_node);
}

static struct ast_node*
end_scope_expr(void *opaque)
{
	struct ast_node_stub *stub_node;
	struct ast_node_end_scope *scope_node;
	
	if (opaque == NULL) {
		error_msg("error: unexpected symbol");
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}

	scope_node = ast_node_end_scope();
	
	return AST_NODE(scope_node);
}

struct ast_node*
process_local_declaration(void *opaque)
{
	struct scope_ctx helper;
	struct ast_node_stub *stub_node;
	struct symbol *var;

	if (opaque == NULL) {
		error_msg("error: `local' outside function");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);	
	}
	
	helper = *(struct scope_ctx *)opaque;
	
	if (!helper.is_func) {
		error_msg("error: `local' outside function");
		sync_stream();
		stub_node = ast_node_stub();
		return AST_NODE(stub_node);
	}
	
	while (!match(TOKEN_EOL)) {
		if (match(TOKEN_ID)) {
			var = symbol_new(lex_prev.id, VALUE_TYPE_UNKNOWN);
			symbol_table_put_symbol(var);	
		}	
		
		if (current_token != TOKEN_EOL && !match(TOKEN_COMMA)) {
			error_msg("error: `,' expected after variable");
			sync_stream();
			stub_node = ast_node_stub();
			return AST_NODE(stub_node);
		}
	}

	stub_node = ast_node_stub();
	
	return AST_NODE(stub_node);
}

static struct ast_node*
stmt(void *opaque)
{
	struct ast_node *node;
	token_t	prev_token;

	prev_token = current_token;

	if (match(TOKEN_FUNCTION))

		node = process_function(opaque);
	
	else if (match(TOKEN_LOCAL))
		
		node = process_local_declaration(opaque);

	else if (match(TOKEN_IF)) 
		
		node = if_expr(opaque);
	
	else if (match(TOKEN_FOR))
			
		node = for_expr(opaque);
	
	else if (match(TOKEN_WHILE))
		
		node = while_expr(opaque);
	
	else if (match(TOKEN_BREAK))
		
		node = break_expr(opaque);

	else if (match(TOKEN_CONTINUE))
	
		node = continue_expr(opaque);

	else if (match(TOKEN_RETURN))

		node = process_return_node(opaque);

	else if (match(TOKEN_LBRACE))

		node = process_scope(opaque);

	else if (match(TOKEN_RBRACE)) 

		node = end_scope_expr(opaque);
	
	else if (match(TOKEN_UNKNOWN)) 

		node = unknown_expr(opaque);

	else	
		node = other_expr(opaque);

	return node;	
}

static struct ast_node*
stmts(void *opaque)
{
	struct ast_node *ret_node;
	struct ast_node *stmt_head;
	struct ast_node *prev_node;

	stmt_head = NULL;
	prev_node = NULL;

	while (current_token != TOKEN_EOF) {	
		if (opaque != NULL && current_token == TOKEN_EOL)
			consume_token();

		if (opaque == NULL && current_token == TOKEN_EOL)
			break;

		ret_node = stmt(opaque);

		if (errors)
			break;
	
		if (ret_node == NULL)
			continue;

		if (stmt_head == NULL)
			stmt_head = ret_node;

		if (prev_node != NULL) {
			prev_node->next  = ret_node;
			ret_node->parent = prev_node;
		}

		prev_node = ret_node;
		
		if (ret_node->type == NODE_TYPE_END_SCOPE) 
			break;			
	}

	return  stmt_head;
}
void
check_eof(int *eof)
{
	switch(current_token) {
	case TOKEN_EOF:
		*eof = TRUE;
		break;
	default:
		*eof = FALSE;
		break;
	}
}

int
programme(struct ast_node **root, int *eof)
{
	struct ast_node *tree;
	
	errors = 0;
	
	consume_token();

	tree  = stmts(NULL);
	*root = (struct ast_node *)ast_node_root(tree);
	
	check_eof(eof);

	return errors;
}

