#include <stdio.h>

typedef struct {
	int x;
	int y;
} pos_t;

#define LST(x) poslist_ ## x
#define LST_ITEM_T pos_t
int LST(compare_func)(LST_ITEM_T *a, LST_ITEM_T *b)
{
  return a->x == b->x && a->y == b->y;
}
#include "list.h"
#include "list.c"
#define POSLIST_FOREACH(loop_item, list) do { \
  poslist_node_t *node = list; \
  pos_t *loop_item; \
  while (node != NULL) { \
    loop_item = &node->item;

#define POSLIST_FOREACH_END() \
    node = node->next; \
  } \
} while(0)
#undef LST
#undef LST_ITEM_T

#define LST(x) intlist_ ## x
#define LST_ITEM_T int
int LST(compare_func)(LST_ITEM_T *a, LST_ITEM_T *b)
{
  return *a == *b;
}
#include "list.h"
#include "list.c"
#define INTLIST_FOREACH(loop_item, list) do { \
  intlist_node_t *node = list; \
  int *loop_item; \
  while (node != NULL) { \
    loop_item = &node->item;

#define INTLIST_FOREACH_END() \
    node = node->next; \
  } \
} while(0)


int main(void)
{
  poslist_node_t *list1 = NULL;
  intlist_node_t *list2 = NULL;
  intlist_node_t *list2_node = NULL;

  pos_t p = {1, 2};
  int i = 4;

  list1 = poslist_prepend(list1, &p); p.x = 4; p.y = 5;
  list1 = poslist_prepend(list1, &p); p.x = 8; p.y = 1;
  list1 = poslist_prepend(list1, &p);

  list2 = intlist_prepend(list2, &i); i = 1;
  list2 = intlist_prepend(list2, &i); i = 5;
  list2 = intlist_prepend(list2, &i);

  i = 5;
  list2 = intlist_remove_item(list2, &i);

  //~ list2_node = list2->next;
  //~ list2 = intlist_remove(list2, list2_node);

  //~ list2 = intlist_remove_front(list2);
  //~ list2 = intlist_remove_front(list2);
  //~ list2 = intlist_remove_front(list2);
  //~ list2 = intlist_remove_front(list2);
  //~ list2 = intlist_remove_front(list2); i = 423;
  //~ list2 = intlist_prepend(list2, &i);

  printf("list1:\n");
  POSLIST_FOREACH(pos, list1)
    printf("%d, %d\n", pos->x, pos->y);
  POSLIST_FOREACH_END();

  printf("list2:\n");
  INTLIST_FOREACH(num, list2)
    printf("%d\n", *num);
  INTLIST_FOREACH_END();
}
