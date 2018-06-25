#include <stdio.h>
#include <time.h>
#include "cdt.h"

cdt_t cdt;

int main(void)
{
	point_t *p, *p1, *p2, *pd[1000];
	edge_t *e;
	int i;
	clock_t t;
	pointlist_node_t *p_violations = NULL;

	/*cdt_init(&cdt, 1000, 1000, 5000, 5000);*/
	cdt_init(&cdt, 0, 0, 100000, 100000);

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

	srand(time(NULL));
	for (i = 0; i < 1000; i++) {
		cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
		pd[i] = cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
	}
	for (i = 0; i < 1000; i++) {
		cdt_delete_point(&cdt, pd[i]);
	}

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

	if (cdt_check_delaunay(&cdt, &p_violations, NULL))
		fprintf(stderr, "delaunay\n");
	else
		fprintf(stderr, "not delaunay\n");
	cdt_dump_animator(&cdt, 0, p_violations, NULL);
}
