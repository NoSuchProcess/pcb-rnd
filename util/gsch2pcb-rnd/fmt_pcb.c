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

#include <stdio.h>
#include <string.h>
#include "gsch2pcb.h"
#include "fmt_pkg.h"

	/* As of 1/9/2004 CVS hi_res Element[] line format:
	 *   Element[element_flags, description, pcb-name, value, mark_x, mark_y,
	 *       text_x, text_y, text_direction, text_scale, text_flags]
	 *   New PCB 1.7 / 1.99 Element() line format:
	 *   Element(element_flags, description, pcb-name, value, mark_x, mark_y,
	 *       text_x, text_y, text_direction, text_scale, text_flags)
	 *   Old PCB 1.6 Element() line format:
	 *   Element(element_flags, description, pcb-name, value,
	 *       text_x, text_y, text_direction, text_scale, text_flags)
	 *
	 *   (mark_x, mark_y) is the element position (mark) and (text_x,text_y)
	 *   is the description text position which is absolute in pre 1.7 and
	 *   is now relative.  The hi_res mark_x,mark_y and text_x,text_y resolutions
	 *   are 100x the other formats.
	 */
PcbElement *pcb_element_line_parse(char * line)
{
	PcbElement *el = NULL;
	char *s, *t, close_char;
	int state = 0, elcount = 0, tmp;

	if (strncmp(line, "Element", 7))
		return NULL;

	el = calloc(sizeof(PcbElement), 1);

	s = line + 7;
	while (*s == ' ' || *s == '\t')
		++s;

	if (*s == '[')
		el->hi_res_format = TRUE;
	else if (*s != '(') {
		free(el);
		return NULL;
	}

	el->res_char = el->hi_res_format ? '[' : '(';
	close_char = el->hi_res_format ? ']' : ')';

	el->flags = token(s + 1, NULL, &tmp, 0); el->quoted_flags = tmp;
	el->description = token(NULL, NULL, NULL, 0);
	el->refdes = token(NULL, NULL, NULL, 0);
	el->value = token(NULL, NULL, NULL, 0);

	el->x = token(NULL, NULL, NULL, 0);
	el->y = token(NULL, &t, NULL, 0);

	el->tail = pcb_strdup(t ? t : "");
	if ((s = strrchr(el->tail, (int) '\n')) != NULL)
		*s = '\0';

	/* Count the tokens in tail to decide if it's new or old format.
	 * Old format will have 3 tokens, new format will have 5 tokens.
	 */
	for (s = el->tail; *s && *s != close_char; ++s) {
		if (*s != ' ') {
			if (state == 0)
				++elcount;
			state = 1;
		}
		else
			state = 0;
	}
	el->nonetlist = 0;
	if (elcount > 4) {
		el->new_format = TRUE;
		if (strstr(el->tail, "nonetlist") != NULL)
			el->nonetlist = 1;
	}

	fix_spaces(el->description);
	fix_spaces(el->refdes);
	fix_spaces(el->value);

	/* Don't allow elements with no refdes to ever be deleted because
	 * they may be desired pc board elements not in schematics.  So
	 * initialize still_exists to TRUE if empty or non-alphanumeric
	 * refdes.
	 */
	if (!*el->refdes || !isalnum((int) (*el->refdes)))
		el->still_exists = TRUE;

	return el;
}


void get_pcb_element_list(char * pcb_file)
{
	FILE *f;
	PcbElement *el;
	char *s, buf[1024];

	if ((f = fopen(pcb_file, "r")) == NULL)
		return;
	while ((fgets(buf, sizeof(buf), f)) != NULL) {
		for (s = buf; *s == ' ' || *s == '\t'; ++s);
		if (!strncmp(s, "PKG_", 4)) {
			need_PKG_purge = TRUE;
			continue;
		}
		if ((el = pcb_element_line_parse(s)) == NULL)
			continue;
		gdl_append(&pcb_element_list, el, all_elems);
	}
	fclose(f);
}

