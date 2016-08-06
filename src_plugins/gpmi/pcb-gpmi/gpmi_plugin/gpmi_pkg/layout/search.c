#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "src/conf_core.h"
#include "layout.h"
#include "config.h"

static inline void search_append(layout_search_t *s, void *obj)
{
	layout_object_t *o;
	if (s->used >= s->alloced) {
		s->alloced += 256;
		s->objects = realloc(s->objects, s->alloced * sizeof(layout_object_t));
	}
	o = s->objects + s->used;
	s->used++;
	o->type = s->searching;
	o->layer = s->layer;
	switch(s->searching) {
		case OM_LINE:     o->obj.l   = obj; break;
		case OM_TEXT:     o->obj.t   = obj; break;
		case OM_POLYGON:  o->obj.p   = obj; break;
		case OM_ARC:      o->obj.a   = obj; break;
		case OM_VIA:      o->obj.v   = obj; break;
		case OM_PIN:      o->obj.pin = obj; break;
		default:
			assert(!"Unimplemented object type");
	}
}

static r_dir_t search_callback (const BoxType * b, void *cl)
{
	search_append(cl, (void *)b);
  return R_DIR_FOUND_CONTINUE;
}

hash_t *layout_searches = NULL;
static layout_search_t *new_search(const char *search_ID)
{
	layout_search_t *s;
	layout_search_free(search_ID);

	if (layout_searches == NULL) {
		layout_searches = hash_create(64, gpmi_hash_ptr);
		assert(layout_searches != NULL);
	}

	s = calloc(sizeof(layout_search_t), 1);

	hash_store(layout_searches, search_ID, s);
	return s;
}

int layout_search_box(const char *search_ID, layout_object_mask_t obj_types, int x1, int y1, int x2, int y2)
{
  BoxType spot;
	layout_search_t *s = new_search(search_ID);

	spot.X1 = x1;
	spot.Y1 = y1;
	spot.X2 = x2;
	spot.Y2 = y2;

	s->layer = -1;
	if (obj_types & OM_LINE) {
		s->searching = OM_LINE;
		r_search (CURRENT->line_tree, &spot, NULL, search_callback, s);
	}

	if (obj_types & OM_TEXT) {
		s->searching = OM_TEXT;
		r_search (CURRENT->text_tree, &spot, NULL, search_callback, s);
	}

	if (obj_types & OM_ARC) {
		s->searching = OM_ARC;
		r_search (CURRENT->arc_tree, &spot, NULL, search_callback, s);
	}

	if (obj_types & OM_VIA) {
		s->searching = OM_VIA;
		r_search (PCB->Data->via_tree, &spot, NULL, search_callback, s);
	}

	if (obj_types & OM_PIN) {
		s->searching = OM_PIN;
		r_search (PCB->Data->pin_tree, &spot, NULL, search_callback, s);
	}

	if (obj_types & OM_POLYGON) {
		s->searching = OM_POLYGON;
		for (s->layer = 0; s->layer < MAX_LAYER + 2; s->layer++)
			r_search (PCB->Data->Layer[s->layer].polygon_tree, &spot, NULL, search_callback, s);
		s->layer = -1;
	}

	return s->used;
}

/* SELECTEDFLAG */

typedef struct {
	int flag;
	layout_search_t *search;
} select_t;

static void select_cb(void *obj_, void *ud)
{
	select_t *ctx = ud;
	AnyObjectTypePtr obj = obj_;
	if (TEST_FLAG(ctx->flag, obj))
		search_append(ctx->search, obj);
}

#define select2(s, om, flag, lst) \
	do { \
		gdl_iterator_t it; \
		void *item; \
		select_t ctx; \
		ctx.flag = flag; \
		ctx.search = s; \
		s->searching = om; \
		linelist_foreach(lst, &it, item) select_cb(item, &ctx); \
		s->searching = 0; \
	} while(0)

static int layout_search_flag(const char *search_ID, multiple layout_object_mask_t obj_types, int flag)
{
	Cardinal l, n;
	layout_search_t *s = new_search(search_ID);
	LayerType *layer = PCB->Data->Layer;

	for (l =0; l < MAX_LAYER + 2; l++, layer++) {
		s->layer = l;
		select2(s, OM_ARC,     flag, &layer->Arc);
		select2(s, OM_LINE,    flag, &layer->Line);
		select2(s, OM_TEXT,    flag, &layer->Text);
		select2(s, OM_POLYGON, flag, &layer->Polygon);
	}
	select2(s, OM_VIA,  flag, &PCB->Data->Via);
/*	select2(s, OM_PIN,  flag, &PCB->Data->Pin); /* TODO */

	return s->used;
}
#undef select

int layout_search_selected(const char *search_ID, multiple layout_object_mask_t obj_types)
{
	return layout_search_flag(search_ID, obj_types, SELECTEDFLAG);
}

int layout_search_found(const char *search_ID, multiple layout_object_mask_t obj_types)
{
	return layout_search_flag(search_ID, obj_types, FOUNDFLAG);
}

layout_object_t *layout_search_get(const char *search_ID, int n)
{
	const layout_search_t *s;

	s = hash_find(layout_searches, search_ID);

/*	printf("s=%p\n", s);*/
	if ((s == NULL) || (n < 0) || (n >= s->used))
		return NULL;
	return s->objects+n;
}

int layout_search_free(const char *search_ID)
{
	layout_search_t *s;

	if (layout_searches == NULL)
		return 1;

	s = (layout_search_t *)hash_find(layout_searches, search_ID);
	if (s != NULL) {
		hash_del_key(layout_searches, search_ID);
		free(s->objects);
		free(s);
		return 0;
	}
	return 2;
}

