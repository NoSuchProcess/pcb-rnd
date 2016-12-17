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

/* The gnetlist backend gnet-gsch2pcb-rnd.scm generates PKG lines:
 *
 *        PKG(footprint,refdes,value)
 *
 */

#include <stdio.h>
#include <string.h>
#include "gsch2pcb.h"
#include "gsch2pcb_rnd_conf.h"
#include "../src/compat_misc.h"

char *token(char * string, char ** next, int * quoted_ret, int parenth)
{
	static char *str;
	char *s, *ret;
	int quoted = FALSE;

	if (string)
		str = string;
	if (!str || !*str) {
		if (next)
			*next = str;
		return pcb_strdup("");
	}
	while (*str == ' ' || *str == '\t' || *str == ',' || *str == '\n')
		++str;

	if (*str == '"') {
		quoted = TRUE;
		if (quoted_ret)
			*quoted_ret = TRUE;
		++str;
		for (s = str; *s && *s != '"' && *s != '\n'; ++s);
	}
	else {
		if (quoted_ret)
			*quoted_ret = FALSE;
		for (s = str; *s != '\0'; ++s) {
			if ((parenth) && (*s == '(')) {
				quoted = TRUE;
				if (quoted_ret)
					*quoted_ret = TRUE;
				for (; *s && *s != ')' && *s != '\n'; ++s);
				/* preserve closing ')' */
				if (*s == ')')
					s++;
				break;
			}
			if (*s == ' ' || *s == '\t' || *s == ',' || *s == '\n')
				break;
		}
	}
	ret = pcb_strndup(str, s - str);
	str = (quoted && *s) ? s + 1 : s;
	if (next)
		*next = str;
	return ret;
}

PcbElement *pkg_to_element(FILE * f, char * pkg_line)
{
	PcbElement *el;
	char *s, *end, *refdes, *fp, *value;

/*fprintf(stderr, "--- %s\n", pkg_line);*/

	if (strncmp(pkg_line, "PKG", 3)
			|| (s = strchr(pkg_line, (int) '(')) == NULL)
		return NULL;

/* remove trailing ")" */
	end = s + strlen(s) - 2;
	if (*end == ')')
		*end = '\0';

/* tokenize the line keeping () */
	fp = token(s + 1, NULL, NULL, 1);
	refdes = token(NULL, NULL, NULL, 1);
	value = token(NULL, NULL, NULL, 1);


/*fprintf(stderr, "refdes: %s\n", refdes);
fprintf(stderr, "    fp: %s\n", fp);
fprintf(stderr, "   val: %s\n", value);*/


	if (!refdes || !fp || !value) {
		if (refdes != NULL)
			free(refdes);
		if (fp != NULL)
			free(fp);
		if (value != NULL)
			free(value);
		fprintf(stderr, "Bad package line: %s\n", pkg_line);
		return NULL;
	}

	fix_spaces(refdes);
	fix_spaces(value);

	el = calloc(sizeof(PcbElement), 1);
	el->description = fp;
	el->refdes = refdes;
	el->value = value;

/*
// wtf?
//  if ((s = strchr (el->value, (int) ')')) != NULL)
//    *s = '\0';
*/

	if (conf_g2pr.utils.gsch2pcb_rnd.empty_footprint_name && !strcmp(el->description, conf_g2pr.utils.gsch2pcb_rnd.empty_footprint_name)) {
		if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
			printf("%s: has the empty footprint attribute \"%s\" so won't be in the layout.\n", el->refdes, el->description);
		n_empty += 1;
		el->omit_PKG = TRUE;
	}
	else if (!strcmp(el->description, "none")) {
		fprintf(stderr, "WARNING: %s has a footprint attribute \"%s\" so won't be in the layout.\n", el->refdes, el->description);
		n_none += 1;
		el->omit_PKG = TRUE;
	}
	else if (!strcmp(el->description, "unknown")) {
		fprintf(stderr, "WARNING: %s has no footprint attribute so won't be in the layout.\n", el->refdes);
		n_unknown += 1;
		el->omit_PKG = TRUE;
	}
	return el;
}
