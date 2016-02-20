#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "coordgeo.h"

#define POINT_MARK_SIZE 2
#define PS_SCALE 3
#define PS_MARGIN 8

#define SCALE(v) ((v+PS_MARGIN) * PS_SCALE)

void line(FILE *ps, int x1, int y1, int x2, int y2)
{
	fprintf(ps, "%d %d moveto\n", SCALE(x1), SCALE(y1));
	fprintf(ps, "%d %d lineto\n", SCALE(x2), SCALE(y2));
}

void color(FILE *ps, float r, float g, float b)
{
	static int colorspace=0;

	if (colorspace == 0) {
		fprintf(ps, "/DeviceRGB setcolorspace\n");
		colorspace = 1;
	}
	fprintf(ps, "%f %f %f setcolor\n", r, g, b);
}


FILE *ps_start(const char *fn)
{
	FILE *f;

	f = fopen(fn, "w");
	if (f == NULL)
		return NULL;

	fprintf(f, "%%!\n");

	return f;
}

int ps_draw(FILE *ps, int ID)
{
	cg_obj_t *o;
	o = cg_get_object(ID);
	assert(o != NULL);
	fprintf(ps, "newpath\n");

	switch(o->type) {
		case CG_POINT:
			fprintf(ps, "0.1 setlinewidth\n");
			line(ps, o->x1-POINT_MARK_SIZE, o->y1, o->x1+POINT_MARK_SIZE, o->y1);
			line(ps, o->x1, o->y1-POINT_MARK_SIZE, o->x1, o->y1+POINT_MARK_SIZE);
			break;
		case CG_LINE:
			fprintf(ps, "0.1 setlinewidth\n");
			line(ps, o->x1, o->y1, o->x2, o->y2);
			break;
		case CG_VECTOR:
			fprintf(ps, "0.1 setlinewidth\n");
			line(ps, o->x1, o->y1, o->x2, o->y2);
			break;
		case CG_CIRCLE:
			fprintf(ps, "0.1 setlinewidth\n");
			fprintf(ps, "%d %d %d %d %d arc\n", SCALE(o->x1), SCALE(o->y1), o->r*PS_SCALE, 0, 360);
			break;
		case CG_ARC:
			fprintf(ps, "0.1 setlinewidth\n");
			fprintf(ps, "%d %d %d %d %d arc\n", SCALE(o->x1), SCALE(o->y1), o->r*PS_SCALE, o->x2, o->y2);
			break;
	}

	fprintf(ps, "stroke\n");	
	return ID;
}

void ps_end(FILE *ps)
{
	fprintf(ps, "showpage\n");
	fclose(ps);
}

void test_primitives()
{
	FILE *ps;

	ps = ps_start("primitives.ps");
	cg_destroy_object(ps_draw(ps, cg_new_object(CG_LINE, 0, 0, 10, 10, 0)));
	cg_destroy_object(ps_draw(ps, cg_new_object(CG_VECTOR, 20, 0, 30, 10, 0)));
	cg_destroy_object(ps_draw(ps, cg_new_object(CG_POINT, 40, 5, 0, 0, 0)));
	cg_destroy_object(ps_draw(ps, cg_new_object(CG_CIRCLE, 70, 10, 0, 0, 5)));
	cg_destroy_object(ps_draw(ps, cg_new_object(CG_ARC, 90, 10, 0, 45, 5)));
	ps_end(ps);
}

void test_parallels()
{
	FILE *ps;
	int obj;

	ps = ps_start("parallels.ps");

	obj = ps_draw(ps, cg_new_object(CG_LINE, 0, 0, 10, 10, 0));
	cg_destroy_object(ps_draw(ps, cg_para_obj(obj, 4)));
	cg_destroy_object(ps_draw(ps, cg_para_obj(obj, -4)));
	cg_destroy_object(obj);

	obj = ps_draw(ps, cg_new_object(CG_CIRCLE, 70, 10, 0, 0, 5));
	cg_destroy_object(ps_draw(ps, cg_para_obj(obj, 4)));
	cg_destroy_object(ps_draw(ps, cg_para_obj(obj, -4)));
	cg_destroy_object(obj);

/*	obj = ps_draw(ps, cg_new_object(CG_ARC, 90, 10, 0, 45, 5));
	cg_destroy_object(ps_draw(ps, cg_para_obj(obj, 4)));
	cg_destroy_object(ps_draw(ps, cg_para_obj(obj, -4)));
	cg_destroy_object(obj);*/


	ps_end(ps);

}

void test_perps()
{
	FILE *ps;
	int obj;

	ps = ps_start("perpendicular.ps");

	obj = ps_draw(ps, cg_new_object(CG_LINE, 0, 0, 10, 10, 0));
	cg_destroy_object(ps_draw(ps, cg_perp_line(obj, 0,0,4)));
	cg_destroy_object(ps_draw(ps, cg_perp_line(obj, 10,10,-4)));
	cg_destroy_object(obj);

	ps_end(ps);

}

void test_intersect()
{
	FILE *ps;
	int o1, o2, i;
	cg_obj_t *oi;

	ps = ps_start("intersect.ps");

	/* two lines, intersecting */
	color(ps, 0, 0, 0);
	o1 = ps_draw(ps, cg_new_object(CG_LINE, 0, 0, 10, 10, 0));
	o2 = ps_draw(ps, cg_new_object(CG_LINE, 5, 0, 6, 13, 0));
	color(ps, 1, 0, 0);
	cg_simplify_object(i = cg_intersect(o1, o2, 1));
	cg_destroy_object(ps_draw(ps, i));
	cg_destroy_object(o1);
	cg_destroy_object(o2);

	/* two lines, no visible intersection */
	color(ps, 0, 0, 0);
	o1 = ps_draw(ps, cg_new_object(CG_LINE, 20, 0, 30, 10, 0));
	o2 = ps_draw(ps, cg_new_object(CG_LINE, 25, 0, 32, 13, 0));
	color(ps, 1, 0, 0);
	cg_simplify_object(i = cg_intersect(o1, o2, 1));
	cg_destroy_object(ps_draw(ps, i));
	cg_destroy_object(o1);
	cg_destroy_object(o2);

	/* circle vs. line */
	color(ps, 0, 0, 0);
	o1 = ps_draw(ps, cg_new_object(CG_LINE,   40, 0, 55, 15, 0));
	o2 = ps_draw(ps, cg_new_object(CG_CIRCLE, 45, 8, 0, 0, 6));
	color(ps, 1, 0, 0);
	cg_simplify_object(i = cg_intersect(o1, o2, 1));
	cg_destroy_object(ps_draw(ps, i));
	cg_destroy_object(o1);
	cg_destroy_object(o2);

	/* circle vs. circle */
	color(ps, 0, 0, 0);
	o1 = ps_draw(ps, cg_new_object(CG_CIRCLE, 69, 6, 0, 0, 4));
	o2 = ps_draw(ps, cg_new_object(CG_CIRCLE, 65, 8, 0, 0, 6));
	color(ps, 1, 0, 0);
	cg_simplify_object(i = cg_intersect(o1, o2, 1));
	cg_destroy_object(ps_draw(ps, i));
	cg_destroy_object(o1);
	cg_destroy_object(o2);


	ps_end(ps);
}


int main()
{
	test_primitives();
	test_parallels();
	test_perps();
	test_intersect();
}
