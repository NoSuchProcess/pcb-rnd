/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "font_geo.c"

typedef enum {
	FSR_SUCCESS = 0,
	FSR_INVALID_FONT_ID, /* fatal, no output generated: print error and abort parsing */
	FSR_INVALID_GLYPH_ID /* non-fatal but sizes may be wrong: print error but do not abort */
} font_size_res_t;

/* Which glyph code to use for a glyph that is not in our arrays */
#define UNKNOWN_GLYPH 32

#define LINE_END \
do { \
	if (lw > tw) \
		tw = lw; \
	lw = 0; \
	lines++; \
} while(0)

/* Calculate the nominal size of a potentially multi-line text object. Output:
   - width is the width of the longest line
   - height is the total net height from bottom to top (depends on number of lines)
   - yoffs is the vertical offset because of the baseline (does not depend on number of lines)
*/
static font_size_res_t font_text_nominal_size(int font_id, const char *txt, rnd_coord_t *width, rnd_coord_t *height, rnd_coord_t *yoffs)
{
	const char *s;
	double lw = 0; /* current line width */
	double tw = 0; /* total width */
	double *gw;
	int lines = 0, bad_glyph = 0;

	if (!PCB_MENTOR_FONT_ID_VALID(font_id))
		return FSR_INVALID_FONT_ID;

	gw = pcb_mentor_font_widths[font_id];
	for(s = txt; *s != '\0'; s++) {
		int code = *s;
		if (*s == '\n') {
			LINE_END;
			continue;
		}
		if (!PCB_MENTOR_FONT_GLYPH_VALID(code)) {
			bad_glyph = 1;
			if (PCB_MENTOR_FONT_GLYPH_VALID(UNKNOWN_GLYPH))
				lw += gw[UNKNOWN_GLYPH];
		}
		else
			lw += gw[code];
	}
	LINE_END;

	*width = RND_MM_TO_COORD(tw);
	*height = RND_MM_TO_COORD((double)lines * (pcb_mentor_font_ymax[font_id] - pcb_mentor_font_ymin[font_id]));
	*yoffs = RND_MM_TO_COORD(pcb_mentor_font_ymin[font_id]);

	return bad_glyph ? FSR_INVALID_GLYPH_ID : FSR_SUCCESS;
}


#undef UNKNOWN_GLYPH
#undef LINE_END
