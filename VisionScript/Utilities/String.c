#include <stdlib.h>
#include <string.h>
#include "String.h"

String StringCreate(const char * chars)
{
	String string = malloc(strlen(chars) + 1);
	memcpy(string, chars, strlen(chars) + 1);
	return string;
}

int32_t StringLength(String string)
{
	return (int32_t)strlen(string);
}

void StringConcat(String * string, const char * chars)
{
	StringInsert(string, StringLength(*string), chars);
}

void StringConcatFront(const char * chars, String * string)
{
	StringInsert(string, 0, chars);
}

String StringSub(String string, int32_t indexStart, int32_t indexEnd)
{
	if (indexStart < 0) { indexStart = 0; }
	if (indexEnd > StringLength(string) - 1) { indexEnd = StringLength(string) - 1; }
	if (indexStart > indexEnd) { return StringCreate(""); }
	char orig = string[indexEnd + 1];
	string[indexEnd + 1] = '\0';
	String newString = StringCreate(string + indexStart);
	string[indexEnd + 1] = orig;
	return newString;
}

int32_t StringIndexOf(String string, char chr)
{
	for (int32_t i = 0; i < StringLength(string); i++)
	{
		if (string[i] == chr) { return i; }
	}
	return -1;
}

void StringSet(String * string, const char * chars)
{
	StringDestroy(*string);
	*string = StringCreate(chars);
}

void StringInsert(String * string, int32_t index, const char * chars)
{
	int32_t len1 = StringLength(*string), len2 = (int32_t)strlen(chars);
	if (index > len1) { index = len1; }
	*string = realloc(*string, len1 + len2 + 1);
	for (int32_t i = len1; i >= index; i--) { (*string)[i + len2] = (*string)[i]; }
	memcpy(*string + index, chars, len2);
}

void StringRemove(String * string, int32_t indexStart, int32_t indexEnd)
{
	if (indexStart < 0) { indexStart = 0; }
	if (indexEnd > StringLength(*string) - 1) { indexEnd = StringLength(*string) - 1; }
	if (indexStart > indexEnd) { StringSet(string, ""); }
	for (int i = 0; i < StringLength(*string) - indexEnd; i++)
	{
		(*string)[indexStart + i] = (*string)[indexEnd + i + 1];
	}
	*string = realloc(*string, StringLength(*string) - (indexEnd - indexStart + 1));
}

bool StringEquals(String string, const char * chars)
{
	return strcmp(string, chars) == 0;
}

void StringDestroy(String string)
{
	free(string);
}
