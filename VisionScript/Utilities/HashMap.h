#ifndef HashMap_h
#define HashMap_h

#include <stdint.h>

uint32_t HashMapKey(const char * key);

typedef struct HashEntry
{
	const char * key;
	void * value;
} HashEntry;

typedef struct HashMap
{
	const uint32_t capacity;
	HashEntry * entries;
} HashMap;

HashMap HashMapCreate(const uint32_t capacity);
void HashMapSet(HashMap map, const char * key, void * value);
void * HashMapGet(HashMap map, const char * key);
void HashMapDestroy(HashMap map);

#endif
