#ifndef List_h
#define List_h

#include <stdint.h>
#include <stdbool.h>

#define list(type) type*
typedef void * List;

list(void) ListCreate(uint32_t elementSize, uint32_t initialCapacity);

uint32_t ListLength(list(void) list);

uint32_t ListElementSize(list(void) list);

uint32_t ListCapacity(list(void) list);

void ListInsert(list(void) * list, void * value, int32_t index);

void ListRemove(list(void) * list, int32_t index);

void ListPush(list(void) * list, void * value);

void ListPop(list(void) * list);

void ListRemoveAll(list(void) * list, void * value);

void ListRemoveFirst(list(void) * list, void * value);

void ListRemoveLast(list(void) * list, void * value);

bool ListContains(list(void) list, void * value);

void ListClear(list(void) * list);

list(void) ListClone(list(void) list);

void ListDestroy(list(void) list);


#endif
