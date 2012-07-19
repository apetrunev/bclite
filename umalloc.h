#ifndef UMALLOC_H_
#define UMALLOC_H_

void*
umalloc(size_t size);

void*
umalloc0(size_t size);

void*
urealloc(void *mem, size_t size);

void*
urealloc0(void *mem, size_t old_size, size_t new_size);

char*
ustrdup(const char *str);

void
ufree(void *mem);

#endif /*UMALLOC_H_*/

