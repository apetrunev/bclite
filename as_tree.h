#ifndef AS_TREE_
#define AS_TREE_

#include "common.h"

struct ast_node;

typedef enum {
	NODE_TYPE_ID,
	NODE_TYPE_CONST,
	NODE_TYPE_ADD_OP,
	NODE_TYPE_MULT_OP,
	NODE_TYPE_AND_OP,
	NODE_TYPE_OR_OP,
	NODE_TYPE_REL_OP,
	NODE_TYPE_EXP_OP,
	NODE_TYPE_ASSIGN,
	NODE_TYPE_FUNC_CALL,
	NODE_TYPE_END_SCOPE,
	NODE_TYPE_RETURN,	
	NODE_TYPE_IF,
	NODE_TYPE_FOR,
	NODE_TYPE_WHILE,
	NODE_TYPE_BREAK,
	NODE_TYPE_CONTINUE,
	NODE_TYPE_ACCESS,
	NODE_TYPE_STUB,
	NODE_TYPE_ARRAY,
	NODE_TYPE_MATRIX,
	NODE_TYPE_VECTOR,
	NODE_TYPE_INCLUDE,
	NODE_TYPE_ROOT,
	NODE_TYPE_UNKNOWN
} node_type_t;

typedef enum {
	OPCODE_UNKNOWN,
	OPCODE_EXP,
	OPCODE_SUB,
	OPCODE_ADD,
	OPCODE_MULT,
	OPCODE_DIV,
	OPCODE_OR,
	OPCODE_AND,
	OPCODE_LT,
	OPCODE_GT,
	OPCODE_LE,
	OPCODE_GE,
	OPCODE_NE,	
	OPCODE_EQ
} opcode_type_t;

#define AST_NODE(obj) ((struct ast_node *)(obj))
#define AST_CONST(obj) ((struct ast_node_const *)(obj))

typedef void ( *destructor_t)(struct ast_node* );


struct ast_node {
	node_type_t type;
	struct ast_node *parent;
	struct ast_node *next;
	struct ast_node *child;
	destructor_t destructor;
};	

struct ast_node_root {
	struct ast_node base;
};

struct ast_node_assign {
	struct ast_node base;
	struct ast_node *left;
	struct ast_node *right;
};

struct ast_node_const {
	struct ast_node base;
	value_t		v_type;
	union {
		double	digit;
		char	*string;
	};
};

struct ast_node_id {
	struct ast_node base;
	char *name;
};

struct ast_node_op {
	struct ast_node base;
	struct ast_node *left;
	struct ast_node *right;
	opcode_type_t opcode;
};

struct ast_node_func_call {
	struct ast_node base;
	char *name;
	int nargs;
	struct ast_node **args;
};


struct ast_node_stub {
	struct ast_node base;
};

struct ast_node_end_scope {
	struct ast_node base;
};

struct ast_node_return {
	struct ast_node base;
	struct ast_node *ret_val;
};

struct ast_node_if {
	struct ast_node base;
	struct ast_node *expr;
	struct ast_node *stmt;
	struct ast_node *_else;
};

struct ast_node_for {
	struct ast_node base;
	struct ast_node *expr1;
	struct ast_node *expr2;
	struct ast_node *expr3;
	struct ast_node *stmt;
};

struct ast_node_while {
	struct ast_node base;
	struct ast_node *expr;
	struct ast_node *stmt;
};

struct ast_node_break {
	struct ast_node base;
};

struct ast_node_continue {
	struct ast_node base;
};

struct ast_node_vector {
	struct ast_node base;
	struct ast_node **elem;
	int size;
};

struct ast_node_matrix {
	struct ast_node base;
	struct ast_node **elem;
	int size1;
	int size2;
};

struct ast_node_include {
	struct ast_node base;
	char *fname;
};

struct ast_node_access {
	struct ast_node base;
	value_t		v_type;
	char		*name;
	int 		ndims;
	struct ast_node	**dims;	
};

struct ast_node_stub*
ast_node_stub(void);

void
ast_node_unref(struct ast_node *node);

struct ast_node_const*
ast_node_const(value_t v_type, void *data);

struct ast_node_id*
ast_node_id(char *name);

struct ast_node_op*
ast_node_op(char op_helper, struct ast_node *left, struct ast_node *right);

struct ast_node_assign*
ast_node_assign(struct ast_node *left, struct ast_node *right);

struct ast_node_end_scope*
ast_node_end_scope(void);

struct ast_node_func_call*
ast_node_func_call(char *name);

void
ast_node_func_call_add_arg(struct ast_node_func_call *func_call, struct ast_node *arg);

struct ast_node_return*
ast_node_return(struct ast_node *ret_val);

struct ast_node_op*
ast_node_rel_op(opcode_type_t opcode, struct ast_node *left, struct ast_node *right);

struct ast_node_op*
ast_node_logic_op(opcode_type_t opcode, struct ast_node *left, struct ast_node *right);

struct ast_node_if*
ast_node_if(struct ast_node *expr, struct ast_node *stmt, struct ast_node *_else);

struct ast_node_for*
ast_node_for(struct ast_node *expr1, struct ast_node *expr2, struct ast_node *expr3, struct ast_node *stmt);

struct ast_node_while*
ast_node_while(struct ast_node *expr, struct ast_node *stmt);

struct ast_node_break*
ast_node_break(void);

struct ast_node_continue*
ast_node_continue(void);

struct ast_node_vector*
ast_node_vector(struct ast_node **elem, int size);

struct ast_node_matrix*
ast_node_matrix(struct ast_node **elem, int size1, int size2);

struct ast_node_include*
ast_node_include(char *fname);

struct ast_node_access*
ast_node_access(value_t v_type, char *name);

void
ast_node_access_add(struct ast_node_access *ac_node, struct ast_node *idx);

struct ast_node_root*
ast_node_root(struct ast_node *node);

#endif /* AS_TREE_ */
