#include <stdint.h>

#ifndef LIST_H
#define LIST_H

typedef struct {
    uint8_t * data;
    size_t elements, element_size, mem_size;
} List;

typedef enum {
    LIST_TRAVERSE_FORWARD,
    LIST_TRAVERSE_BACKWARD,
} ListTraverseDirection;

int list_created(List *);
void list_create(List *, size_t element_size);
void list_free(List *);
void list_sort(List *, int (*cmp)(const void * obj, const void * ref));
void * list_find(List *, void * ref, ListTraverseDirection, int (*cmp)(void *, void *));
void * list_get(List *, size_t i);
void * list_add(List *, void *);
void list_remove(List *, size_t i);
void list_clear(List *);
int list_size(List *);

#endif
