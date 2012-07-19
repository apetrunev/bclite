#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include  "macros.h"

void*
umalloc(size_t size)
{
	void *ptr;

	if (size == 0 || size > SSIZE_MAX)
		error(1, "umalloc: requested %lu bytes", (unsigned long) size);
	
	ptr = malloc(size);
	
	if (ptr == NULL)
		error(1, "umalloc: out of memory allocating %lu bytes", (unsigned long)size);
	
	return ptr;	
}

void*
umalloc0(size_t size)
{
	void *ptr;
	
	if (size == 0 || size > SSIZE_MAX)
		error(1, "umalloc0: requested %lu bytes", (unsigned long)size);
	
	ptr = malloc(size);
	
	if (ptr == NULL)
		error(1, "umallo0: requested %lu bytes", (unsigned long)size);
		
	memset(ptr, 0, size);
	
	return ptr;
}

void*
urealloc(void *mem, size_t size)
{
	void *ptr;

	if (size == 0 || size > SSIZE_MAX)
		error(1, "urealloc: requested %lu bytes", (unsigned long)size);
	
	if (mem == NULL)
		ptr = malloc(size);
	else 
		ptr = realloc(mem, size);
		
	if (ptr == NULL)
		error(1, "urealloc: out of memory allocating %lu bytes",
							(unsigned long)size);
	return ptr;
}

void*
urealloc0(void *mem, size_t old_size, size_t new_size)
{
	void *ptr;	

	if (mem == NULL && old_size)
		error(1,"urealloc0: old_size != 0 on NULL memory");
		
	if (new_size == 0 || new_size > SSIZE_MAX)
		error(1, "urealloc0: requested %lu byte",(unsigned long)new_size);
	
	if (mem == NULL)
		ptr = malloc(new_size);
	else
		ptr = realloc(mem, new_size);
	
	if (new_size > old_size)
		memset(ptr + old_size, 0, new_size - old_size);
	
	return mem;
}

void
ufree(void *mem)
{
	if (mem == NULL)
		error(1,"ufree: NULL pointer");
	
	free(mem);
}

char*
ustrdup(const char *str)
{
	char *res;
	size_t len;

	if (str == NULL)
		error(1, "ustrdup: str NULL");

	len = strlen(str);
	res = umalloc0(len + 1);
	
	memcpy(res, str, len);
	
	res[len] = '\0';

	return res;
}

