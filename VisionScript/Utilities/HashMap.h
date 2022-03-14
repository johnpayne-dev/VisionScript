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
	uint32_t capacity;
	HashEntry * entries;
} HashMap;

HashMap HashMapCreate(uint32_t capacity);
void HashMapSet(HashMap map, const char * key, void * value);
void * HashMapGet(HashMap map, const char * key);
void HashMapFree(HashMap map);

#endif
