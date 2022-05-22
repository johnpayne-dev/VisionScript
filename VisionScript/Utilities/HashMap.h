#ifndef HashMap_h
#define HashMap_h

#include <stdint.h>
#include "List.h"

typedef struct HashSlot {
	List(const char *) keys;
	List(void) values;
} HashSlot;

#define HashMap(type) HashSlot*

uint32_t Hash(const char * key);
HashMap(void) HashMapCreate(uint32_t elementSize);
void HashMapSet(HashMap(void) map, const char * key, void * value);
void * HashMapGet(HashMap(void) map, const char * key);
void HashMapFree(HashMap(void) map);

#endif
