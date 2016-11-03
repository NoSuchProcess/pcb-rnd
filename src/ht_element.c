#include <string.h>
#include "config.h"
#include "global_element.h"
#include "list_element.h"

#define HT(x) htep_ ## x
#include <genht/ht.c>
#undef HT


#warning TODO: move these in the big split

/* compare two strings and return 0 if they are equal. NULL == NULL means equal. */
static int neqs(const char *s1, const char *s2)
{
	if ((s1 == NULL) && (s2 == NULL)) return 0;
	if ((s1 == NULL) || (s2 == NULL)) return 1;
	return strcmp(s1, s2) != 0;
}

/* compare two fields and return 0 if they are equal */
#define neq(s1, s2, f) ((s1)->f != (s2)->f)

#define eoffs(e,ef, s,sf) ((e == NULL) ? (s)->sf : ((s)->sf) - ((e)->ef))
#define neqox(e1, x1, e2, x2, f) (eoffs(e1, MarkX, x1, f) != eoffs(e2, MarkX, x2, f))
#define neqoy(e1, y1, e2, y2, f) (eoffs(e1, MarkY, y1, f) != eoffs(e2, MarkY, y2, f))

static inline unsigned h_coord(Coord c)
{
	return murmurhash(&(c), sizeof(Coord));
}

#define h_coordox(e, c) ((e) == NULL ? h_coord(c) : h_coord(c - e->MarkX))
#define h_coordoy(e, c) ((e) == NULL ? h_coord(c) : h_coord(c - e->MarkY))

#define h_str(s) ((s) == NULL ? 0 : strhash(s))

int pcb_pin_eq(const ElementType *e1, const PinType *p1, const ElementType *e2, const PinType *p2)
{
	if (neq(p1, p2, Thickness) || neq(p1, p2, Clearance)) return 0;
	if (neq(p1, p2, Mask) || neq(p1, p2, DrillingHole)) return 0;
	if (neqox(e1, p1, e2, p2, X) || neqoy(e1, p1, e2, p2, Y)) return 0;
	if (neqs(p1->Name, p2->Name)) return 0;
	if (neqs(p1->Number, p2->Number)) return 0;
	return 1;
}

unsigned int pcb_pin_hash(const ElementType *e, const PinType *p)
{
	return
		h_coord(p->Thickness) ^ h_coord(p->Clearance) ^
		h_coord(p->Mask) ^ h_coord(p->DrillingHole) ^
		h_coordox(e, p->X) ^ h_coordoy(e, p->Y) ^
		h_str(p->Name) ^ h_str(p->Number);
}

int pcb_pad_eq(const ElementType *e1, const PadType *p1, const ElementType *e2, const PadType *p2)
{
	if (neq(p1, p2, Thickness) || neq(p1, p2, Clearance)) return 0;
	if (neqox(e1, p1, e2, p2, Point1.X) || neqoy(e1, p1, e2, p2, Point1.Y)) return 0;
	if (neqox(e1, p1, e2, p2, Point2.X) || neqoy(e1, p1, e2, p2, Point2.Y)) return 0;
	if (neq(p1, p2, Mask)) return 0;
	if (neqs(p1->Name, p2->Name)) return 0;
	if (neqs(p1->Number, p2->Number)) return 0;
	return 1;
}

unsigned int pcb_pad_hash(const ElementType *e, const PadType *p)
{
	return
		h_coord(p->Thickness) ^ h_coord(p->Clearance) ^
		h_coordox(e, p->Point1.X) ^ h_coordoy(e, p->Point1.Y) ^
		h_coordox(e, p->Point2.X) ^ h_coordoy(e, p->Point2.Y) ^
		h_coord(p->Mask) ^
		h_str(p->Name) ^ h_str(p->Number);
}


int pcb_line_eq(const ElementType *e1, const LineType *l1, const ElementType *e2, const LineType *l2)
{
	if (neq(l1, l2, Thickness) || neq(l1, l2, Clearance)) return 0;
	if (neqox(e1, l1, e2, l2, Point1.X) || neqoy(e1, l1, e2, l2, Point1.Y)) return 0;
	if (neqox(e1, l1, e2, l2, Point2.X) || neqoy(e1, l1, e2, l2, Point2.Y)) return 0;
	if (neqs(l1->Number, l2->Number)) return 0;
	return 1;
}


unsigned int pcb_line_hash(const ElementType *e, const LineType *l)
{
	return
		h_coord(l->Thickness) ^ h_coord(l->Clearance) ^
		h_coordox(e, l->Point1.X) ^ h_coordoy(e, l->Point1.Y) ^
		h_coordox(e, l->Point2.X) ^ h_coordoy(e, l->Point2.Y) ^
		h_str(l->Number);
}

int pcb_arc_eq(const ElementType *e1, const ArcType *a1, const ElementType *e2, const ArcType *a2)
{
	if (neq(a1, a2, Thickness) || neq(a1, a2, Clearance)) return 0;
	if (neq(a1, a2, Width) || neq(a1, a2, Height)) return 0;
	if (neqox(e1, a1, e2, a2, X) || neqoy(e1, a1, e2, a2, Y)) return 0;
	if (neq(a1, a2, StartAngle) || neq(a1, a2, Delta)) return 0;

	return 1;
}

unsigned int pcb_arc_hash(const ElementType *e, const ArcType *a)
{
	return 
		h_coord(a->Thickness) ^ h_coord(a->Clearance) ^
		h_coord(a->Width) ^ h_coord(a->Height) ^
		h_coordox(e, a->X) ^ h_coordoy(e, a->Y) ^
		h_coord(a->StartAngle) ^ h_coord(a->Delta);
}

#undef h_coord
#undef neq

unsigned int pcb_element_hash(const ElementType *e)
{
	unsigned int val = 0;
	gdl_iterator_t it;

	{
		PinType *p;
		pinlist_foreach(&e->Pin, &it, p) {
			val ^= pcb_pin_hash(e, p);
		}
	}

	{
		PadType *p;
		padlist_foreach(&e->Pad, &it, p) {
			val ^= pcb_pad_hash(e, p);
		}
	}

	{
		LineType *l;
		linelist_foreach(&e->Line, &it, l) {
			val ^= pcb_line_hash(e, l);
		}
	}

	{
		ArcType *a;
		linelist_foreach(&e->Arc, &it, a) {
			val ^= pcb_arc_hash(e, a);
		}
	}

	return val;
}

int pcb_element_eq(const ElementType *e1, const ElementType *e2)
{
	/* Require the same objects in the same order - bail out at the first mismatch */

	{
		PinType *p1, *p2;
		p1 = pinlist_first((pinlist_t *)&e1->Pin);
		p2 = pinlist_first((pinlist_t *)&e2->Pin);
		for(;;) {
			if ((p1 == NULL) && (p2 == NULL))
				break;
			if (!pcb_pin_eq(e1, p1, e2, p2))
				return 0;
			p1 = pinlist_next(p1);
			p2 = pinlist_next(p2);
		}
	}

	{
		PadType *p1, *p2;
		p1 = padlist_first((padlist_t *)&e1->Pad);
		p2 = padlist_first((padlist_t *)&e2->Pad);
		for(;;) {
			if ((p1 == NULL) && (p2 == NULL))
				break;
			if (!pcb_pad_eq(e1, p1, e2, p2))
				return 0;
			p1 = padlist_next(p1);
			p2 = padlist_next(p2);
		}
	}

	{
		LineType *l1, *l2;
		l1 = linelist_first((linelist_t *)&e1->Line);
		l2 = linelist_first((linelist_t *)&e2->Line);
		for(;;) {
			if ((l1 == NULL) && (l2 == NULL))
				break;
			if (!pcb_line_eq(e1, l1, e2, l2))
				return 0;
			l1 = linelist_next(l1);
			l2 = linelist_next(l2);
		}
	}

	{
		ArcType *a1, *a2;
		a1 = arclist_first((arclist_t *)&e1->Arc);
		a2 = arclist_first((arclist_t *)&e2->Arc);
		for(;;) {
			if ((a1 == NULL) && (a2 == NULL))
				break;
			if (!pcb_arc_eq(e1, a1, e2, a2))
				return 0;
			a1 = arclist_next(a1);
			a2 = arclist_next(a2);
		}
	}

	return 1;
}

