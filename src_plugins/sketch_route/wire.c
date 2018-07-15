#include <assert.h>

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
  memcpy(dst, src, sizeof(wire_point_t));
  dst->points = realloc(dst->points, src->point_max*sizeof(wire_point_t));
  memcpy(dst->points, src->points, src->point_num*sizeof(wire_point_t));
}

static int node_index(wire_t *w, wirelist_node_t *node)
{
  int i;
  for (i = 0; i < w->point_num; i++) {
    if (w->points[i].wire_node == node)
      break;
  }
  if (i == w->point_num)
    return -1;
  return i;
}

int wire_is_node_connected_with_point(wirelist_node_t *node, point_t *p)
{
  wire_t *w = node->item;
  int i = node_index(w, node);
  if (i == -1)
    return 0;
  if (i == 0)
    return w->points[1].p == p;
  if (i == w->point_num - 1)
    return w->points[w->point_num - 2].p == p;
  return (w->points[i - 1].p == p) || (w->points[i + 1].p == p);
}

int wire_is_coincident_at_node(wirelist_node_t *node, point_t *p1, point_t *p2)
{
  wire_t *w = node->item;
  int i = node_index(w, node);
  assert(i != -1 && i != 0 && i != w->point_num - 1);
  return (w->points[i - 1].p == p1 && w->points[i + 1].p == p2);
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
