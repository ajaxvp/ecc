#ifndef _STDLIB_H
#define _STDLIB_H

#define NULL ((void*) 0)

#define EXIT_SUCCESS (0)
#define EXIT_FAILURE (1)

typedef unsigned long int size_t;

void* malloc(size_t size);
void free(void* ptr);

int abs(int j);

#endif
