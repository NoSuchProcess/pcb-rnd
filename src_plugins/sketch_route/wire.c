#include <assert.h>
#include "pointdata.h"

#define GVT_DONT_UNDEF
#define LST_DONT_UNDEF
#include "wire.h"

#define WIRE_POINTS_STEP 10

void wire_init(wire_t *w)
{
	w->point_num = 0;
	w->point_max = WIRE_POINTS_STEP;
	w->points = malloc(WIRE_POINTS_STEP*sizeof(wire_point_t));
}

void wire_uninit(wire_t *w)
{
	free(w->points);
}

void wire_push_point(wire_t *w, point_t *p, int side)
{
	w->points[w->point_num].p = p;
	w->points[w->point_num].side = side;
	if (++w->point_num >= w->point_max) {
		w->point_max += WIRE_POINTS_STEP;
		w->points = realloc(w->points, w->point_max*sizeof(wire_point_t));
	}
}

void wire_pop_point(wire_t *w)
{
	if(w->point_num > 0)
		w->point_num--;
}

void wire_copy(wire_t *dst, wire_t *src)
{
	wire_point_t *temp = dst->points;
	memcpy(dst, src, sizeof(wire_point_t));
	dst->points = temp;
	dst->points = realloc(dst->points, src->point_max*sizeof(wire_point_t));
	memcpy(dst->points, src->points, src->point_num*sizeof(wire_point_t));
}

int wire_node_index(wire_t *w, wirelist_node_t *node)
{
  int i;
  for (i = 0; i < w->point_num; i++) {
    if (w->points[i].wire_node == node)
      return i;
  }
  return -1;
}

int wire_node_index_at_connected_point(wirelist_node_t *node, point_t *p)
{
	wire_t *w = node->item;
  int i = wire_node_index(w, node);
  if (i == -1)
    return -1;
  if (i == 0)
    return w->points[1].p == p ? 1 : -1;
  if (i == w->point_num - 1)
    return w->points[w->point_num - 2].p == p ? (w->point_num - 2) : -1;
  return (w->points[i - 1].p == p) ? i - 1
				 : (w->points[i + 1].p == p) ? i + 1 : -1;
}

int wire_is_node_connected_with_point(wirelist_node_t *node, point_t *p)
{
	return wire_node_index_at_connected_point(node, p) != -1;
}

int wire_is_coincident_at_node(wirelist_node_t *node, point_t *p1, point_t *p2)
{
  wire_t *w = node->item;
  int i = wire_node_index(w, node);
  assert(i != -1 && i != 0 && i != w->point_num - 1);
  return (w->points[i - 1].p == p1 && w->points[i + 1].p == p2)
				 || (w->points[i + 1].p == p1 && w->points[i - 1].p == p2);
}

int wire_point_position(wire_point_t *wp)
{
  pointdata_t *pd = wp->p->data;
  if (wirelist_get_index(pd->uturn_wires, wp->wire_node) != -1)
    return wirelist_length(wp->wire_node) - 1;
  else
    return wirelist_length(wp->wire_node) + wirelist_length(pd->uturn_wires) - 1;
}

static int LST(compare_func)(LST_ITEM_T *a, LST_ITEM_T *b)
{
  return *a == *b;
}

int GVT(constructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem)
{
  *elem = malloc(sizeof(**elem));
  if (*elem == NULL)
    return 1;
  wire_init(*elem);
  return 0;
}

void GVT(destructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem)
{
  wire_uninit(*elem);
  free(*elem);
}

#include "cdt/list/list.c"
#include <genvector/genvector_impl.c>
