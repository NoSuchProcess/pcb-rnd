#include <stdio.h>
#include <time.h>
#include "cdt.h"

#define ORIENT_CCW(a, b, c) (orientation(a, b, c) < 0)
#define ORIENT_CW(a, b, c) (orientation(a, b, c) > 0)
/* TODO: check epsilon for collinear case? */
#define ORIENT_COLLINEAR(a, b, c) (orientation(a, b, c) == 0)
#define ORIENT_CCW_CL(a, b, c) (orientation(a, b, c) <= 0)
static double orientation(point_t *p1, point_t *p2, point_t *p3)
{
	return ((double)p2->pos.y - (double)p1->pos.y) * ((double)p3->pos.x - (double)p2->pos.x)
				 - ((double)p2->pos.x - (double)p1->pos.x) * ((double)p3->pos.y - (double)p2->pos.y);
}
#define LINES_INTERSECT(p1, q1, p2, q2) \
	(ORIENT_CCW(p1, q1, p2) != ORIENT_CCW(p1, q1, q2) && ORIENT_CCW(p2, q2, p1) != ORIENT_CCW(p2, q2, q1))

cdt_t cdt;

int main(void)
{
	point_t *p, *p1, *p2, *pd[1000];
	edge_t *e, *ed[500];
	int i, j, e_num;
	clock_t t;
	pointlist_node_t *p_violations = NULL;

	cdt_init(&cdt, 1000, 1000, 5000, 5000);
	//cdt_init(&cdt, 0, 0, 100000, 100000);

	/*
	cdt_insert_point(&cdt, 2500, 3000);
	cdt_insert_point(&cdt, 2600, 3000);
	cdt_insert_point(&cdt, 2700, 3000);
	cdt_insert_point(&cdt, 2800, 3000);
	cdt_insert_point(&cdt, 2900, 3000);
	cdt_insert_point(&cdt, 3000, 3000);
	cdt_insert_point(&cdt, 3100, 3000);
	cdt_insert_point(&cdt, 3200, 3000);
	cdt_insert_point(&cdt, 3300, 3000);
	cdt_insert_point(&cdt, 2800, 4800);
	*/

	/*
	cdt_insert_point(&cdt, 2700, 2700);
	cdt_insert_point(&cdt, 2400, 2400);
	cdt_insert_point(&cdt, 2400, 3000);

	cdt_insert_point(&cdt, 3400, 2700);
	cdt_insert_point(&cdt, 3700, 2400);
	cdt_insert_point(&cdt, 3700, 3000);

	cdt_insert_point(&cdt, 2900, 2700);
	cdt_insert_point(&cdt, 3000, 2700);
	cdt_insert_point(&cdt, 3100, 2700);
	cdt_insert_point(&cdt, 3200, 2700);
	*/

	/*
	srand(time(NULL));
	t = clock();
	for (i = 0; i < 1000; i++) {
		cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
	}
	t = clock() - t;
	fprintf(stderr, "Triangulation time: %f\n", ((float)t)/CLOCKS_PER_SEC);
	*/

	/* octagon */
	/*
	cdt_insert_point(&cdt, 4000, 3000);
	cdt_insert_point(&cdt, 3700, 3700);
	cdt_insert_point(&cdt, 3000, 4000);
	cdt_insert_point(&cdt, 2300, 3700);
	cdt_insert_point(&cdt, 2000, 3000);
	cdt_insert_point(&cdt, 2300, 2300);
	cdt_insert_point(&cdt, 3000, 2000);
	cdt_insert_point(&cdt, 3700, 2300);

	p = cdt_insert_point(&cdt, 3000, 3000);
	cdt_delete_point(&cdt, p);
	*/

	/* concave poly */
	/*
	cdt_insert_point(&cdt, 2000, 4000);
	cdt_insert_point(&cdt, 3000, 3500);
	cdt_insert_point(&cdt, 4000, 4000);
	cdt_insert_point(&cdt, 4000, 2000);
	cdt_insert_point(&cdt, 2000, 2000);

	p = cdt_insert_point(&cdt, 3000, 3000);
	cdt_delete_point(&cdt, p);
	*/

	/* constrained edge */
	/*
	p1 = cdt_insert_point(&cdt, 1500, 3000);
	cdt_insert_point(&cdt, 2000, 3500);
	cdt_insert_point(&cdt, 2500, 2500);
	cdt_insert_point(&cdt, 3000, 3500);
	cdt_insert_point(&cdt, 3500, 2500);
	cdt_insert_point(&cdt, 4000, 3500);
	p2 = cdt_insert_point(&cdt, 4500, 3000);

	e = cdt_insert_constrained_edge(&cdt, p1, p2);
	cdt_delete_constrained_edge(&cdt, e);
	*/

	/* constrained edge 2 */
	/*
	p1 = cdt_insert_point(&cdt, 1500, 3800);
	cdt_insert_point(&cdt, 2700, 3000);
	cdt_insert_point(&cdt, 2800, 3000);
	cdt_insert_point(&cdt, 2900, 3000);
	cdt_insert_point(&cdt, 3000, 3000);
	cdt_insert_point(&cdt, 3100, 3000);
	cdt_insert_point(&cdt, 3200, 3000);
	cdt_insert_point(&cdt, 3300, 3000);
	p2 = cdt_insert_point(&cdt, 4500, 3800);
	cdt_insert_point(&cdt, 3000, 4800);

	e = cdt_insert_constrained_edge(&cdt, p1, p2);
	*/

	/*
	srand(time(NULL));
	for (i = 0; i < 1000; i++) {
		cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
		pd[i] = cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
	}
	for (i = 0; i < 1000; i++) {
		cdt_delete_point(&cdt, pd[i]);
	}
	*/

	/* tricky case for triangulate_polygon */
	/*
	p = cdt_insert_point(&cdt, 98347, 51060);
	cdt_insert_point(&cdt, 92086, 47852);
	cdt_insert_point(&cdt, 95806, 44000);
	cdt_insert_point(&cdt, 95030, 41242);
	cdt_insert_point(&cdt, 94644, 55629);
	cdt_insert_point(&cdt, 94000, 45300);

	cdt_delete_point(&cdt, p);
	*/

	/* delete constrained edge */
	/*
	p1 = cdt_insert_point(&cdt, 1500, 3000);
	cdt_insert_point(&cdt, 2000, 3500);
	cdt_insert_point(&cdt, 1900, 2500);
	cdt_insert_point(&cdt, 2400, 2500);

	cdt_insert_point(&cdt, 2800, 3500);
	cdt_insert_point(&cdt, 3000, 3500);
	cdt_insert_point(&cdt, 3200, 3500);

	cdt_insert_point(&cdt, 3600, 2500);
	cdt_insert_point(&cdt, 4100, 2500);
	cdt_insert_point(&cdt, 4000, 3500);
	p2 = cdt_insert_point(&cdt, 4500, 3000);

	e = cdt_insert_constrained_edge(&cdt, p1, p2);
	cdt_delete_constrained_edge(&cdt, e);
	*/

	/* delete 2 constrained edges */
	p1 = cdt_insert_point(&cdt, 2400, 1500);
	cdt_insert_point(&cdt, 2200, 3500);
	cdt_insert_point(&cdt, 2200, 2000);
	p2 = cdt_insert_point(&cdt, 2400, 4000);
	ed[0] = cdt_insert_constrained_edge(&cdt, p1, p2);

	p1 = cdt_insert_point(&cdt, 2500, 3000);
	cdt_insert_point(&cdt, 3000, 2800);
	cdt_insert_point(&cdt, 3500, 2800);
	cdt_insert_point(&cdt, 2800, 3200);
	cdt_insert_point(&cdt, 3500, 3200);
	p2 = cdt_insert_point(&cdt, 4500, 3000);
	ed[1] = cdt_insert_constrained_edge(&cdt, p1, p2);

	p2 = cdt_insert_point(&cdt, 3500, 3500);
	ed[2] = cdt_insert_constrained_edge(&cdt, p1, p2);

	cdt_delete_constrained_edge(&cdt, ed[0]);
	cdt_delete_constrained_edge(&cdt, ed[1]);
	cdt_delete_constrained_edge(&cdt, ed[2]);

	/*
	srand(time(NULL));
	e_num = 0;
	for (i = 0; i < 100; i++) {
		int isect = 0;
		p1 = cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
		p2 = cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
		for (j = 0; j < e_num; j++)
			if (LINES_INTERSECT(p1, p2, ed[j]->endp[0], ed[j]->endp[1])) {
				isect = 1;
				break;
			}
		if (!isect) {
			ed[e_num] = cdt_insert_constrained_edge(&cdt, p1, p2);
			e_num++;
		}
	}
	for (i = 0; i < e_num; i++) {
		fprintf(stderr, "deleting edge (%d, %d) - (%d, %d)\n", ed[i]->endp[0]->pos.x, ed[i]->endp[0]->pos.y, ed[i]->endp[1]->pos.x, ed[i]->endp[1]->pos.y);
		cdt_delete_constrained_edge(&cdt, ed[i]);
	}
	*/

	if (cdt_check_delaunay(&cdt, &p_violations, NULL))
		fprintf(stderr, "delaunay\n");
	else
		fprintf(stderr, "not delaunay\n");
	cdt_dump_animator(&cdt, 0, p_violations, NULL);
}
