#ifndef COMMON_H_
#define COMMON_H_

typedef int boolean_t;

typedef enum {
	VALUE_TYPE_UNKNOWN,
	VALUE_TYPE_DIGIT,
	VALUE_TYPE_STRING,
	VALUE_TYPE_VECTOR,
	VALUE_TYPE_MATRIX,
	VALUE_TYPE_VOID
} value_t;

typedef enum {  
	ret_not_found     = -6,	
	ret_entry_exists  = -5,
        ret_no_access     = -4,
        ret_out_of_memory = -3,
        ret_invalid       = -2,
        ret_err           = -1,
        ret_ok            = 0,
        ret_file_exists   = 1,
        ret_not_file      = 2,
} ret_t;

#endif /*COMMON_H_*/
