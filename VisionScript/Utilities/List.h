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

list(void) ListInsert(list(void) list, void * value, int32_t index);

list(void) ListRemove(list(void) list, int32_t index);

list(void) ListPush(list(void) list, void * value);

list(void) ListPop(list(void) list);

list(void) ListRemoveAll(list(void) list, void * value);

list(void) ListRemoveFirst(list(void) list, void * value);

list(void) ListRemoveLast(list(void) list, void * value);

bool ListContains(list(void) list, void * value);

list(void) ListClear(list(void) list);

list(void) ListClone(list(void) list);

void ListFree(list(void) list);


#endif
