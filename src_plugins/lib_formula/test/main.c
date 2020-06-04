#include <stdio.h>
#include "glue.c"
#include "../impedance.c"
int main()
{
	double tt, th, sh, d, res;

	if (scanf("%lf %lf %lf %lf", &tt, &th, &sh, &d) != 4) {
		fprintf(stderr, "need: trace_width, trace_height, subst_height, dielect\n");
		return 1;
	}

	res = pcb_impedance_microstrip(RND_MM_TO_COORD(tt), RND_MM_TO_COORD(th), RND_MM_TO_COORD(sh), d);
	printf("res=%f\n", res);
	return 0;
}
