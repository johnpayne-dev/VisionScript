#ifndef String_h
#define String_h

#include <stdint.h>

typedef char * String;

String StringCreate(char * chars);

int32_t StringLength(String string);

void StringConcat(String * string, char * chars);

void StringConcatFront(char * chars, String * string);

String StringSub(String string, int32_t indexStart, int32_t indexEnd);

int32_t StringIndexOf(String string, char chr);

void StringSet(String * string, char * chars);

void StringInsert(String * string, int32_t index, char * chars);

void StringRemove(String * string, int32_t indexStart, int32_t indexEnd);

void StringDestroy(String string);

#endif
