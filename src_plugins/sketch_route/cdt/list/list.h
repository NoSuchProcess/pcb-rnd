
struct LST(node_s) {
	LST_ITEM_T item;
	struct LST(node_s) *next;
};
typedef struct LST(node_s) LST(node_t);

LST(node_t) *LST(prepend)(LST(node_t) *list, LST_ITEM_T *item);
LST(node_t) *LST(remove_front)(LST(node_t) *list);
LST(node_t) *LST(remove)(LST(node_t) *list, LST(node_t) *node);
LST(node_t) *LST(remove_item)(LST(node_t) *list, LST_ITEM_T *item);
size_t LST(length)(LST(node_t) *list);
