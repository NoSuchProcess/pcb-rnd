#include <time.h>
#include <locale.h>
#include "config.h"
#include "global.h"
#include "pcb-printf.h"

#ifdef SPEED
	char buff[8192];
	int pc = 0;
#	define NUMREP 4000
#	define PCB_PRINTF(fmt, ...) \
	do { \
		pc++; \
		pcb_snprintf(buff, sizeof(buff), fmt, __VA_ARGS__); \
	} while(0)
#else
#	define NUMREP 1
#	define PCB_PRINTF pcb_printf
#endif

int main()
{
	Coord c[] = {0, 1, 1024, 1024*1024, 1024*1024*1024};
	char *fmt[] = {
	"%mI",  "%mm",  "%mM",  "%ml",  "%mL",  "%ms",  "%mS",  "%md",  "%mD",  "%m3",  "%mr",
	"%$mI", "%$mm", "%$mM", "%$ml", "%$mL", "%$ms", "%$mS", "%$md", "%$mD", "%$m3", "%$mr",
	NULL };
	char **f;
	int n, rep;

	setlocale(LC_ALL, "");

	for(rep = 0; rep < NUMREP; rep++) {
		for(n = 0; n < sizeof(c) / sizeof(c[0]); n++) {
#ifndef SPEED
			printf("---------------\n");
#endif
			/* common format strings */
			for(f = fmt; *f != NULL; f++) {
				char fmt_tmp[32];
				sprintf(fmt_tmp, "%s=%s\n", (*f)+1, *f);
				PCB_PRINTF(fmt_tmp, c[n], c[n], c[n]);
			}

			/* special: custom argument list */
			PCB_PRINTF("m*=%m* mil\n", "mil", c[n]);
			PCB_PRINTF("m*=%m* mm\n", "mm", c[n]);

			PCB_PRINTF("ma=%ma\n", 1.1234);
			PCB_PRINTF("ma=%ma\n", 130.1234);
			PCB_PRINTF("ma=%ma\n", 555.1234);

			PCB_PRINTF("m+=%m+%mS mil\n", PCB_UNIT_ALLOW_MIL, c[n]);
			PCB_PRINTF("m+=%m+%mS mm\n", PCB_UNIT_ALLOW_MM, c[n]);
			PCB_PRINTF("m+=%m+%mS m\n", PCB_UNIT_ALLOW_M, c[n]);
			PCB_PRINTF("m+=%m+%mS mm or m\n", PCB_UNIT_ALLOW_MM | PCB_UNIT_ALLOW_M, c[n]);
		}
	}

#ifdef SPEED
	{
		double spent = (double)clock() / (double)CLOCKS_PER_SEC;
		printf("total number of pcb_printf calls:  %d\n", pc);
		printf("total number of CPU seconds spent: %.4f\n", spent);
		printf("Average prints per second: %.4f\n", (double)pc/spent);
	}
#endif
	return 0;
}


