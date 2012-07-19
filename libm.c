#include <stdio.h>
#include <stdlib.h>

#include <gsl/gsl_linalg.h>

#include "libm.h"
#include "misc.h"
#include "macros.h"
#include "umalloc.h"

#define err_msg_ret(ret, fmt, arg...) \
do { \
	message(fmt, ##arg); \
	return (ret); \
} while(0)

#define err_msg(fmt, arg...) \
do { \
	message(fmt, ##arg); \
} while (0)

static int
vector_compare_size(gsl_vector *a, gsl_vector *b)
{
	if (a->size == b->size)
		return TRUE;

	return FALSE;	
}

static int
matrix_compare_size(gsl_matrix *a, gsl_matrix *b)
{
	if (a->size1 != b->size2)
		return FALSE;

	if (a->size2 != b->size2)
		return FALSE;
	
	return TRUE;
}

static double
vector_and(gsl_vector *a, gsl_vector *b)
{
	double x, y, dg;
	int i;

	for (i = 0; i < a->size; i++) {
		x  = gsl_vector_get(a, i);
		y  = gsl_vector_get(b, i);	
		dg = x && y;
		if (dg == 0.0)
			break;
	} 
	
	return dg;
}

static double
vector_or(gsl_vector *a, gsl_vector *b)
{
	double x, y, dg;
	int i;
	
	for (i = 0; i < a->size; i++) {
		x  = gsl_vector_get(a, i);
		y  = gsl_vector_get(b, i);
		dg = x || y;
		if (dg == 1.0)
			break;
	}
	
	return dg;
}

static double
matrix_and(gsl_matrix *a, gsl_matrix *b)
{
	double x, y, dg;
	int i, j;
	
	for (i = 0; i < a->size1; i++) {
		for (j = 0; j < a->size2; j++) {
			x  = gsl_matrix_get(a, i ,j);
			y  = gsl_matrix_get(b, i, j);
			dg = x && y;
			if (dg == 0.0)
				break; 
		}
	}

	return dg;
}

static double 
matrix_or(gsl_matrix *a, gsl_matrix *b)
{
	double x, y, dg;
	int i, j;
	
	for (i = 0; i < a->size1; i++) {
		for (j = 0; j < a->size2; j++) {
			x  = gsl_matrix_get(a, i, j);
			y  = gsl_matrix_get(b, i, j);
			dg = x || y;
			if (dg == 1.0)
				break;
		}
	}
	return dg;
}

static int 
matrix_is_square(gsl_matrix *mx)
{
	return_val_if_fail(mx != NULL, -1);
	
	if (mx->size1 == mx->size2)
		return TRUE;
	
	return FALSE;
}

static void
vector_init(gsl_vector **dest, gsl_vector *src)
{
	gsl_vector *vc;

	vc = gsl_vector_alloc(src->size);

	gsl_vector_memcpy(vc, src);
	
	*dest = vc;
}

static void
matrix_init(gsl_matrix **dest, gsl_matrix *src)
{
	gsl_matrix *mx;

	mx = gsl_matrix_alloc(src->size1, src->size2);

	gsl_matrix_memcpy(mx, src);
	
	*dest = mx;
}

double
libm_digit_op(double a, double b, opcode_type_t op)
{
	int pow;
	double dg;

	switch(op) {
	case OPCODE_ADD:
		dg = a + b;
		break;
	case OPCODE_SUB:
		dg = a - b;
		break;
	case OPCODE_MULT:
		dg = a * b;
		break;
	case OPCODE_DIV:
		if (b == 0.0)
			err_msg("division by zero");	
		else 
			dg = a / b;
		break;
	case OPCODE_AND:
		dg = a && b;
		break;
	case OPCODE_OR:
		dg = a || b;
		break;
	case OPCODE_LT:
		dg = a < b;
		break;
	case OPCODE_LE:
		dg = a <= b;
		break;
	case OPCODE_GT:
		dg = a > b;
		break;
	case OPCODE_GE:
		dg = a >= b;
		break;
	case OPCODE_EQ:
		dg = a == b;
		break;
	case OPCODE_NE:
		dg = a != b;
		break;
	case OPCODE_EXP:
		pow = (int)b;
		dg  = (pow & 0x1) ? a : 1.0;
		pow >>= 1;
	
		while(pow) {
			a *= a;
			if (pow & 0x1) 
				dg = (dg == 1) ? a : a*dg;
			pow >>= 1;
		}		
		break;
	default:
		error(1, "unknown operation");
	}
	
	return dg;	
}

/* digit op Vector */
gsl_vector*
libm_digit_vector_add_op(double a, gsl_vector *b, opcode_type_t op)
{
	gsl_vector *vc;

	return_val_if_fail(b != NULL, NULL);
	
	vector_init(&vc, b);

	switch(op) {
	case OPCODE_ADD:
		gsl_vector_add_constant(vc, a);
		break;
	case OPCODE_SUB:
		gsl_vector_scale(vc, -1.0);
		gsl_vector_add_constant(vc, a);
		break;
	default:
		error(1, "wrong operation type");
	}
	
	return vc;
}

gsl_vector*
libm_digit_vector_mult_op(double a, gsl_vector *b, opcode_type_t op)
{
	gsl_vector *vc;

	return_val_if_fail(b != NULL, NULL);
	
	switch(op) {
	case OPCODE_MULT:
		vector_init(&vc, b);		
		gsl_vector_scale(vc, a);
		break;
	case OPCODE_DIV:
		err_msg_ret(NULL, "nonconformant argument");
	default:
		error(1, "wrong operation type");
	}	
		
	return vc;
}

double
libm_digit_vector_logic_op(double a, gsl_vector *b, opcode_type_t op)
{
	double dg, x;
	int i;

	return_val_if_fail(b != NULL, -1.0);
	
	switch(op) {
	case OPCODE_AND:
		for (i = 0; i < b->size; i++) {
			x  = gsl_vector_get(b, i);
			dg = libm_digit_op(a, x, op);
			if (dg == 0.0)
				break;
		}
		break;				
	case OPCODE_OR:
		for (i = 0; i < b->size; i++) {
			x  = gsl_vector_get(b, i);
			dg = libm_digit_op(a, x, op);
			if (dg == 1.0)
				break;
		}
		break;
	default:
		error(1, "wrong operation type");	
	}
	
	return dg;
}

gsl_vector*
libm_digit_vector_rel_op(double a, gsl_vector *b, opcode_type_t op)
{
	gsl_vector *vc;
	double x;
	int i;

	return_val_if_fail(b != NULL, NULL);
	
	vc = gsl_vector_alloc(b->size);
	
	switch(op) {
	case OPCODE_LT:
	case OPCODE_LE:
	case OPCODE_GT:
	case OPCODE_GE:
	case OPCODE_EQ:
	case OPCODE_NE:
		for (i = 0; i < vc->size; i++) {
			x = gsl_vector_get(vc, i);
			x = libm_digit_op(a, x, op);
			gsl_vector_set(vc, i, x);
		}
		break;
	default:	
		error(1, "nonconformant operation");
	}
	
	return vc;	
}	

/* Vector op digit */
gsl_vector*
libm_vector_digit_mult_op(gsl_vector *a, double b, opcode_type_t op)
{
	gsl_vector *vc;

	return_val_if_fail(a != NULL, NULL);
		
	vector_init(&vc, a);
	
	switch(op) {
	case OPCODE_MULT:
		gsl_vector_scale(vc, b);
		break;
	case OPCODE_DIV:
		if (b == 0.0) {	
			gsl_vector_free(vc);
			err_msg_ret(NULL, "division by zero");
		} else {
			gsl_vector_scale(vc, 1 / b);
		}
		break;
	default:
		error(1, "unconformant operation");
	}
	
	return vc;
}

/* digit op Matrix */
gsl_matrix*
libm_digit_matrix_add_op(double a, gsl_matrix *b, opcode_type_t op)
{
	gsl_matrix *mx;

	return_val_if_fail(b != NULL, NULL);
	
	matrix_init(&mx, b);

	switch(op) {
	case OPCODE_ADD:
		gsl_matrix_add_constant(mx, a);
		break;
	case OPCODE_SUB:
		gsl_matrix_scale(mx, -1.0);
		gsl_matrix_add_constant(mx, a);
		break;
	default:
		error(1, "unconformant operation");
	}
	
	return mx;
}

gsl_matrix*
libm_digit_matrix_mult_op(double a, gsl_matrix *b, opcode_type_t op)
{
	gsl_matrix *mx;

	return_val_if_fail(b != NULL, NULL);
			
	switch(op) {
	case OPCODE_MULT:	
		matrix_init(&mx, b);
		gsl_matrix_scale(mx, a);
		break;
	case OPCODE_DIV:
		err_msg_ret(NULL, "non conformant arguments");
	default:
		error(1, "nonconformant operation");
	}
	
	return mx;
}
	
double
libm_digit_matrix_logic_op(double a, gsl_matrix *b, opcode_type_t op)
{
	double dg, x;
	int i, j;

	return_val_if_fail(b != NULL, -1.0);
	
	switch(op) {
	case OPCODE_AND:
		for (i = 0; i < b->size1; i++) {
			for (j = 0; j < b->size2; j++) {
				x  = gsl_matrix_get(b, i, j);
				dg = libm_digit_op(a, x, op);
				if (dg == 0.0)
					break;
			}
		}
		break;		
	case OPCODE_OR:
		for (i = 0; i < b->size1; i++) {
			for (j = 0; j < b->size2; j++) {
				x  = gsl_matrix_get(b, i, j);
				dg = libm_digit_op(a, x, op);
				if (dg == 1.0)
					break;
			}
		}
		break;
	default:
		error(1, "nonconformant operation");
	}
	
	return dg;
}

gsl_matrix*
libm_digit_matrix_rel_op(double a, gsl_matrix *b, opcode_type_t op)
{	
	gsl_matrix *mx;
	double x;
	int i, j;

	return_val_if_fail(b != NULL, NULL);
	
	matrix_init(&mx, b);
	
	switch(op) {
	case OPCODE_LT:
	case OPCODE_LE:
	case OPCODE_GT:
	case OPCODE_GE:
	case OPCODE_EQ:
	case OPCODE_NE:
		for (i = 0; i < mx->size1; i++) {
			for (j = 0; j < mx->size2; j++) {
				x  = gsl_matrix_get(mx, i, j);
				x  = libm_digit_op(a, x, op);
				gsl_matrix_set(mx, i, j, x);
			}
		}
		break;
	default:
		error(1, "nonconformant operation");
	}
	
	return mx;
}

/* Matrix op digit */
gsl_matrix*
libm_matrix_digit_mult_op(gsl_matrix *a, double b, opcode_type_t op)
{
	gsl_matrix *mx;

	return_val_if_fail(a != NULL, NULL);
	
	matrix_init(&mx, a);

	switch(op) {
	case OPCODE_MULT:
		gsl_matrix_scale(mx, b);
		break;
	case OPCODE_DIV:
		if (b == 0.0) {
			gsl_matrix_free(mx);
			err_msg_ret(NULL, "division by zero");
		} else {
			gsl_matrix_scale(mx, 1 / b);
		}
		break;
	default:
		error(1, "nonconformant operation");
	}	
		
	return mx;		
}

/* Vector op Vector */
gsl_vector*
libm_vector_add_op(gsl_vector *a, gsl_vector *b, opcode_type_t op)
{
	gsl_vector *vc;
	int ok;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	ok = vector_compare_size(a, b);
	
	if (!ok)
		err_msg_ret(NULL, "error: nonconformant arguments");

	vector_init(&vc, a);

	switch(op) {
	case OPCODE_ADD:
		gsl_vector_add(vc, b);
		break;	
	case OPCODE_SUB:
		gsl_vector_sub(vc, b);
		break;
	default:
		error(1, "nonconformant operation");
	}	
	
	return vc;
}

double
libm_vector_mult_op(gsl_vector *a, gsl_vector *b, opcode_type_t op)
{
	double dg, x, y;
	int ok;

	return_val_if_fail(a != NULL, -1.0);
	return_val_if_fail(b != NULL, -1.0);
	
	ok = vector_compare_size(a, b);

	if (!ok)	
		err_msg_ret(-1.0, "noncormant argument");

	switch(op) {
	case OPCODE_MULT:
		gsl_blas_ddot(a, b, &dg);
		break;
	case OPCODE_DIV:
		ok = gsl_vector_isnull(b);

		if (!ok)
			err_msg_ret(-1.0, "division by zero");	
		/*
		 * a / b == (a * b) / (b * b)
		 * x = a * b
		 * y = b * b
		 */
		gsl_blas_ddot(a, b, &x);
		gsl_blas_ddot(b, b, &y);
		dg = x / y;
		break;
	default:
		error(1, "nonconformant operation");
	}

	return dg;
}

double
libm_vector_logic_op(gsl_vector *a, gsl_vector *b, opcode_type_t op)
{
	double dg;
	int ok;

	return_val_if_fail(a != NULL, -1.0);
	return_val_if_fail(b != NULL, -1.0);
	
	ok = vector_compare_size(a, b);
	
	if (!ok)
		err_msg_ret(-1.0, "nonconformant size");

	switch(op) {
	case OPCODE_AND:
		dg = vector_and(a, b);
		break;
	case OPCODE_OR:
		dg = vector_or(a, b);
		break;
	default:
		error(1, "nonconformant operation");
	}	
	
	return dg;
}

gsl_vector*
libm_vector_rel_op(gsl_vector *a, gsl_vector *b, opcode_type_t op)
{
	gsl_vector *vc;
	double dg, x, y;
	int i, ok;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	ok = vector_compare_size(a, b);
	
	if (!ok)
		err_msg_ret(NULL, "nonconformant arguments");

	vc = gsl_vector_alloc(a->size);
	
	switch(op) {
	case OPCODE_LT:
	case OPCODE_LE:
	case OPCODE_GT:
	case OPCODE_GE:
	case OPCODE_EQ:
	case OPCODE_NE:
		for (i = 0; i < a->size; i++) {
			x  = gsl_vector_get(a, i);
			y  = gsl_vector_get(b, i);			
			dg = libm_digit_op(x, y, op);
			gsl_vector_set(vc, i, dg);	
		}
		break;	
	default:
		error(1, "nonconformant operation");
	}
	
	return vc;
}

/* Vector op Matrix */
gsl_vector*
libm_vector_matrix_mult_op(gsl_vector *a, gsl_matrix *b, opcode_type_t op)
{
	gsl_permutation *pm;
	gsl_matrix *mx;
	gsl_vector *vc;
	int err, ok, signum;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);	
	
	switch(op) {	
	case OPCODE_MULT:
		vc  = gsl_vector_alloc(b->size1);
		err = gsl_blas_dgemv(CblasTrans, 1.0, b, a, 0.0, vc);
		if (err) {
			gsl_vector_free(vc);
			return NULL;
		}
		break;
	case OPCODE_DIV:
		ok = matrix_is_square(b);
		if (!ok) {
			fprintf(stderr, "error: matrix is not square\n");
			return NULL;	
		}
		
		pm = gsl_permutation_alloc(b->size1);
		vc = gsl_vector_alloc(b->size1);
		
		matrix_init(&mx, b);

		gsl_linalg_LU_decomp(mx, pm, &signum);
		gsl_linalg_LU_solve(mx, pm, a, vc);
			
		gsl_permutation_free(pm);
		gsl_matrix_free(mx);		
		 
		break;
	default:
		error(1, "nonconformant operation");
	}
	
	return vc;		
}

/* Matrix op Vector */
gsl_vector*
libm_matrix_vector_mult_op(gsl_matrix *a, gsl_vector *b, opcode_type_t op)
{
	gsl_vector *vc;
	int err;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	switch(op) {
	case OPCODE_MULT:
		vc  = gsl_vector_alloc(b->size);
		err = gsl_blas_dgemv(CblasNoTrans, 1.0, a, b, 0.0, vc);
		if (err) {
			gsl_vector_free(vc);
			return NULL;
		}
		break;
	case OPCODE_DIV:
		err_msg_ret(NULL, "wrong paprameters operation\n");	
		break;
	default:
		error(1, "nonconformant operation");
	}
	
	return vc;
}

/* Matrix op Matrix */
gsl_matrix*
libm_matrix_add_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op)
{
	gsl_matrix *mx;
	int ok;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	ok = matrix_compare_size(a, b);

	if (!ok)
		err_msg_ret(NULL, "nonconformant matrix size");

	matrix_init(&mx, a);

	switch(op) {
	case OPCODE_ADD:
		gsl_matrix_add(mx, b);
		break;
	case OPCODE_OR:
		gsl_matrix_sub(mx, b);
		break;
	default:
		error(1, "nonconformant operation");
	}
	
	return mx;
}

gsl_matrix*
libm_matrix_mult_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op)
{
	gsl_permutation *pm;
	gsl_matrix *inv, *mx = NULL;
	int ok, err, signum;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	mx = gsl_matrix_alloc(a->size1, b->size2);

	switch(op) {
	case OPCODE_MULT:	
		err = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0,
					 a, b, 0.0, mx);
		if (err) {
			gsl_matrix_free(mx);
			return NULL;
		}
		break;
	case OPCODE_DIV:
		ok = matrix_is_square(b);

		if (!ok) {
			fprintf(stderr, "error: matrix is not square\n");
			return NULL;	
		}
			
		pm = gsl_permutation_alloc(b->size1);
			
		inv = gsl_matrix_alloc(b->size1, b->size2);

		gsl_linalg_LU_decomp(b, pm, &signum);	
		gsl_linalg_LU_invert(b, pm, inv);
	
		mx = gsl_matrix_alloc(b->size1, a->size2);
		
		err = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0,
						inv, a, 0.0, mx);
			
		gsl_permutation_free(pm);
		gsl_matrix_free(inv);
			
		if (err) {
			gsl_matrix_free(mx);
			return NULL;
		}
	
		break;
	default:
		error(1, "nonconformant operation");
	}	
	
	return mx;	
}

double
libm_matrix_logic_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op)
{
	double dg;
	int ok;

	return_val_if_fail(a != NULL, -1.0);
	return_val_if_fail(b != NULL, -1.0);
		
	ok = matrix_compare_size(a, b);
	
	if (!ok)
		err_msg_ret(-1.0, "nonconformant size");

	switch(op) {
	case OPCODE_AND:
		dg = matrix_and(a, b);
		break;
	case OPCODE_OR:
		dg = matrix_or(a, b);
		break;
	default:
		error(1, "nonconformant operation");
	}	
	
	return dg;
}

gsl_matrix*
libm_matrix_rel_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op)
{
	gsl_matrix *mx;
	double dg, x, y;
	int ok, i, j;

	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);
	
	ok = matrix_compare_size(a, b);
	
	if (!ok)
		return NULL;	
	
	mx = gsl_matrix_alloc(a->size1, a->size2);

	switch(op) {
	case OPCODE_LT:
	case OPCODE_LE:
	case OPCODE_GT:
	case OPCODE_GE:
	case OPCODE_EQ:
	case OPCODE_NE:
		for (i = 0; i < mx->size1; i++) {
			for (j = 0; j < mx->size2; j++) {
				x  = gsl_matrix_get(a, i, j);
				y  = gsl_matrix_get(b, i, j);
				dg = libm_digit_op(x, y, op);
				gsl_matrix_set(mx, i, j, dg);
			}
		}
		break;
	default:
		error(1, "nonconformant operation");
	}

	return mx;
}

gsl_matrix*    
libm_matrix_exp(gsl_matrix *a, int pow)
{
	gsl_matrix *mx, *amx, *buf;
	int ok;

	return_val_if_fail(a != NULL, NULL);
		
	ok = matrix_is_square(a);

	if (!ok)
		err_msg_ret(NULL, "in this operation matrix"
				  " must be square");
	
	matrix_init(&mx, a);
	matrix_init(&amx, a);
	
	buf = gsl_matrix_alloc(a->size1, a->size2);
	
	(pow & 0x1) ? : gsl_matrix_set_identity(mx);
		
	pow >>= 1;
	
	while (pow) {
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0,
					amx, amx, 0.0, buf);
		matrix_init(&amx, buf);

		if (pow & 0x1) {
			gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0,
						amx, mx, 0.0, buf);
			matrix_init(&mx, buf);
		}

		pow >>= 1;
	}
	
	gsl_matrix_free(amx);
	gsl_matrix_free(buf);
	
	return mx;
} 
