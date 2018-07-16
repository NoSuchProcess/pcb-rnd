#define GVT_DONT_UNDEF
#include "ewire.h"

void ewire_init(ewire_t *ew)
{
	vtewire_point_init(&ew->points);
}

void ewire_uninit(ewire_t *ew)
{
	vtewire_point_uninit(&ew->points);
}

void ewire_append_point(ewire_t *ew, spoke_t *sp, side_t side, int sp_slot, wirelist_node_t *w_node)
{
	ewire_insert_point(ew, ew->points.used-1, sp, side, sp_slot, w_node);
}

void ewire_insert_point(ewire_t *ew, int after_i, spoke_t *sp, side_t side, int sp_slot, wirelist_node_t *w_node)
{
	ewire_point_t *ewp = vtewire_point_alloc_append(&ew->points, 1);
	ewp->sp = sp;
	ewp->side = side;
	ewp->sp_slot = sp_slot;
	ewp->w_node = w_node;
}

void ewire_remove_point(ewire_t *ew, int i)
{
	vtewire_point_remove(&ew->points, i, 1);
}

ewire_point_t *ewire_get_point_at_slot(ewire_t *ew, spoke_t *sp, int slot_num)
{
  int i;
  for (i = 0; i < vtewire_point_len(&ew->points); i++) {
    if (ew->points.array[i].sp == sp && ew->points.array[i].sp_slot == slot_num)
      return &ew->points.array[i];
  }
  return NULL;
}

int GVT(constructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem)
{
  *elem = malloc(sizeof(**elem));
  if (*elem == NULL)
    return 1;
  ewire_init(*elem);
  return 0;
}

void GVT(destructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem)
{
  ewire_uninit(*elem);
  free(*elem);
}

#include <genvector/genvector_impl.c>
