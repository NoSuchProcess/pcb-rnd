#include <stdlib.h>

typedef struct
{
  size_t MaxLength;
  char *Data;
} DynamicStringType, *DynamicStringTypePtr;

void DSAddCharacter (DynamicStringTypePtr, char);
void DSAddString (DynamicStringTypePtr, const char *);
void DSClearString (DynamicStringTypePtr);
