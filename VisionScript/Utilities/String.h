#ifndef String_h
#define String_h

#include <stdint.h>
#include <stdbool.h>

typedef char * String;

String StringCreate(const char * chars);

int32_t StringLength(String string);

void StringConcat(String * string, const char * chars);

void StringConcatFront(const char * chars, String * string);

String StringSub(String string, int32_t indexStart, int32_t indexEnd);

int32_t StringIndexOf(String string, const char chr);

void StringSet(String * string, const char * chars);

void StringInsert(String * string, int32_t index, const char * chars);

void StringRemove(String * string, int32_t indexStart, int32_t indexEnd);

bool StringEquals(String string, const char * chars);

void StringDestroy(String string);

#endif
