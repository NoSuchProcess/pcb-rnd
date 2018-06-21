#define LST_DONT_UNDEF
#define GVT_DONT_UNDEF
#include "edge.h"

static int LST(compare_func)(LST_ITEM_T *a, LST_ITEM_T *b)
{
  return *a == *b;
}

int GVT(constructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem)
{
  *elem = calloc(1, sizeof(**elem));
  return *elem == NULL;
}

void GVT(destructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem)
{
  free(*elem);
}

#include <list/list.c>
#include <genvector/genvector_impl.c>
