#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "HashMap.h"

#define FNV_OFFSET_32 0x811C9DC5
#define FNV_PRIME_32  0x01000193

#define HASH_MAP_CAPCITY 256

uint32_t Hash(const char * key) {
	uint32_t hash = FNV_OFFSET_32;
	while (*key) {
		hash ^= (uint32_t)(*key);
		hash *= FNV_PRIME_32;
		key++;
	}
	return hash;
}

HashMap(void) HashMapCreate(uint32_t elementSize) {
	HashSlot * map = malloc(HASH_MAP_CAPCITY * sizeof(HashSlot));
	for (int32_t i = 0; i < HASH_MAP_CAPCITY; i++) {
		map[i] = (HashSlot) {
			.keys = ListCreate(sizeof(const char *), 1),
			.values = ListCreate(elementSize, 1),
		};
	}
	return map;
}

void HashMapSet(HashMap(void) map, const char * key, void * value) {
	HashSlot * slot = &map[Hash(key) % HASH_MAP_CAPCITY];
	for (int32_t i = 0; i < ListLength(slot->keys); i++) {
		if (strcmp(slot->keys[i], key) == 0) {
			if (value == NULL) {
				slot->keys = ListRemove(slot->keys, i);
				slot->values = ListRemove(slot->values, i);
			} else {
				ListSet(slot->values, i, value);
			}
			return;
		}
	}
	if (value != NULL) {
		slot->keys = ListPush(slot->keys, &key);
		slot->values = ListPush(slot->values, value);
	}
}

void * HashMapGet(HashMap(void) map, const char * key) {
	HashSlot * slot = &map[Hash(key) % HASH_MAP_CAPCITY];
	for (int32_t i = 0; i < ListLength(slot->keys); i++) {
		if (strcmp(slot->keys[i], key)) { return ListGet(slot->values, i); }
	}
	return NULL;
}

void HashMapFree(HashMap(void) map) {
	for (int32_t i = 0; i < HASH_MAP_CAPCITY; i++) {
		ListFree(map[i].keys);
		ListFree(map[i].values);
	}
	free(map);
}
