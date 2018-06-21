#include <stdio.h>
#include <time.h>
#include "cdt.h"

cdt_t cdt;

int main(void)
{
	int i;
	clock_t t;
	//cdt_init(&cdt, 1000, 1000, 5000, 5000);
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

	srand(time(NULL));
	t = clock();
	for (i = 0; i < 1000; i++) {
		cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
	}
	t = clock() - t;
	fprintf(stderr, "Triangulation time: %f\n", ((float)t)/CLOCKS_PER_SEC);


	if (cdt_check_delaunay(&cdt))
		fprintf(stderr, "delaunay\n");
	else
		fprintf(stderr, "not delaunay\n");
	cdt_dump_animator(&cdt, 0);
}
