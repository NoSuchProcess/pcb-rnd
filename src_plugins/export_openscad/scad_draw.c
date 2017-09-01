#include "../lib_polyhelp/topoly.h"


#define TRX(x)
#define TRY(y) \
do { \
	y = PCB->MaxHeight - y; \
} while(0)

static int scad_draw_outline(void)
{
	pcb_any_obj_t *start = pcb_topoly_find_1st_outline(PCB);
	pcb_polygon_t *poly;
	int n;

	if (start == NULL) {
		poly = pcb_poly_alloc(PCB->Data->Layer);
		pcb_poly_point_new(poly, 0, 0);
		pcb_poly_point_new(poly, PCB->MaxWidth, 0);
		pcb_poly_point_new(poly, PCB->MaxWidth, PCB->MaxHeight);
		pcb_poly_point_new(poly, 0, PCB->MaxHeight);
	}
	else {
		poly = pcb_topoly_conn(PCB, start, 0);
		if (poly == NULL)
			return -1;
	}

	fprintf(f, "module pcb_outline() {\n");
	fprintf(f, "	polygon([\n\t\t");
	/* we need the as-drawn polygon and we know there are no holes */
	for(n = 0; n < poly->PointN; n++)
		pcb_fprintf(f, "[%mm,%mm]%s", poly->Points[n].X, poly->Points[n].X, ((n < (poly->PointN-1)) ? "," : "\n"));
	fprintf(f, "	]);\n");
	fprintf(f, "}\n");

	pcb_poly_free(poly);
	return 0;
}

static void scad_draw_finish()
{
	fprintf(f, "module pcb_board() {\n");
	fprintf(f, "	linear_extrude(height=1.6) board_outline();\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");
	fprintf(f, "pcb_board()");
}
