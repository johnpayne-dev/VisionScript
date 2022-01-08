#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "List.h"

static const uint32_t MetaSize = 2 * sizeof(uint32_t);

list(void) ListCreate(uint32_t elementSize)
{
	uint32_t * list = malloc(MetaSize + 2 * elementSize);
	list[0] = elementSize;
	list[1] = 0;
	return (uint8_t *)list + MetaSize;
}

struct ListData
{
	uint32_t ElementSize;
	uint32_t * Count;
	uint32_t Capacity;
};

static struct ListData ListData(list(void) list)
{
	struct ListData data = { 0 };
	data.ElementSize = *(uint32_t *)((uint8_t *)list - MetaSize);
	data.Count = (uint32_t *)((uint8_t *)list - MetaSize / 2);
	data.Capacity = pow(2.0, floor(log2(*data.Count)) + 1.0);
	return data;
}

uint32_t ListLength(list(void) list)
{
	return *(uint32_t *)((uint8_t *)list - MetaSize / 2);
}

uint32_t ListElementSize(list(void) list)
{
	return *(uint32_t *)((uint8_t *)list - MetaSize);
}

uint32_t ListCapacity(list(void) list)
{
	return ListData(list).Capacity;
}

void ListInsert(list(void) * list, void * element, int32_t index)
{
	struct ListData data = ListData(*list);
	if (index < 0 || index > *data.Count)
	{
		//printf("Trying to add a value to a list, but the index is out of bounds.\n");
		exit(1);
	}
	
	(*data.Count)++;
	if (*data.Count == data.Capacity)
	{
		*list = (uint8_t *)realloc((uint8_t *)*list - MetaSize, MetaSize + 2 * data.Capacity * data.ElementSize) + MetaSize;
	}
	data = ListData(*list);
	memmove((uint8_t *)*list + (index + 1) * data.ElementSize, (uint8_t *)*list + index * data.ElementSize, (*data.Count - 1 - index) * data.ElementSize);
	memcpy((uint8_t *)*list + index * data.ElementSize, element, data.ElementSize);
}

void ListRemove(list(void) * list, int32_t index)
{
	struct ListData data = ListData(*list);
	if (index < 0 || index > *data.Count)
	{
		//printf("Trying to remove a value from a list, but the index is out of bounds.\n");
		exit(1);
	}

	for (int j = (index + 1) * (int)data.ElementSize; j < (*data.Count) * (int)data.ElementSize; j++)
	{
		((uint8_t *)*list)[j - data.ElementSize] = ((uint8_t *)*list)[j];
	}
	(*data.Count)--;
	if (*data.Count == data.Capacity / 2 - 1)
	{
		*list = (uint8_t *)realloc((uint8_t *)*list - MetaSize, MetaSize + (data.Capacity / 2) * data.ElementSize) + MetaSize;
	}
}

void ListPush(list(void) * list, void * value)
{
	return ListInsert(list, value, ListLength(*list));
}

void ListPop(list(void) * list)
{
	if (ListLength(*list) == 0)
	{
		//printf("Trying to pop from an empty list.\n");
		exit(1);
	}
	return ListRemove(list, ListLength(*list) - 1);
}

void ListRemoveAll(list(void) * list, void * value)
{
	for (int i = 0; i < ListLength(*list); i++)
	{
		if (memcmp((uint8_t *)*list + i * ListElementSize(*list), value, ListElementSize(*list)) == 0)
		{
			ListRemove(list, i);
			i--;
		}
	}
}

bool ListContains(list(void) list, void * value)
{
	for (int i = 0; i < ListLength(list); i++)
	{
		if (memcmp((uint8_t *)list + i * ListElementSize(list), value, ListElementSize(list))) { return true; }
	}
	return false;
}

void ListClear(list(void) * list)
{
	list(void) newList = ListCreate(ListElementSize(*list));
	ListDestroy(*list);
	*list = newList;
}

void ListDestroy(list(void) list)
{
	free((uint8_t *)list - MetaSize);
}
