#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "as_tree.h"
#include "macros.h"
#include "umalloc.h"

static struct ast_node*
ast_node_new(size_t size)
{
	struct ast_node *node;
	
	node = umalloc0(size);
	
	return node;
}

static void
ast_node_stub_free(struct ast_node *node)
{
	return_if_fail(node != NULL);

	ufree(node);
}

struct ast_node_stub*
ast_node_stub(void)
{
	struct ast_node_stub *node;
	
	node = (struct ast_node_stub *)ast_node_new(sizeof(*node));

	AST_NODE(node)->destructor = ast_node_stub_free;
	AST_NODE(node)->type = NODE_TYPE_STUB;

	return node;
}

void
ast_node_unref(struct ast_node *node)
{
	return_if_fail(node != NULL);
	
	if (node->destructor != NULL)
		node->destructor(node);
}


static void
ast_node_const_free(struct ast_node *node)
{
	struct ast_node_const *_const;

	return_if_fail(node != NULL);
	
	_const = (struct ast_node_const *)node;
	
	if (_const->v_type == VALUE_TYPE_STRING)	
		ufree(_const->string);

	ufree(_const);
}

struct ast_node_const*
ast_node_const(value_t v_type, void *data)
{
	struct ast_node_const *_const;

	_const = (struct ast_node_const *)ast_node_new(sizeof(*_const));

	switch(v_type) {
	case VALUE_TYPE_DIGIT:
		_const->digit = *(double *)data;
		break;
	case VALUE_TYPE_STRING:
		_const->string = ustrdup((char *)data);
		break;
	default:
		error(1, "incompatible value type");
	}

	_const->v_type = v_type;	

	AST_NODE(_const)->type = NODE_TYPE_CONST;
	AST_NODE(_const)->destructor = ast_node_const_free;

	return _const;
}


static void
ast_node_id_free(struct ast_node *node)
{
	struct ast_node_id *id;

	return_if_fail(node != NULL);
		
	id = (struct ast_node_id *)node;

	ufree(id->name);
	ufree(id);
}	

struct ast_node_id*
ast_node_id(char *name)
{
	struct ast_node_id *node;
	
	node = (struct ast_node_id *)ast_node_new(sizeof(*node));
	node->name = strdup(name);

	AST_NODE(node)->type = NODE_TYPE_ID;
	AST_NODE(node)->destructor = ast_node_id_free;

	return node; 
}

static void
ast_node_op_free(struct ast_node *node)
{
	struct ast_node_op *operation;

	return_if_fail(node != NULL);

	operation = (struct ast_node_op *)node;
	
	ast_node_unref(operation->left);
	ast_node_unref(operation->right);
	
	ufree(operation);
}

struct ast_node_op*
ast_node_op(char op_helper, struct ast_node *left, struct ast_node *right)
{
	struct ast_node_op *operation;

	return_val_if_fail(left != NULL, NULL);
	return_val_if_fail(right != NULL, NULL);
	
	operation = (struct ast_node_op *)ast_node_new(sizeof(*operation));
	switch(op_helper) {
	case '^':
		operation->opcode = OPCODE_EXP;
		AST_NODE(operation)->type = NODE_TYPE_EXP_OP;
		break;
	case '*':
		operation->opcode = OPCODE_MULT;
		AST_NODE(operation)->type = NODE_TYPE_MULT_OP;
		break;
	case '/':
		operation->opcode = OPCODE_DIV;
		AST_NODE(operation)->type = NODE_TYPE_MULT_OP;
		break;
	case '+':
		operation->opcode = OPCODE_ADD;
		AST_NODE(operation)->type = NODE_TYPE_ADD_OP;
		break;
	case '-':
		operation->opcode = OPCODE_SUB;
		AST_NODE(operation)->type = NODE_TYPE_ADD_OP;
		break;
	default:
		fprintf(stderr, "unexpected operation %c\n", op_helper);
		break;
	}
	
	operation->left  = left;
	operation->right = right;
	
	AST_NODE(operation)->child = left;
	AST_NODE(operation)->destructor = ast_node_op_free;

	left->parent = AST_NODE(operation);
	left->next   = right;
	
	right->parent = AST_NODE(operation);
	return  operation;	
}

static void
ast_node_assign_free(struct ast_node *node)
{
	struct ast_node_assign *assign;
	
	assign = (struct ast_node_assign *)node;

	ast_node_unref(assign->left);
	ast_node_unref(assign->right);
	
	ufree(assign);
}

struct ast_node_assign*
ast_node_assign(struct ast_node *left, struct ast_node *right)
{
	struct ast_node_assign *assign;
	
	return_val_if_fail(left != NULL, NULL);
	return_val_if_fail(right != NULL, NULL);

	assign = (struct ast_node_assign *)ast_node_new(sizeof(*assign));
	
	assign->left  = left;
	assign->right = right;
	
	AST_NODE(assign)->type  = NODE_TYPE_ASSIGN;
	AST_NODE(assign)->child = left;
	AST_NODE(assign)->destructor = ast_node_assign_free;
	
	left->parent  = AST_NODE(assign);
	left->next    = right;

	right->parent = AST_NODE(assign);
	
	return assign; 
}

static void
ast_node_func_call_free(struct ast_node *node)
{
	int i;
	struct ast_node_func_call *func_call;
	
	func_call = (struct ast_node_func_call *)node;
	
	for(i = 0; i < func_call->nargs; i++)
		ast_node_unref(func_call->args[i]);
	
	ufree(func_call->name);	
	ufree(func_call);
}

struct ast_node_func_call*
ast_node_func_call(char *name)
{
	struct ast_node_func_call *func_call;
	
	func_call = (struct ast_node_func_call *)ast_node_new(sizeof(*func_call));
	
	AST_NODE(func_call)->type = NODE_TYPE_FUNC_CALL;
	AST_NODE(func_call)->destructor = ast_node_func_call_free;

	func_call->name = ustrdup(name);
	
	return func_call;
	
}

void
ast_node_func_call_add_arg(struct ast_node_func_call *func_call, struct ast_node *arg)
{
	int i;

	return_if_fail(func_call != NULL);
	
	i = func_call->nargs++;	

	func_call->args = urealloc(func_call->args, sizeof(struct ast_node *) * func_call->nargs);

	func_call->args[i] = arg;
}

static void
ast_node_end_scope_free(struct ast_node *node)
{
	struct ast_node_end_scope *end_scope;	
	
	end_scope = (struct ast_node_end_scope *)node;
	
	ufree(end_scope);
}

struct ast_node_end_scope*
ast_node_end_scope(void)
{
	struct ast_node_end_scope *end_scope;
	
	end_scope = (struct ast_node_end_scope *)ast_node_new(sizeof(*end_scope));
	
	AST_NODE(end_scope)->type = NODE_TYPE_END_SCOPE;
	AST_NODE(end_scope)->destructor = ast_node_end_scope_free;
	
	return end_scope;
}

void
ast_node_return_free(struct ast_node *node)
{
	struct ast_node_return *_return;

	return_if_fail(node != NULL);

	_return = (struct ast_node_return *)node;
	
	if (_return->ret_val)
		ast_node_unref(_return->ret_val);
	
	ufree(_return);
}

struct ast_node_return*
ast_node_return(struct ast_node *ret_val)
{
	struct ast_node_return *_return;

	_return = (struct ast_node_return *)ast_node_new(sizeof(*_return));
	_return->ret_val = ret_val;

	AST_NODE(_return)->type = NODE_TYPE_RETURN;
	AST_NODE(_return)->child = ret_val;
	AST_NODE(_return)->destructor = ast_node_return_free;

	ret_val->parent = AST_NODE(_return);
	
	return _return;
}
	
struct ast_node_op*
ast_node_rel_op(opcode_type_t opcode, struct ast_node *left, struct ast_node *right)
{
	struct ast_node_op *rel_node;
	
	return_val_if_fail(left != NULL, NULL);
	return_val_if_fail(right != NULL, NULL);
	
	rel_node = (struct ast_node_op *)ast_node_new(sizeof(*rel_node));

	switch(opcode) {
	case OPCODE_LT:
	case OPCODE_LE:
	case OPCODE_GT:
	case OPCODE_GE:
	case OPCODE_EQ:
	case OPCODE_NE:
		rel_node->opcode = opcode;
		break;
	default:
		error(1, "incompatible operation");
		break;	
	}

	rel_node->left  = left;
	rel_node->right = right;

	AST_NODE(rel_node)->type  = NODE_TYPE_REL_OP;
	AST_NODE(rel_node)->child = left;
	AST_NODE(rel_node)->destructor = ast_node_op_free;
	
	left->parent = AST_NODE(rel_node);
	left->next   = right;
	
	right->parent = AST_NODE(rel_node);
	
	return rel_node;
}

struct ast_node_op*
ast_node_logic_op(opcode_type_t opcode, struct ast_node *left, struct ast_node *right)
{
	struct ast_node_op *logic_node;

	return_val_if_fail(left != NULL, NULL);
	return_val_if_fail(right != NULL, NULL);
	
	logic_node = (struct ast_node_op *)ast_node_new(sizeof(*logic_node));

	switch(opcode) {
	case OPCODE_OR:
		logic_node->opcode = opcode;
		AST_NODE(logic_node)->type = NODE_TYPE_AND_OP;
		break;
	case OPCODE_AND:
		logic_node->opcode = opcode;
		AST_NODE(logic_node)->type = NODE_TYPE_OR_OP;
		break;
	default:
		error(1, "incompatible operation");
		break;
	} 

	logic_node->left  = left;
	logic_node->right = right;
	
	AST_NODE(logic_node)->child = left;
	AST_NODE(logic_node)->destructor = ast_node_op_free;

	left->parent = AST_NODE(logic_node);
	left->next   = right;
	
	right->parent = AST_NODE(logic_node);
	
	return logic_node;
}

static void
ast_node_if_free(struct ast_node *node)
{
	struct ast_node_if *if_node;

	return_if_fail(node != NULL);
	
	if_node = (struct ast_node_if *)node;

	ast_node_unref(if_node->expr);
	ast_node_unref(if_node->stmt);
		
	if (if_node->_else)
		ast_node_unref(if_node->_else);
	
	ufree(if_node);
}

struct ast_node_if*
ast_node_if(struct ast_node *expr, struct ast_node *stmt, struct ast_node *_else)
{
	struct ast_node_if *if_node;

	return_val_if_fail(expr != NULL, NULL);
	return_val_if_fail(stmt != NULL, NULL);
		
	if_node = (struct ast_node_if *)ast_node_new(sizeof(*if_node));
	
	if_node->expr  = expr;
	if_node->stmt  = stmt;
	if_node->_else = _else;

	AST_NODE(if_node)->type  = NODE_TYPE_IF;
	AST_NODE(if_node)->child = expr;
	AST_NODE(if_node)->destructor =  ast_node_if_free;

	expr->parent = AST_NODE(if_node); 
	
	return if_node;
}

static void
ast_node_for_free(struct ast_node *node)
{
	struct ast_node_for *for_node;

	return_if_fail(node != NULL);
	
	for_node = (struct ast_node_for *)node;
	
	if (for_node->expr1)
		ast_node_unref(for_node->expr1);
	if (for_node->expr2)
		ast_node_unref(for_node->expr2);
	if (for_node->expr3)
		ast_node_unref(for_node->expr3);
	if (for_node->stmt)
		ast_node_unref(for_node->stmt);
	
	ufree(for_node);
}

struct ast_node_for*
ast_node_for(struct ast_node *expr1, struct ast_node *expr2, struct ast_node *expr3, struct ast_node *stmt)
{
	struct ast_node_for *for_node;
	
	for_node = (struct ast_node_for *)ast_node_new(sizeof(*for_node));
	
	for_node->expr1 = expr1;
	for_node->expr2 = expr2;
	for_node->expr3 = expr3;
	for_node->stmt  = stmt;

	AST_NODE(for_node)->type  = NODE_TYPE_FOR;
	AST_NODE(for_node)->child = expr1;
	AST_NODE(for_node)->destructor = ast_node_for_free;
	
	if (expr1)
		expr1->parent = AST_NODE(for_node);
	if (expr2)
		expr2->parent = AST_NODE(for_node);
	if (expr3)
		expr3->parent = AST_NODE(for_node);
	if (stmt)	
		stmt->parent  = AST_NODE(for_node);
	
	return for_node;	
}

static void
ast_node_while_free(struct ast_node *node)
{
	struct ast_node_while *while_node;
	
	return_if_fail(node != NULL);
	
	while_node = (struct ast_node_while *)node;
	
	ast_node_unref(while_node->expr);
	
	if (while_node->stmt)	
		ast_node_unref(while_node->stmt);
	
	ufree(while_node);
}

struct ast_node_while*
ast_node_while(struct ast_node *expr, struct ast_node *stmt)
{
	struct ast_node_while *while_node;
	
	return_val_if_fail(expr != NULL, NULL);

	while_node = (struct ast_node_while *)ast_node_new(sizeof(*while_node));
	
	while_node->expr = expr;
	while_node->stmt = stmt;
	
	AST_NODE(while_node)->type   = NODE_TYPE_WHILE;
	AST_NODE(while_node)->child  = expr;
	AST_NODE(while_node)->destructor = ast_node_while_free;
	
	expr->parent = AST_NODE(while_node);
	stmt->parent = AST_NODE(while_node);
	
	return while_node;
}

static void
ast_node_break_free(struct ast_node *node)
{
	struct ast_node_break *break_node;
	
	return_if_fail(node != NULL);

	break_node = (struct ast_node_break *)node;	
			
	ufree(break_node);
}

struct ast_node_break*
ast_node_break(void)
{
	struct ast_node_break *break_node;
	
	break_node = (struct ast_node_break *)ast_node_new(sizeof(*break_node));
	
	AST_NODE(break_node)->type = NODE_TYPE_BREAK;
	AST_NODE(break_node)->destructor = ast_node_break_free;
	
	return break_node;
}

static void
ast_node_continue_free(struct ast_node *node)
{
	struct ast_node_continue *continue_node;

	return_if_fail(node != NULL);
	
	continue_node = (struct ast_node_continue *)node;
		
	ufree(continue_node);
}

struct ast_node_continue*
ast_node_continue(void)
{
	struct ast_node_continue *continue_node;
	
	continue_node = (struct ast_node_continue *)ast_node_new(sizeof(*continue_node));
	
	AST_NODE(continue_node)->type = NODE_TYPE_CONTINUE;
	AST_NODE(continue_node)->destructor = ast_node_continue_free;	
	
	return continue_node;
}

static void
ast_node_vector_free(struct ast_node *node)
{
	struct ast_node_vector *vc_node;
	int i;

	return_if_fail(node != NULL);
	
	vc_node = (struct ast_node_vector *)node;

	for (i = 0; i < vc_node->size; i++)
		ast_node_unref(vc_node->elem[i]);
	
	ufree(vc_node);
}

struct ast_node_vector*
ast_node_vector(struct ast_node **elem, int size)
{
	struct ast_node_vector *vc_node;

	return_val_if_fail(elem != NULL, NULL);
	
	vc_node = (struct ast_node_vector *)ast_node_new(sizeof(*vc_node));
	
	AST_NODE(vc_node)->type = NODE_TYPE_VECTOR;
	AST_NODE(vc_node)->destructor = ast_node_vector_free;
	
	vc_node->elem = elem;
	vc_node->size = size;
	
	return vc_node;
}

static void
ast_node_matrix_free(struct ast_node *node)
{
	struct ast_node_matrix  *mx_node;
	int i, len;

	return_if_fail(node != NULL);
	
	mx_node = (struct ast_node_matrix *)node;

	len = mx_node->size1 * mx_node->size2;
		
	for (i = 0; i < len; i++)
		ast_node_unref(mx_node->elem[i]);
	
	ufree(mx_node);
}

struct ast_node_matrix*
ast_node_matrix(struct ast_node **elem, int size1, int size2)
{
	struct ast_node_matrix *mx_node;

	return_val_if_fail(elem != NULL, NULL);
	
	mx_node = (struct ast_node_matrix *)ast_node_new(sizeof(*mx_node));
	
	AST_NODE(mx_node)->type = NODE_TYPE_MATRIX; 
	AST_NODE(mx_node)->destructor = ast_node_matrix_free;

	mx_node->elem   = elem;
	mx_node->size1  = size1;
	mx_node->size2  = size2;

	return mx_node;	
}

void
ast_node_include_free(struct ast_node *node)
{
	struct ast_node_include *inc_node;

	return_if_fail(node != NULL);
	
	inc_node = (struct ast_node_include *)node;
	
	ufree(inc_node->fname);
		
	ufree(inc_node);
}

struct ast_node_include*
ast_node_include(char *fname)
{
	struct ast_node_include *inc_node;

	return_val_if_fail(fname != NULL, NULL);
	
	inc_node = (struct ast_node_include *)ast_node_new(sizeof(*inc_node));
	
	AST_NODE(inc_node)->type = NODE_TYPE_INCLUDE;
	AST_NODE(inc_node)->destructor = ast_node_include_free;
	
	inc_node->fname = ustrdup(fname);
	
	return inc_node;	
}

static void
ast_node_access_free(struct ast_node *node)
{
	struct ast_node_access *ac_node;

	return_if_fail(node != NULL);
		
	ac_node = (struct ast_node_access *)node;
	
	ufree(ac_node->name);
	
	if (ac_node->dims)
		ufree(ac_node->dims);
	
	ufree(ac_node);
}

struct ast_node_access*
ast_node_access(value_t v_type, char *name)
{
	struct ast_node_access *ac_node;
	
	return_val_if_fail(name != NULL, NULL);
	
	ac_node = (struct ast_node_access *)ast_node_new(sizeof(*ac_node));
	
	AST_NODE(ac_node)->type = NODE_TYPE_ACCESS;
	AST_NODE(ac_node)->destructor = ast_node_access_free;
	
	ac_node->v_type = v_type;
	ac_node->name   = ustrdup(name);

	return ac_node;
}

void
ast_node_access_add(struct ast_node_access *ac_node, struct ast_node *dim)
{
	int idx;
	
	return_if_fail(ac_node != NULL);
	
	idx = ac_node->ndims++;	
		
	ac_node->dims = urealloc(ac_node->dims,
				 sizeof(struct ast_node *) * ac_node->ndims);

	ac_node->dims[idx] = dim;
}

static void
ast_node_root_free(struct ast_node *node)
{
	struct ast_node_root *root_node;

	return_if_fail(node != NULL);
	
	root_node = (struct ast_node_root *)node;

	if (AST_NODE(root_node)->child)
		ast_node_unref(node->child);
	
	ufree(root_node);
}

struct ast_node_root*
ast_node_root(struct ast_node *node)
{
	struct ast_node_root *root_node;

	root_node = (struct ast_node_root *)ast_node_new(sizeof(*root_node));
	
	AST_NODE(root_node)->type = NODE_TYPE_ROOT;
	AST_NODE(root_node)->destructor = ast_node_root_free;
	AST_NODE(root_node)->child = node;
	
	return root_node;	
}
