#ifndef List_h
#define List_h

#include <stdint.h>
#include <stdbool.h>

#define List(type) type*

List(void) ListCreate(uint32_t elementSize, uint32_t initialCapacity);

uint32_t ListLength(List(void) List);

uint32_t ListElementSize(List(void) List);

uint32_t ListCapacity(List(void) List);

List(void) ListInsert(List(void) List, void * value, int32_t index);

List(void) ListRemove(List(void) List, int32_t index);

List(void) ListPush(List(void) List, void * value);

List(void) ListPop(List(void) List);

List(void) ListRemoveAll(List(void) List, void * value);

List(void) ListRemoveFirst(List(void) List, void * value);

List(void) ListRemoveLast(List(void) List, void * value);

bool ListContains(List(void) List, void * value);

List(void) ListClear(List(void) List);

List(void) ListClone(List(void) List);

void ListFree(List(void) List);


#endif
