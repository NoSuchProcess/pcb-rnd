#include <stdio.h>
#include "misc_util.h"
#include "pcb-printf.h"
#include "pcb_bool.h"

int main(int argc, char *argv[])
{
	const char *fmt = argv[1];
	Coord crd;
	int n;

	pcb_printf_slot[0] = "%mr";

	for(n = 2; n < argc; n++) {
		pcb_bool success;
		double val = pcb_get_value_ex(argv[n], NULL, NULL, NULL, "", &success);
		if (!success) {
			fprintf(stderr, "Unable to convert '%s' to coord\n", argv[n]);
			return 1;
		}
		crd = val;
	}

	pcb_fprintf(stdout, fmt, crd, 70000, 70000, 70000, 70000);

	printf("\n");

	return 0;
}