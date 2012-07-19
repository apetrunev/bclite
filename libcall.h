#ifndef LIBCALL_H_
#define LIBCALL_H_

#include "function.h"
 
int
libcall_sin(struct function *func, value_t *v_type, void **result);

int
libcall_cos(struct function *func, value_t *v_type, void **result);

int 
libcall_ln(struct function *func, value_t *v_type, void **result);

int
libcall_tan(struct function *func, value_t *v_type, void **result);

int 
libcall_exp(struct function *func, value_t *v_type, void **result);

int
libcall_matrix(struct function *func, value_t *v_type, void **result);

int
libcall_vector(struct function *func, value_t *v_type, void **result);

int
libcall_sqrt(struct function *func, value_t *v_type, void **result);

#endif /* LIBCALL_H_ */ 
