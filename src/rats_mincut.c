#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "global.h"

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "misc.h"
#include "mymem.h"
#include "polygon.h"
#include "rats.h"
#include "search.h"
#include "set.h"
#include "undo.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "rats.h"
#include "pcb-mincut/graph.h"

typedef struct short_conn_s short_conn_t;
struct short_conn_s {
	int gid; /* id in the graph */
/*	int from_type;
	AnyObjectType *from;*/
	int from_id;
	int to_type;
	AnyObjectType *to;
	found_conn_type_t type;
	short_conn_t *next;
};

static short_conn_t *short_conns = NULL;
static int num_short_conns = 0;
static int short_conns_maxid = 0;

static void proc_short_cb(int current_type, void *current_obj, int from_type, void *from_obj, found_conn_type_t type)
{
	AnyObjectType *curr = current_obj, *from = from_obj;
	short_conn_t *s;

	s = malloc(sizeof(short_conn_t));
/*	s->from_type = from_type;
	s->from = from;*/
	s->from_id = from == NULL ? -1 : from->ID;
	s->to_type = current_type;
	s->to = curr;
	s->type = type;
	s->next = short_conns;
	short_conns = s;
	if (curr->ID > short_conns_maxid)
		short_conns_maxid = curr->ID;
	num_short_conns++;




	printf(" found %d %d/%p type %d from %d\n", current_type, curr->ID, current_obj, type, from == NULL ? -1 : from->ID);


}

static void proc_short(PinType *pin, PadType *pad)
{
	find_callback_t old_cb;
	Coord x, y;
	short_conn_t *n, **lut_by_oid, **lut_by_gid, *next;
	int gids;
	gr_t *g;
	void *S, *T;
	int *solution;
	int i;

	/* only one should be set, but one must be set */
	assert((pin != NULL) || (pad != NULL));
	assert((pin == NULL) || (pad == NULL));

	if (pin != NULL) {
		printf("short on pin!\n");
		SET_FLAG (WARNFLAG, pin);
		x = pin->X;
		y = pin->Y;
	}
	else if (pad != NULL) {
		printf("short on pad!\n");
		SET_FLAG (WARNFLAG, pad);
		if (TEST_FLAG (EDGE2FLAG, pad)) {
			x = pad->Point2.X;
			y = pad->Point2.Y;
		}
		else {
			x = pad->Point1.X;
			y = pad->Point1.Y;
		}
	}

short_conns = NULL;
num_short_conns = 0;
short_conns_maxid = 0;

	/* perform a search using MINCUTFLAG, calling back proc_short_cb() with the connections */
	old_cb = find_callback;
	find_callback = proc_short_cb;
	SaveFindFlag(MINCUTFLAG);
	LookupConnection (x, y, false, 1, MINCUTFLAG);

	printf("- alloced for %d\n", (short_conns_maxid+1));
	lut_by_oid = calloc(sizeof(short_conn_t *), (short_conns_maxid+1));
	lut_by_gid = calloc(sizeof(short_conn_t *), (num_short_conns+3));

	g = gr_alloc(num_short_conns+2);

	g->node2name = calloc(sizeof(char *), (num_short_conns+2));

	/* conn 0 is S and conn 1 is T */
	for(n = short_conns, gids=2; n != NULL; n = n->next, gids++) {
		char *s, *typ;
		n->gid = gids;
		printf(" {%d} found %d %d/%p type %d from %d\n", n->gid, n->to_type, n->to->ID, n->to, n->type, n->from_id);
		lut_by_oid[n->to->ID] = n;
		lut_by_gid[n->gid] = n;
		
		s = malloc(256);
		switch(n->to_type) {
			case PIN_TYPE: typ = "pin"; break;
			case VIA_TYPE: typ = "via"; break;
			case PAD_TYPE: typ = "pad"; break;
			case LINE_TYPE: typ = "line"; break;
			default: typ="other"; break;
		}
		sprintf(s, "%s #%d", typ, n->to->ID);
		g->node2name[n->gid] = s;
	}
	g->node2name[0] = strdup("S");
	g->node2name[1] = strdup("T");

	S = NULL;
	T = NULL;
	for(n = short_conns; n != NULL; n = n->next) {
		short_conn_t *from;
		void *spare;

		spare = NULL;
		if (n->to_type == PIN_TYPE)
			spare = ((PinType *)n->to)->Spare;
		if (n->to_type == PAD_TYPE)
			spare = ((PadType *)n->to)->Spare;
		if (spare != NULL) {
			void *net = &(((LibraryMenuTypePtr)spare)->Name[2]);
			printf(" net=%s\n", net);
			if (S == NULL) {
				printf(" -> became S\n");
				S = net;
			}
			else if ((T == NULL) && (net != S)) {
				printf(" -> became T\n");
				T = net;
			}

			if (net == S)
				gr_add_(g, n->gid, 0, 100000);
			else if (net == T)
				gr_add_(g, n->gid, 1, 100000);
		}
		/* if we have a from object, look it up and make a connection between the two gids */
		if (n->from_id >= 0) {
			int weight;
			from = lut_by_oid[n->from_id];
			/* weight: 1 for connections we can break, large value for connections we shall not break */
			if ((n->type == FCT_COPPER) || (n->type == FCT_START))
				weight = 1;
			else
				weight = 10000;
			if (from != NULL) {
				gr_add_(g, n->gid, from->gid, weight);
				printf(" CONN %d %d\n", n->gid, from->gid);
			}
		}
	}

	static int drw = 0;
	char gfn[256];
	drw++;
	sprintf(gfn, "A_%d_a", drw);
	printf("gfn=%s\n", gfn);
	gr_draw(g, gfn, "png");

	solution = solve(g);

/*	sprintf(gfn, "A_%d_b", drw);
	printf("gfn=%s\n", gfn);
	gr_draw(g, gfn, "png");*/

	printf("Would cut:\n");
	for(i = 0; solution[i] != -1; i++) {
		short_conn_t *s;
		printf("%d:", i);
		s = lut_by_gid[solution[i]];
		printf("%d %p", solution[i], s);
		if (s != NULL) {
			SET_FLAG (WARNFLAG, s->to);
			printf("  -> %d", s->to->ID);
		}
		printf("\n");
	}

	free(solution);
	free(lut_by_oid);
	free(lut_by_gid);

	for(n = short_conns; n != NULL; n = next) {
		next = n->next;
		free(n);
	}


	ResetFoundLinesAndPolygons(false);
	ResetFoundPinsViasAndPads(false);
	RestoreFindFlag();

	find_callback = old_cb;
}

typedef struct pinpad_s pinpad_t;
struct pinpad_s {
	PinType *pin;
	PadType *pad;
	pinpad_t *next;
};

static pinpad_t *shorts = NULL;

void rat_found_short(PinType *pin, PadType *pad)
{
	pinpad_t *pp;
	pp = malloc(sizeof(pinpad_t));
	pp->pin = pin;
	pp->pad = pad;
	pp->next = shorts;
	shorts = pp;
}

void rat_proc_shorts(void)
{
	pinpad_t *n, *next;
	for(n = shorts; n != NULL; n = next) {
		next = n->next;
		proc_short(n->pin, n->pad);
		free(n);
	}
	shorts = NULL;
}
