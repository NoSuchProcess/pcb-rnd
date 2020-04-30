#include <stdio.h>
#include <locale.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/rnd_bool.h>

int main(int argc, char *argv[])
{
	const char *fmt = argv[1];
	rnd_coord_t crd;
	int n;

	setlocale(LC_ALL, "C");

	rnd_printf_slot[0] = "%mr";

	for(n = 2; n < argc; n++) {
		rnd_bool success;
		double val = rnd_get_value_ex(argv[n], NULL, NULL, NULL, "", &success);
		if (!success) {
			fprintf(stderr, "Unable to convert '%s' to rnd_coord_t\n", argv[n]);
			return 1;
		}
		crd = val;
	}

	rnd_fprintf(stdout, fmt, crd, 70000, 70000, 70000, 70000);

	printf("\n");

	return 0;
}
