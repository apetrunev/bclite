#ifndef EVAL_H_
#define EVAL_H_

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

#include "common.h"
#include "as_tree.h"

typedef enum {
	TAG_CONST,
	TAG_SYMBOL
} tag_type_t;

struct eval {
	tag_type_t		tag;	
	value_t			v_type;
	union {
		double		digit;
		char		*string;
		gsl_vector	*vector;
		gsl_matrix	*matrix;
	};
};

struct eval*
eval_new(tag_type_t tag, value_t v_type, void *val);

struct eval*
eval_add_op(struct eval *a, struct eval *b, opcode_type_t op);

struct eval*
eval_mult_op(struct eval *a, struct eval *b, opcode_type_t op);

struct eval*
eval_logic_op(struct eval *a, struct eval *b, opcode_type_t op);

struct eval* 
eval_rel_op(struct eval *a, struct eval *b, opcode_type_t op);

struct eval*
eval_exp_op(struct eval *a, struct eval *b, opcode_type_t op);

void
eval_print(struct eval *eval);
	
void
eval_free(struct eval *eval);

#endif /* EVAL_H_ */
