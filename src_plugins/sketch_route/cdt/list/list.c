#include <stdlib.h>

static LST(node_t) *LST(create)(LST_ITEM_T *item, LST(node_t) *next)
{
	LST(node_t) *new_node = (LST(node_t)*) malloc(sizeof(LST(node_t)));
	new_node->item = *item;
	new_node->next = next;
	return new_node;
}

LST(node_t) *LST(prepend)(LST(node_t) *list, LST_ITEM_T *item)
{
	LST(node_t) *new_head = LST(create)(item, list);
	return new_head;
}

LST(node_t) *LST(insert_after)(LST(node_t) *node, LST_ITEM_T *item)
{
	LST(node_t) *new_node;
	if (node == NULL)
		return NULL;
	new_node = LST(create)(item, node->next);
	node->next = new_node;
	return new_node;
}

LST(node_t) *LST(remove_front)(LST(node_t) *list)
{
	LST(node_t) *new_head;
	if (list == NULL)
		return NULL;
	new_head = list->next;
	free(list);
	return new_head;
}

LST(node_t) *LST(remove)(LST(node_t) *list, LST(node_t) *node)
{
	LST(node_t) *prev_node;
	if (list == NULL)
		return NULL;
	if (list == node)
		return LST(remove_front)(list);
  for (prev_node = list; prev_node->next != NULL; prev_node = prev_node->next) {
		if (prev_node->next == node) {
			prev_node->next = node->next;
			free(node);
			break;
		}
	}
	return list;
}

LST(node_t) *LST(remove_item)(LST(node_t) *list, LST_ITEM_T *item)
{
	LST(node_t) *prev_node, *node;
	if (list == NULL)
		return NULL;
	if (LST(compare_func)(&list->item, item))
		return LST(remove_front)(list);
  for (prev_node = list, node = prev_node->next; node != NULL; prev_node = prev_node->next, node = node->next) {
		if (LST(compare_func)(&node->item, item)) {
			prev_node->next = node->next;
			free(node);
			break;
		}
	}
	return list;
}

size_t LST(length)(LST(node_t) *list)
{
	size_t len = 0;
	if (list == NULL)
		return 0;
	for (; list != NULL; list = list->next)
		len++;
	return len;
}

void LST(free)(LST(node_t) *list)
{
	LST(node_t) *node, *next_node;
	if (list == NULL)
		return;
  for (node = list, next_node = node->next; next_node != NULL; node = next_node, next_node = next_node->next)
		free(node);
	free(node);
}
