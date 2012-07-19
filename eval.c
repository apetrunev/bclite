#include <stdio.h>
#include <stdlib.h>

#include <gsl/gsl_blas.h>
 
#include "eval.h"
#include "macros.h"
#include "umalloc.h"
#include "misc.h"
#include "libm.h"

#define err_msg_ret(ret, fmt, arg...) \
do { \
	message(fmt, ##arg); \
	return (ret); \
} while(0)	

struct eval*
eval_new(tag_type_t tag, value_t v_type, void *val)
{
	struct eval *eval;

	return_val_if_fail(val != NULL, NULL);
	
	eval = umalloc0(sizeof(*eval));
		
	switch(tag) {
	case TAG_CONST:
	case TAG_SYMBOL:
		eval->tag = tag;
		break;
	default:
		error(1, "unknown tag value");
	}

	switch(v_type) {
	case VALUE_TYPE_DIGIT:
		eval->v_type = v_type;
		eval->digit  = *(double *)val;
		break;
	case VALUE_TYPE_STRING:
		eval->v_type = v_type;
		eval->string =ustrdup((char *)val);
		break;
	case VALUE_TYPE_VECTOR:
		eval->v_type = v_type;
		eval->vector = (gsl_vector *)val;
		break;	
	case VALUE_TYPE_MATRIX:
		eval->v_type = v_type;
		eval->matrix = (gsl_matrix *)val;
		break;
	case VALUE_TYPE_VOID:	
		eval->v_type = v_type;
		break;
	default:
		error(1, "unknown value type");
	}
	
	return eval;	
}

struct eval*
eval_add_op(struct eval *a, struct eval *b, opcode_type_t op)
{
	struct eval *c;
	gsl_vector *vc;
	gsl_matrix *mx;
	double dg;
	tag_type_t tag;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	tag = TAG_CONST;

	switch(a->v_type) {
	case VALUE_TYPE_DIGIT:
		switch(b->v_type) { 
		case VALUE_TYPE_DIGIT: /* digit op digit */
			dg = libm_digit_op(a->digit, b->digit, op);
			c  = eval_new(tag, VALUE_TYPE_DIGIT, &dg);
			break;
		case VALUE_TYPE_VECTOR: /* digit op vector */
			vc = libm_digit_vector_add_op(a->digit, b->vector, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;
		case VALUE_TYPE_MATRIX: /* digit op matrix */
			mx = libm_digit_matrix_add_op(a->digit, b->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}	
		break;
	case VALUE_TYPE_VECTOR:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT: /* vector op digit */
			vc = libm_digit_vector_add_op(b->digit, a->vector, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;			
		case VALUE_TYPE_VECTOR: /* vector op vector */
			vc = libm_vector_add_op(a->vector, b->vector, op);
			if (!vc)
				return NULL;
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);	
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	case VALUE_TYPE_MATRIX: 
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT: /* matrix op digit */
			mx = libm_digit_matrix_add_op(b->digit,a->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		case VALUE_TYPE_MATRIX: /* matrix op matrix */
			mx = libm_matrix_add_op(a->matrix, b->matrix, op);
			if (!mx)
				return NULL;
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);	
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	default:
		err_msg_ret(NULL, "incompatible value type");
	}
		
	return c;
}

struct eval*
eval_mult_op(struct eval *a, struct eval *b, opcode_type_t op)
{
	struct eval *c;
	gsl_vector *vc;
	gsl_matrix *mx;
	double dg;
	tag_type_t tag;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	tag = TAG_CONST;
	
	switch(a->v_type) {
	case VALUE_TYPE_DIGIT:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			dg = libm_digit_op(a->digit, b->digit, op);
			c  = eval_new(tag, VALUE_TYPE_DIGIT, &dg);
			break;				
		case VALUE_TYPE_VECTOR:
			vc = libm_digit_vector_mult_op(a->digit, b->vector, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;
		case VALUE_TYPE_MATRIX:
			mx = libm_digit_matrix_mult_op(a->digit, b->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		default:	
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	case VALUE_TYPE_VECTOR:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			vc = libm_vector_digit_mult_op(a->vector, b->digit, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;	
		case VALUE_TYPE_VECTOR:
			dg = libm_vector_mult_op(a->vector, b->vector, op);
			c  = eval_new(tag, VALUE_TYPE_DIGIT, &dg);
			break;	
		case VALUE_TYPE_MATRIX:
			vc = libm_vector_matrix_mult_op(a->vector, b->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;
		default:	
			err_msg_ret(NULL, "incompatible value type");
		}
		break;	
	case VALUE_TYPE_MATRIX:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:	
			mx = libm_matrix_digit_mult_op(a->matrix, b->digit, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		case VALUE_TYPE_VECTOR:
			vc = libm_matrix_vector_mult_op(a->matrix, b->vector, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;
		case VALUE_TYPE_MATRIX:
			mx = libm_matrix_mult_op(a->matrix, b->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;	
	default:
		err_msg_ret(NULL, "incompatible value type");
	}
	
	return c;
}

struct eval*
eval_logic_op(struct eval *a, struct eval *b, opcode_type_t op)
{
	struct eval *c;
	double dg;
	tag_type_t tag;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	tag = TAG_CONST;

	switch(a->v_type) {
	case VALUE_TYPE_DIGIT:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			dg = libm_digit_op(a->digit, b->digit, op);
			break;	
		case VALUE_TYPE_VECTOR:
			dg = libm_digit_vector_logic_op(a->digit, b->vector, op);
			break;
		case VALUE_TYPE_MATRIX:
			dg = libm_digit_matrix_logic_op(a->digit, b->matrix, op);
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	case VALUE_TYPE_VECTOR:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			dg = libm_digit_vector_logic_op(b->digit, a->vector, op);
			break;			
		case VALUE_TYPE_VECTOR:
			dg = libm_vector_logic_op(a->vector, b->vector, op);
			break;	
		default:
			err_msg_ret(NULL, "incompatoble value type");
		}
		break;
	case VALUE_TYPE_MATRIX:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			dg = libm_digit_matrix_logic_op(b->digit, a->matrix, op);
			break;	
		case VALUE_TYPE_MATRIX:
			dg = libm_matrix_logic_op(a->matrix, b->matrix, op);
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	default:
		err_msg_ret(NULL, "incompatible value type");	
	}
	
	c = eval_new(tag, VALUE_TYPE_DIGIT, &dg);
	
	return c;
}
	
struct eval* 
eval_rel_op(struct eval *a, struct eval *b, opcode_type_t op)
{
	struct eval *c;
	gsl_vector *vc;
	gsl_matrix *mx;
	double dg;
	tag_type_t tag;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	tag = TAG_CONST;

	switch(a->v_type) {
	case VALUE_TYPE_DIGIT:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			dg = libm_digit_op(a->digit, b->digit, op);
			c  = eval_new(tag, VALUE_TYPE_DIGIT, &dg);
			break;
		case VALUE_TYPE_VECTOR:
			vc = libm_digit_vector_rel_op(a->digit, b->vector, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;
		case VALUE_TYPE_MATRIX:
			mx = libm_digit_matrix_rel_op(a->digit, b->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	case VALUE_TYPE_VECTOR:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			vc = libm_digit_vector_rel_op(a->digit, b->vector, op);
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc); 
			break;
		case VALUE_TYPE_VECTOR:
			vc = libm_vector_rel_op(a->vector, b->vector, op);
			if (!vc)
				return NULL;
			c  = eval_new(tag, VALUE_TYPE_VECTOR, vc);
			break;
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	case VALUE_TYPE_MATRIX:
		switch(b->v_type) {
		case VALUE_TYPE_DIGIT:
			mx = libm_digit_matrix_rel_op(b->digit, a->matrix, op);
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;
		case VALUE_TYPE_MATRIX:
			mx = libm_matrix_rel_op(a->matrix, b->matrix, op);
			if (!mx)
				return NULL;
			c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
			break;	
		default:
			err_msg_ret(NULL, "incompatible value type");
		}
		break;
	default:
		err_msg_ret(NULL, "incompatible value type");
	}
	
	return c;
}

struct eval*
eval_exp_op(struct eval *a, struct eval *b, opcode_type_t op)
{
	struct eval *c;
	gsl_matrix *mx;
	tag_type_t tag;
	double dg;
	int pow;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	tag = TAG_CONST;

	switch(b->v_type) {
	case VALUE_TYPE_DIGIT:
		pow = (int)b->digit;	
		break;
	default:
		err_msg_ret(NULL, "power must be a digit");
	}
	
	switch(a->v_type) {
	case VALUE_TYPE_DIGIT:
		dg = libm_digit_op(a->digit, pow, op);
		c  = eval_new(tag, VALUE_TYPE_DIGIT, &dg);
		break;	
	case VALUE_TYPE_MATRIX:
		mx = libm_matrix_exp(a->matrix, pow);
		c  = eval_new(tag, VALUE_TYPE_MATRIX, mx);
		break;
	default:
		err_msg_ret(NULL, "in this operation value"
				  " must be a mutrix(square) or a digit");
	}	
	
	return c;
}

static void
matrix_fprintf(FILE *stream, const gsl_matrix *mx)
{
	int i, j, row, col;

	return_if_fail(stream != NULL);
	return_if_fail(mx != NULL);
	
	row = mx->size1;
	col = mx->size2;

	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) 
			fprintf(stream, "%f ", gsl_matrix_get(mx, i, j));
			
		printf("\n");
	}
}

void
eval_print(struct eval *eval)
{
	return_if_fail(eval != NULL);
	
	switch(eval->v_type) {
	case VALUE_TYPE_DIGIT:
		fprintf(stdout, "%f\n", eval->digit);
		break;
	case VALUE_TYPE_STRING:
		fprintf(stdout, "%s\n", eval->string);
		break;
	case VALUE_TYPE_VECTOR:
		gsl_vector_fprintf(stdout, eval->vector, "%f");
		break;
	case VALUE_TYPE_MATRIX:
		matrix_fprintf(stdout, eval->matrix);
		break;
	case VALUE_TYPE_VOID:
		break;
	default:
		fprintf(stderr, "error: unknown value type\n");
		break;
	}
}
	
void
eval_free(struct eval *eval)
{
	return_if_fail(eval != NULL);
	
	if (eval->tag == TAG_SYMBOL)
		goto free;
	
	switch(eval->v_type) {
	case VALUE_TYPE_STRING:
		ufree(eval->string);
		break;
	case VALUE_TYPE_VECTOR:
		gsl_vector_free(eval->vector);
		break;
	case VALUE_TYPE_MATRIX:
		gsl_matrix_free(eval->matrix);
		break;
	default:	
		break;
	}
free:	
	ufree(eval);
}



