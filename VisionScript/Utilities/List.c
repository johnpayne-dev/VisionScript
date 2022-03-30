#include <string.h>
#include <stdlib.h>
#include "List.h"

struct ListInfo
{
	uint32_t elementSize;
	uint32_t capacity;
	uint32_t length;
};

list(void) ListCreate(uint32_t elementSize, uint32_t initialCapacity)
{
	struct ListInfo * list = malloc(sizeof(struct ListInfo) + initialCapacity * elementSize);
	list->elementSize = elementSize;
	list->capacity = initialCapacity;
	list->length = 0;
	return list + 1;
}

uint32_t ListLength(list(void) list)
{
	return ((struct ListInfo *)list - 1)->length;
}

uint32_t ListElementSize(list(void) list)
{
	return ((struct ListInfo *)list - 1)->elementSize;
}

uint32_t ListCapacity(list(void) list)
{
	return ((struct ListInfo *)list - 1)->capacity;
}

list(void) ListInsert(list(void) list, void * element, int32_t index)
{
	struct ListInfo * info = (struct ListInfo *)list - 1;
	info->length++;
	if (info->length == info->capacity)
	{
		info->capacity *= 2;
		info = realloc(info, sizeof(struct ListInfo) + info->capacity * info->elementSize);
		list = info + 1;
	}
	memmove(list + (index + 1) * info->elementSize, list + index * info->elementSize, (info->length - 1 - index) * info->elementSize);
	memcpy(list + index * info->elementSize, element, info->elementSize);
	return list;
}

list(void) ListRemove(list(void) list, int32_t index)
{
	struct ListInfo * info = (struct ListInfo *)list - 1;
	for (uint32_t j = (index + 1) * info->elementSize; j < info->length * info->elementSize; j++)
	{
		((uint8_t *)list)[j - info->elementSize] = ((uint8_t *)list)[j];
	}
	info->length--;
	if (info->length == info->capacity / 2 - 1)
	{
		info->capacity /= 2;
		info = realloc(info, sizeof(struct ListInfo) + info->capacity * info->elementSize);
		list = info + 1;
	}
	return list;
}

list(void) ListPush(list(void) list, void * value)
{
	return ListInsert(list, value, ListLength(list));
}

list(void) ListPop(list(void) list)
{
	return ListRemove(list, ListLength(list) - 1);
}

list(void) ListRemoveAll(list(void) list, void * value)
{
	for (uint32_t i = 0; i < ListLength(list); i++)
	{
		if (memcmp(list + i * ListElementSize(list), value, ListElementSize(list)) == 0)
		{
			list = ListRemove(list, i);
			i--;
		}
	}
	return list;
}

bool ListContains(list(void) list, void * value)
{
	for (uint32_t i = 0; i < ListLength(list); i++)
	{
		if (memcmp(list + i * ListElementSize(list), value, ListElementSize(list))) { return true; }
	}
	return false;
}

list(void) ListClear(list(void) list)
{
	list(void) newList = ListCreate(ListElementSize(list), ListCapacity(list));
	ListFree(list);
	return newList;
}

list(void) ListClone(list(void) list)
{
	uint64_t size = sizeof(struct ListInfo) + ListElementSize(list) * ListCapacity(list);
	struct ListInfo * clone = malloc(size);
	memcpy(clone, (struct ListInfo *)list - 1, size);
	return clone + 1;
}

void ListFree(list(void) list)
{
	free((struct ListInfo *)list - 1);
}
