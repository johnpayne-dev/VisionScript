#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "HashMap.h"

#define FNV_OFFSET_32 0x811C9DC5
#define FNV_PRIME_32  0x01000193

uint32_t HashKey(const char * key)
{
	uint32_t hash = FNV_OFFSET_32;
	while (*key)
	{
		hash ^= (uint32_t)(*key);
		hash *= FNV_PRIME_32;
		key++;
	}
	return hash;
}

HashMap HashMapCreate(const uint32_t capacity)
{
	return (HashMap)
	{
		.capacity = capacity,
		.entries = calloc(capacity, sizeof(HashEntry)),
	};
}

void HashMapSet(HashMap map, const char * key, void * value)
{
	uint32_t i = 0;
	uint32_t hash = HashKey(key);
	uint32_t index = hash % map.capacity;
	while (true)
	{
		if (map.entries[index].value == NULL)
		{
			map.entries[index] = (HashEntry){ key, value };
			return;
		}
		i++;
		index = (hash + (i * (i + 1)) / 2) % map.capacity;
	}
}

void * HashMapGet(HashMap map, const char * key)
{
	uint32_t i = 0;
	uint32_t hash = HashKey(key);
	uint32_t index = hash % map.capacity;
	while (true)
	{
		if (map.entries[index].value == NULL) { return NULL; }
		if (strcmp(map.entries[index].key, key) == 0) { return map.entries[index].value; }
		i++;
		index = (hash + (i * (i + 1)) / 2) % map.capacity;
	}
}

void HashMapDestroy(HashMap map)
{
	free(map.entries);
}
