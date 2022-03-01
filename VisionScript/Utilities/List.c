#include <string.h>
#include <stdlib.h>
#include "List.h"

struct ListData
{
	uint32_t elementSize;
	uint32_t capacity;
	uint32_t length;
};

list(void) ListCreate(uint32_t elementSize, uint32_t initialCapacity)
{
	struct ListData * list = malloc(sizeof(struct ListData) + initialCapacity * elementSize);
	list->elementSize = elementSize;
	list->capacity = initialCapacity;
	list->length = 0;
	return list + 1;
}

uint32_t ListLength(list(void) list)
{
	return ((struct ListData *)list - 1)->length;
}

uint32_t ListElementSize(list(void) list)
{
	return ((struct ListData *)list - 1)->elementSize;
}

uint32_t ListCapacity(list(void) list)
{
	return ((struct ListData *)list - 1)->capacity;
}

void ListInsert(list(void) * list, void * element, int32_t index)
{
	struct ListData * data = (struct ListData *)*list - 1;
	data->length++;
	if (data->length == data->capacity)
	{
		data->capacity *= 2;
		data = realloc(data, sizeof(struct ListData) + data->capacity * data->elementSize);
		*list = data + 1;
	}
	memmove(*list + (index + 1) * data->elementSize, *list + index * data->elementSize, (data->length - 1 - index) * data->elementSize);
	memcpy(*list + index * data->elementSize, element, data->elementSize);
}

void ListRemove(list(void) * list, int32_t index)
{
	struct ListData * data = (struct ListData *)*list - 1;
	for (uint32_t j = (index + 1) * data->elementSize; j < data->length * data->elementSize; j++)
	{
		((uint8_t *)*list)[j - data->elementSize] = ((uint8_t *)*list)[j];
	}
	data->length--;
	if (data->length == data->capacity / 2 - 1)
	{
		data->capacity /= 2;
		data = realloc(data, sizeof(struct ListData) + data->capacity * data->elementSize);
		*list = data + 1;
	}
}

void ListPush(list(void) * list, void * value)
{
	return ListInsert(list, value, ListLength(*list));
}

void ListPop(list(void) * list)
{
	return ListRemove(list, ListLength(*list) - 1);
}

void ListRemoveAll(list(void) * list, void * value)
{
	for (uint32_t i = 0; i < ListLength(*list); i++)
	{
		if (memcmp(*list + i * ListElementSize(*list), value, ListElementSize(*list)) == 0)
		{
			ListRemove(list, i);
			i--;
		}
	}
}

bool ListContains(list(void) list, void * value)
{
	for (uint32_t i = 0; i < ListLength(list); i++)
	{
		if (memcmp(list + i * ListElementSize(list), value, ListElementSize(list))) { return true; }
	}
	return false;
}

void ListClear(list(void) * list)
{
	list(void) newList = ListCreate(ListElementSize(*list), ListCapacity(*list));
	ListDestroy(*list);
	*list = newList;
}

void ListDestroy(list(void) list)
{
	free((struct ListData *)list - 1);
}
