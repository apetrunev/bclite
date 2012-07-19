#ifndef LIBM_H_
#define LIBM_H_

#include <gsl/gsl_blas.h>

#include "as_tree.h"

double
libm_digit_op(double a, double b, opcode_type_t op);

/* digit op Vector */
gsl_vector*
libm_digit_vector_add_op(double a, gsl_vector *b, opcode_type_t op);

gsl_vector*
libm_digit_vector_mult_op(double a, gsl_vector *b, opcode_type_t op);

double
libm_digit_vector_logic_op(double a, gsl_vector *b, opcode_type_t op);

gsl_vector*
libm_digit_vector_rel_op(double a, gsl_vector *b, opcode_type_t op);

/* Vector op digit */
gsl_vector*
libm_vector_digit_mult_op(gsl_vector *a, double b, opcode_type_t op);

/* digit op Matrix */
gsl_matrix*
libm_digit_matrix_add_op(double a, gsl_matrix *b, opcode_type_t op);

gsl_matrix*
libm_digit_matrix_mult_op(double a, gsl_matrix *b, opcode_type_t op);

double
libm_digit_matrix_logic_op(double a, gsl_matrix *b, opcode_type_t op);

gsl_matrix*
libm_digit_matrix_rel_op(double a, gsl_matrix *b, opcode_type_t op);

/* Matrix op digit */
gsl_matrix*
libm_matrix_digit_mult_op(gsl_matrix *a, double b, opcode_type_t op);

/* Vector op Vector */
gsl_vector*
libm_vector_add_op(gsl_vector *a, gsl_vector *b, opcode_type_t op);

double
libm_vector_mult_op(gsl_vector *a, gsl_vector *b, opcode_type_t op);

double
libm_vector_logic_op(gsl_vector *a, gsl_vector *b, opcode_type_t op);

gsl_vector*
libm_vector_rel_op(gsl_vector *a, gsl_vector *b, opcode_type_t op);

/* Vector op Matrix */
gsl_vector*
libm_vector_matrix_mult_op(gsl_vector *a, gsl_matrix *b, opcode_type_t op);

/* Matrix op Vector */
gsl_vector*
libm_matrix_vector_mult_op(gsl_matrix *a, gsl_vector *b, opcode_type_t op);

/* Matrix op Matrix */
gsl_matrix*
libm_matrix_add_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op);

gsl_matrix*
libm_matrix_mult_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op);

double
libm_matrix_logic_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op);

gsl_matrix*
libm_matrix_rel_op(gsl_matrix *a, gsl_matrix *b, opcode_type_t op);

gsl_matrix*
libm_matrix_exp(gsl_matrix *a, int dg);

#endif /* LIBM_H_ */
