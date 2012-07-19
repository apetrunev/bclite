#include <stdlib.h>
#include <math.h>

#include <gsl/gsl_matrix.h>

#include "libcall.h"
#include "umalloc.h"
#include "macros.h"
#include "misc.h"

#define err_msg(fmt, arg...) \
do { \
	message(fmt, ##arg); \
} while(0)
	
int
libcall_sin(struct function *func, value_t *v_type, void **result)
{
	double *dg;

	return_val_if_fail(func != NULL, FALSE);
	
	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {	
		err_msg("error: incompatible argument type");
		return FALSE;
	}
	
	dg  = umalloc(sizeof(double));
	*dg = sin(func->args[0]->digit);

	*v_type = VALUE_TYPE_DIGIT;	
	*result = dg;
	
	return TRUE;
}

int
libcall_cos(struct function *func, value_t *v_type, void **result)
{
	double *dg;
		
	return_val_if_fail(func != NULL, FALSE);
	
	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: incompatible argument type");
		return FALSE;
	}
	
	dg  = umalloc(sizeof(double));
	*dg = cos(func->args[0]->digit);

	*v_type = VALUE_TYPE_DIGIT;	
	*result = dg;
	
	return TRUE;
}

int 
libcall_ln(struct function *func, value_t *v_type, void **result)
{
	double *dg;
	
	return_val_if_fail(func != NULL, FALSE);
	
	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: incompatible argument type");
		return FALSE;
	}
	
	dg  = umalloc(sizeof(double));
	*dg = log(func->args[0]->digit);

	*v_type = VALUE_TYPE_DIGIT;	
	*result = dg;
	
	return TRUE;

}

int 
libcall_exp(struct function *func, value_t *v_type, void **result)
{
	double *dg;
	
	return_val_if_fail(func != NULL, FALSE);
	
	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: incompatible argument type");
		return FALSE;
	}
	
	dg  = umalloc(sizeof(double));
	*dg = exp(func->args[0]->digit);

	*v_type = VALUE_TYPE_DIGIT;	
	*result = dg;
	
	return TRUE;
}

int
libcall_tan(struct function *func, value_t *v_type, void **result)
{
	double *dg;
	
	return_val_if_fail(func != NULL, FALSE);
	
	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: incompatible argument type");
		return FALSE;
	}
	
	dg  = umalloc(sizeof(double));
	*dg = tan(func->args[0]->digit);

	*v_type = VALUE_TYPE_DIGIT;	
	*result = dg;
	
	return TRUE;

}

int
libcall_matrix(struct function *func, value_t *v_type, void **result)
{
	gsl_matrix *mx;	
	unsigned int nargs, i;
	unsigned int row, col;

	return_val_if_fail(func != NULL, FALSE);	

	nargs = func->nargs;
	
	for (i = 0; i < nargs; i++) {
		if (func->args[i]->v_type != VALUE_TYPE_DIGIT) {
			err_msg("error: arguments in the `matrix()' must be the digits");
			return FALSE;
		}
	}	
	
	row = func->args[0]->digit;
	col = func->args[1]->digit;

	mx = gsl_matrix_calloc(row, col);
	
	*v_type = VALUE_TYPE_MATRIX;
	*result = mx;
	
	return TRUE;
}

int
libcall_vector(struct function *func, value_t *v_type, void **result)
{
	gsl_vector *vc;
	unsigned int len;

	return_val_if_fail(func != NULL, FALSE);

	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: arguments in the `vector()' must be the digits");
		return FALSE;
	}
	
	len = func->args[0]->digit;
	vc  = gsl_vector_calloc(len);
	
	*v_type = VALUE_TYPE_VECTOR;
	*result = vc;
	
	return TRUE;
}

int
libcall_sqrt(struct function *func, value_t *v_type, void **result)
{
	double *dg;
	
	return_val_if_fail(func != NULL, FALSE);
	
	if (func->args[0]->v_type != VALUE_TYPE_DIGIT) {
		err_msg("error: arguments in the `vector()' must be the digits");
		return FALSE;
	}

	dg  = umalloc(sizeof(double));
	*dg = sqrt(func->args[0]->digit);

	*v_type = VALUE_TYPE_DIGIT;	
	*result = dg;
	
	return TRUE;
}
