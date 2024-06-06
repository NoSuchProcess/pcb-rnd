#include "obj_arc.h"
#include "obj_line.h"

#include "ht_endp.h"

typedef enum { /* bitfield inidicating the role of this object inside a curve */
	ENDP_ST_MID = 0,     /* if neither START nor END, it's in the middle */
	ENDP_ST_START = 1,   /* first object in a new curve */
	ENDP_ST_END = 2,     /* last object in a curve */
	ENDP_ST_REVERSE = 4  /* draw the object from the second endpoint to the first */
} endp_state_t;

typedef struct hpgl_line_s {
	void *next;
	pcb_flag_t Flags;
	pcb_objtype_t type;
	rnd_cheap_point_t Point1, Point2;
	double length;
} hpgl_line_t;

typedef struct hpgl_line_s hpgl_any_obj_t;

typedef struct hpgl_arc_s {
	void *next;
	pcb_flag_t Flags;
	pcb_objtype_t type;
	rnd_cheap_point_t Point1, Point2;
	double length;

	rnd_coord_t R, X, Y;
	rnd_angle_t StartAngle, Delta;
} hpgl_arc_t;


void hpgl_add_arc(htendp_t *ht, hpgl_arc_t *a, pcb_dynf_t flg);
void hpgl_add_line(htendp_t *ht, hpgl_line_t *l, pcb_dynf_t dflg);

void hpgl_endp_init(htendp_t *ht);
void hpgl_endp_uninit(htendp_t *ht);
void hpgl_endp_render(htendp_t *ht, void (*cb)(void *uctx, hpgl_any_obj_t *o, endp_state_t st), void *uctx, pcb_dynf_t dflg);


