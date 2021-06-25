/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

static pcb_2netmap_oseg_t *oseg_new(pcb_2netmap_t *map, pcb_net_t *net, int shorted)
{
	pcb_2netmap_oseg_t *os;
	os = calloc(sizeof(pcb_2netmap_oseg_t), 1);
	os->next = map->osegs;
	map->osegs = os;
	os->net = net;
	os->shorted = shorted;
	return os;
}

static pcb_2netmap_obj_t *map_seg_out_obj(pcb_2netmap_t *map, pcb_any_obj_t *obj)
{
	pcb_2netmap_obj_t *res = malloc(sizeof(pcb_2netmap_obj_t));

	/* copy the object but reset some fields as this object is not part of any layer list */
	switch(obj->type) {
		case PCB_OBJ_LINE: memcpy(&res->o, obj, sizeof(res->o.line)); memset(&res->o.line.link, 0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_RAT:  memcpy(&res->o, obj, sizeof(res->o.rat));  memset(&res->o.rat.link, 0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_ARC:  memcpy(&res->o, obj, sizeof(res->o.arc));  memset(&res->o.arc.link,  0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_PSTK: memcpy(&res->o, obj, sizeof(res->o.pstk)); memset(&res->o.pstk.link,  0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_POLY: memcpy(&res->o, obj, sizeof(res->o.poly)); memset(&res->o.poly.link,  0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_TEXT: memcpy(&res->o, obj, sizeof(res->o.text)); memset(&res->o.text.link,  0, sizeof(gdl_elem_t)); break;
		default:;
	}

	return res;
}

/* copy all objects of i->seg to o, starting from start_side; returns last
   object copied */
static pcb_any_obj_t *map_seg_out_copy(pcb_2netmap_t *map, pcb_2netmap_oseg_t *o, pcb_2netmap_iseg_t *i, int start_side)
{
	long n;
	if (start_side == 0) {
		for(n = 0; n < i->seg->objs.used; n++)
			vtp0_append(&o->objs, map_seg_out_obj(map, i->seg->objs.array[n]));
		return i->seg->objs.array[i->seg->objs.used-1];
	}

	for(n = i->seg->objs.used-1; n >= 0; n--)
		vtp0_append(&o->objs, map_seg_out_obj(map, i->seg->objs.array[n]));
	return i->seg->objs.array[0];
}


static pcb_any_obj_t *map_seg_out_coords(pcb_2netmap_t *map, pcb_2netmap_oseg_t *o, pcb_2netmap_iseg_t *i, int start_side)
{
	long n;
	if (start_side == 0) {
		for(n = 0; n < i->seg->objs.used; n++)
			vtp0_append(&o->objs, map_seg_out_obj(map, i->seg->objs.array[n]));
		return i->seg->objs.array[i->seg->objs.used-1];
	}

	for(n = i->seg->objs.used-1; n >= 0; n--)
		vtp0_append(&o->objs, map_seg_out_obj(map, i->seg->objs.array[n]));
	return i->seg->objs.array[0];
}

/* replace a hub object with a dummy object that acts as a bridge between 'from'
   object and the starting object of to_iseg */
static void map_seg_add_bridge(pcb_2netmap_t *map, pcb_2netmap_oseg_t *oseg, pcb_any_obj_t *from,  pcb_any_obj_t *hub_obj, pcb_2netmap_iseg_t *to_iseg, int to_start_side)
{
	pcb_any_obj_t *to;
	pcb_2netmap_obj_t *tmp = calloc(sizeof(pcb_2netmap_obj_t), 1);

	assert(to_iseg->seg->objs.used > 0);
	if (to_start_side == 0)
		to = to_iseg->seg->objs.array[0];
	else
		to = to_iseg->seg->objs.array[to_iseg->seg->objs.used - 1];

	switch(hub_obj->type) {
		case PCB_OBJ_LINE:
			tmp->o.line = *(pcb_line_t *)hub_obj;
			map_seg_get_end_coords_on_line(from, (pcb_line_t *)hub_obj, &tmp->o.line.Point1.X, &tmp->o.line.Point1.Y);
			map_seg_get_end_coords_on_line(to,   (pcb_line_t *)hub_obj, &tmp->o.line.Point2.X, &tmp->o.line.Point2.Y);
			memset(&tmp->o.line.link, 0, sizeof(tmp->o.line.link));
			break;
		case PCB_OBJ_RAT:
			abort();
		case PCB_OBJ_ARC:
			{
				double sa, ea;
				pcb_arc_t *hub_arc = (pcb_arc_t *)hub_obj;
				map_seg_get_end_coords_on_arc(from, hub_arc, &sa);
				map_seg_get_end_coords_on_arc(to,   hub_arc, &ea);
				tmp->o.arc = *hub_arc;

				/* check in which direction middle of the new arc would still fall
				   onto the original arc; this decides the angle range so the
				   new arc curves the right direction */
				if (pcb_angle_in_arc(hub_arc, sa+(ea-sa)/2, 1)) {
					tmp->o.arc.StartAngle = sa;
					tmp->o.arc.Delta = ea - sa;
				}
				else {
					tmp->o.arc.StartAngle = ea;
					tmp->o.arc.Delta = 360-(ea - sa);
				}
				memset(&tmp->o.arc.link, 0, sizeof(tmp->o.arc.link));
			}
			break;
		case PCB_OBJ_PSTK:
			tmp->o.pstk = *(pcb_pstk_t *)hub_obj;
			memset(&tmp->o.pstk.link, 0, sizeof(tmp->o.pstk.link));
			break;

		default:;
			tmp->o.line.type = PCB_OBJ_VOID;
			break;
	}
	vtp0_append(&oseg->objs, tmp);
}

static void oseg_map_coords(pcb_2netmap_t *map, pcb_2netmap_oseg_t *oseg)
{
	long n;
	pcb_2netmap_obj_t *curr, *prev = NULL;
	rnd_coord_t px, py;

	curr = oseg->objs.array[0];
	pcb_obj_center(&curr->o.any, &px, &py);
	curr->x = px;
	curr->y = py;

	/* find connected endpoints */
	for(n = 0; n < oseg->objs.used; n++) {
		curr = oseg->objs.array[n];
		if (prev != NULL) {
			if (prev->o.any.type == PCB_OBJ_PSTK) {
				
			}
			else if (curr->o.any.type == PCB_OBJ_PSTK) {
				
			}
			else if (map_seg_get_isc_coords(&prev->o.any, &curr->o.any, &curr->x, &curr->y) != 0)
				printf("map coords fail %ld (prev=#%ld/%d curr=#%ld/%d)\n", n, prev->o.any.ID, prev->o.any.type, curr->o.any.ID, curr->o.any.type);
		}
		prev = curr;
		px = prev->x;
		py = prev->y;
	}
}

static void map_seg_out(pcb_2netmap_t *map, pcb_2netmap_iseg_t *iseg)
{
	pcb_2netmap_iseg_t *prev, *es;
	pcb_2netmap_oseg_t *oseg = oseg_new(map, iseg->net, iseg->shorted);
	int n, start_side = iseg->term[0] ? 0 : 1; /* assumes we are starting from a terminal */
	pcb_any_obj_t *end_obj, *hub_last_obj = NULL, *last_obj = NULL, *hub_obj;

	for(;;) {
		/* copy current iseg */
printf("* iseg: %p %d\n", (void *)iseg, start_side);
		if (iseg->seg->hub) {
			hub_last_obj = last_obj;
			hub_obj = iseg->seg->objs.array[0];
		}
		else {
			if (hub_last_obj != NULL) {
				map_seg_add_bridge(map, oseg, hub_last_obj, hub_obj, iseg, start_side);
				hub_last_obj = NULL;
			}
			last_obj = map_seg_out_copy(map, oseg, iseg, start_side);
		}

		prev = iseg;
		iseg = iseg->path_next;
		if (iseg == NULL)
			break;

		end_obj = NULL;

printf("* prev term: %d %d\n", prev->term[0], prev->term[1]);

		/* figure the common object and determine start_side for the new iseg */
		end_obj = NULL;
		for(n = 0; n < 2; n++) {
			printf("* junc: %d %p\n", n, (void *)prev->seg->junction[n]);
			if (prev->seg->junction[n] != NULL) {
				es = htpp_get(&map->o2n, prev->seg->junction[n]);
				if (es == iseg) {
					end_obj = prev->seg->junction[n];
					break;
				}
			}
		}

		assert(end_obj != NULL);
		if (iseg->seg->objs.array[0] == end_obj)
			start_side = 0;
		else 
			start_side = 1;
	}

	oseg_map_coords(map, oseg);
}

/*** Search a path from a starting segment to any terminal using the A* algorithm ***/

typedef struct ast_ctx_s {
	pcb_2netmap_t *map;
	pcb_2netmap_iseg_t *start;
} ast_ctx_t;

static long heur(usrch_a_star_t *ctx, void *node_)
{
	ast_ctx_t *actx = ctx->user_data;
	pcb_2netmap_iseg_t *node = node_;
	long score = node->seg->objs.used;

	/* avoid shorts: overlaps should happen on legit segs */
	if (node->net != actx->start->net) score += 1000;

	/* try to use new segment: least overlap */
	if (node->used) score += 300;

	/* avoid bridge segments: go for paths with less junctions */
	if (!node->term[0] && !node->term[1]) score += 200;

	return score;
}


static long cost(usrch_a_star_t *ctx, void *from, void *to_)
{
	return heur(ctx, to_);
}

static void *neighbor_pre(usrch_a_star_t *ctx, void *curr)
{
	static int count;
	count = 0;
	return &count;
}

static void *neighbor(usrch_a_star_t *ctx, void *curr_, void *nctx)
{
	ast_ctx_t *actx = ctx->user_data;
	pcb_2netmap_iseg_t  *curr = curr_;
	pcb_any_obj_t *obj = NULL;
	void *res;
	int *count = nctx;

	for(;;) {
		if (*count == 2)
			return NULL;

		obj = curr->seg->junction[*count];
		(*count)++;
		if (obj != NULL) {
			res = htpp_get(&actx->map->o2n, obj);
			if (res != NULL)
				break;
		}
	}

	return res;
}

static int is_target(usrch_a_star_t *ctx, void *curr_)
{
	ast_ctx_t *actx = ctx->user_data;
	pcb_2netmap_iseg_t *curr = curr_;
	return ((curr != actx->start) && (curr->term[0] || curr->term[1]));
}

static void set_mark(usrch_a_star_t *ctx, void *node, usrch_a_star_node_t *mark)
{
	pcb_2netmap_iseg_t *seg = node;
	seg->mark = mark;
}

static usrch_a_star_node_t *get_mark(usrch_a_star_t *ctx, void *node)
{
	pcb_2netmap_iseg_t *seg = node;
	return seg->mark;
}

static void map_seg_search(pcb_2netmap_t *map, pcb_2netmap_iseg_t *iseg)
{
	usrch_a_star_node_t *it;
	usrch_a_star_t a = {0};
	ast_ctx_t actx;
	pcb_2netmap_iseg_t *n, *last;
	usrch_res_t sr;

	actx.map = map;
	actx.start = iseg;

	a.heuristic = heur;
	a.cost = cost;
	a.is_target = is_target;
	a.neighbor_pre = neighbor_pre;
	a.neighbor = neighbor;
	a.set_mark = set_mark;
	a.get_mark = get_mark;
	a.user_data = &actx;

	sr = usrch_a_star_search(&a, iseg, NULL);

printf("-------------------\n");

	if (sr == USRCH_RES_FOUND) {
		/* pick up the the results and build a path using ->path_next and render the output net */
		last = NULL;
		for(n = usrch_a_star_path_first(&a, &it); n != NULL; n = usrch_a_star_path_next(&a, &it)) {
printf(" + %p\n", (void *)n);
			n->path_next = last;
			last = n;
			n->used = 1;
		}
	}
	else {
		/* didn't find any connection, iseg is an unfinished net */
		last = iseg;
		last->path_next = NULL;
	}
	map_seg_out(map, last);

	usrch_a_star_uninit(&a);
}

static void map_segs(pcb_2netmap_t *map)
{
	pcb_2netmap_iseg_t *i;

	/* search nets that start or end in a terminal */
	for(i = map->isegs; i != NULL; i = i->next) {
		if (i->used) continue;
		if (i->term[0] && i->term[1]) /* simplest case: terminal-to-terminal */
			map_seg_out(map, i);
		else if (i->term[0] || i->term[1])
			map_seg_search(map, i);
	}
}