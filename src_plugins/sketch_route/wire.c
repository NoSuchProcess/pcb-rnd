#include "pcb-printf.h"

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
  dst->point_max = src->point_max;
  dst->point_num = src->point_num;
  dst->points = realloc(dst->points, src->point_max*sizeof(wire_point_t));
  memcpy(dst->points, src->points, src->point_num*sizeof(wire_point_t));
}

int wire_is_node_connected_with_point(wire_t *w, wirelist_node_t *node, point_t *p)
{
  int i;
  for (i = 0; i < w->point_num; i++) {
    if (w->points[i].wire_node == node)
      break;
  }
  if (i == w->point_num)
    return 0;
  if (i == 0)
    return w->points[1].p == p;
  if (i == w->point_num - 1)
    return w->points[w->point_num - 2].p == p;
  return (w->points[i - 1].p == p) || (w->points[i + 1].p == p);
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
