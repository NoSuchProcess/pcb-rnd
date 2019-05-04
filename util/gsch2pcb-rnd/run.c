/* gsch2pcb-rnd
 *
 *  Original version: Bill Wilson    billw@wt.net
 *  rnd-version: (C) 2015..2016, Tibor 'Igor2' Palinkas
 *
 *  This program is free software which I release under the GNU General Public
 *  License. You may redistribute and/or modify this program under the terms
 *  of that license as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.  Version 2 is in the
 *  COPYRIGHT file in the top level directory of this distribution.
 *
 *  To get a copy of the GNU General Puplic License, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "gsch2pcb.h"
#include "../src_3rd/genvector/vts0.h"
#include "../src/compat_misc.h"
#include "../src/safe_fs.h"
#include "gsch2pcb_rnd_conf.h"


int build_and_run_command(const char * format_, ...)
{
	va_list vargs;
	int result = FALSE;
	vts0_t args;
	char *format, *s, *start;

	/* Translate the format string; args elements point to const char *'s
	   within a copy of the format string. The format string is copied so
	   that these parts can be terminated by overwriting whitepsace with \0 */
	va_start(vargs, format_);
	format = pcb_strdup(format_);
	vts0_init(&args);
	for(s = start = format; *s != '\0'; s++) {
		/* if word separator is reached, save the previous word */
		if (isspace(s[0])) {
			if (start == s) { /* empty word - skip */
				start++;
				continue;
			}
			*s = '\0';
			vts0_append(&args, start);
			start = s+1;
			continue;
		}

		/* check if current word is a format */
		if ((s == start) && (s[0] == '%') && (s[1] != '\0') && ((s[2] == '\0') || isspace(s[2]))) {
			switch(s[1]) {
				case 'L': /* append contents of char * gadl_list_t */
					{
						gadl_list_t *list = va_arg(vargs, gadl_list_t *);
						gadl_iterator_t it;
						char **s;
						gadl_foreach(list, &it, s) {
							vts0_append(&args, *s);
						}
					}
					start = s+2;
					s++;
					continue;
				case 's':
					{
						char *arg = va_arg(vargs, char *);
						if (arg != NULL)
							vts0_append(&args, arg);
						start = s+2;
						s++;
					}
					continue;
			}
		}
	}
	va_end(vargs);

	if (args.used > 0) {
		int i, l;
		char *cmd, *end, line[1024];
		FILE *f;

		l = 0;
		for (i = 0; i < args.used; i++)
			l += strlen(args.array[i]) + 3;

		end = cmd = malloc(l+1);
		for (i = 0; i < args.used; i++) {
			l = strlen(args.array[i]);
			*end = '"'; end++;
			memcpy(end, args.array[i], l);
			end += l;
			*end = '"'; end++;
			*end = ' '; end++;
		}
		end--;
		*end = '\0';

		/* we have something in the list, build & call command */
		if (conf_g2pr.utils.gsch2pcb_rnd.verbose) {
			printf("Running command:\n\t%s\n", cmd);
			printf("%s", SEP_STRING);
		}

		f = pcb_popen(NULL, cmd, "r");
		while(fgets(line, sizeof(line), f) != NULL) {
			if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
				fputs(line, stdout);
		}

		if (pcb_pclose(f) == 0)
			result = TRUE;
		else
			fprintf(stderr, "Failed to execute external program\n");
		free(cmd);

		if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
			printf("\n%s", SEP_STRING);
	}

	free(format);
	vts0_uninit(&args);
	return result;
}
