#include <stdio.h>
#include <locale.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/pcb_bool.h>

int main(int argc, char *argv[])
{
	const char *fmt = argv[1];
	pcb_coord_t crd;
	int n;

	setlocale(LC_ALL, "C");

	pcb_printf_slot[0] = "%mr";

	for(n = 2; n < argc; n++) {
		pcb_bool success;
		double val = pcb_get_value_ex(argv[n], NULL, NULL, NULL, "", &success);
		if (!success) {
			fprintf(stderr, "Unable to convert '%s' to pcb_coord_t\n", argv[n]);
			return 1;
		}
		crd = val;
	}

	pcb_fprintf(stdout, fmt, crd, 70000, 70000, 70000, 70000);

	printf("\n");

	return 0;
}
