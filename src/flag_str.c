/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2005 DJ Delorie
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
 *
 *
 *  Old contact info:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "flag_str.h"
#include "compat_misc.h"
#include "obj_common.h"
#include "macro.h"

/* Because the common flag access macros expect an object struct to work on. */
typedef struct {
	pcb_flag_t Flags;
} FlagHolder;

/* Be careful to list more specific flags first, followed by general
 * flags, when two flags use the same bit.  For example, "onsolder" is
 * for elements only, while "auto" is for everything else.  They use
 * the same bit, but onsolder is listed first so that elements will
 * use it and not auto.
 *
 * Thermals are handled separately, as they're layer-selective.
 */

#define N(x) x, sizeof(x)-1
#define FN(x) x, #x
pcb_flag_bits_t pcb_object_flagbits[] = {
	{FN(PCB_FLAG_PIN),           N("pin"),           PCB_OBJ_ANY, "If set, this object is a pin.  This flag is for internal use only.", 0, 1},
	{FN(PCB_FLAG_VIA),           N("via"),           PCB_OBJ_ANY, "If set, this object is a via.  This flag is for internal use only.", 0, 1},
	{FN(PCB_FLAG_FOUND),         N("found"),         PCB_OBJ_ANY, "If set, this object has been found by FindConnection()", 0, 0},
	{FN(PCB_FLAG_HOLE),          N("hole"),          PCB_OBJ_CLASS_PIN, "For pins and vias, this flag means that the pin or via is a hole without a copper annulus.", 0, 0},
	{FN(PCB_FLAG_RAT),           N("rat"),           PCB_OBJ_RAT, "If set for a line, indicates that this line is a rat line instead of a copper trace.", 0, 0},
	{FN(PCB_FLAG_PININPOLY),     N("pininpoly"),     0, "For pins and pads, this flag is used internally to indicate that the pin or pad overlaps a polygon on some layer.", 0, 0},
	{FN(PCB_FLAG_CLEARPOLY),     N("clearpoly"),     PCB_OBJ_POLY, "For polygons, this flag means that pins and vias will normally clear these polygons (thus, thermals are required for electrical connection).  When clear, polygons will solidly connect to pins and vias. ", 0, 0},
	{FN(PCB_FLAG_HIDENAME),      N("hidename"),      0, "For elements, when set the name of the element is hidden.", PCB_OBJ_SUBC, 0},
	{FN(PCB_FLAG_DISPLAYNAME),   N("showname"),      0, "OBSOLETE: For elements, when set the names of pins are shown.", 0, 0},
	{FN(PCB_FLAG_CLEARLINE),     N("clearline"),     PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_TEXT | PCB_OBJ_PSTK, "For lines and arcs, the line/arc will clear polygons instead of connecting to them.", 0, 0 },
	{FN(PCB_FLAG_SELECTED),      N("selected"),      PCB_OBJ_ANY, "Set when the object is selected.", 0, 0},
	{FN(PCB_FLAG_ONSOLDER),      N("onsolder"),      PCB_OBJ_TEXT, "For text, indicates that it is on the solder side.", PCB_OBJ_SUBC | PCB_OBJ_PSTK, 0},
	{FN(PCB_FLAG_AUTO),          N("auto"),          PCB_OBJ_ANY, "For lines and vias, indicates that these were created by the autorouter.", 0, 0},
	{FN(PCB_FLAG_SQUARE),        N("square"),        0, "For pins and pads, indicates a square (vs round) pin/pad.", PCB_OBJ_PSTK, 0},
	{FN(PCB_FLAG_RUBBEREND),     N("rubberend"),     PCB_OBJ_LINE | PCB_OBJ_ARC, "For lines, used internally for rubber band moves: indicates one end already rubber banding.", 0, 0},
	{FN(PCB_FLAG_WARN),          N("warn"),          PCB_OBJ_CLASS_PIN, "For pins, vias, and pads, set to indicate a warning.", 0, 0},
	{FN(PCB_FLAG_USETHERMAL),    N("usetherm"),      PCB_OBJ_CLASS_PIN | PCB_OBJ_LINE | PCB_OBJ_ARC, "Obsolete, indicates that pins/vias should be drawn with thermal fingers.", 0, 1},
	{FN(PCB_FLAG_OCTAGON),       N("octagon"),       0, "Draw pins and vias as octagons.", 0, 0},
	{FN(PCB_FLAG_DRC),           N("drc"),           PCB_OBJ_ANY, "Set for objects that fail DRC: flag like FOUND flag for DRC checking.", 0},
	{FN(PCB_FLAG_LOCK),          N("lock"),          PCB_OBJ_ANY, "Set for locked objects.", 0, 0},
	{FN(PCB_FLAG_EDGE2),         N("edge2"),         PCB_OBJ_ANY, "For pads, indicates that the second point is closer to the edge.  For pins, indicates that the pin is closer to a horizontal edge and thus pinout text should be vertical. (Padr.Point2 is closer to outside edge also pinout text for pins is vertical)", 0, 1},
	{FN(PCB_FLAG_FULLPOLY),      N("fullpoly"),      PCB_OBJ_POLY, "For polygons, the full polygon is drawn (i.e. all parts instead of only the biggest one).", 0, 0},
	{FN(PCB_FLAG_NOPASTE),       N("nopaste"),       0, "Pad should not receive solderpaste.  This is to support fiducials", 0, 0},
	{FN(PCB_FLAG_NONETLIST),     N("nonetlist"),     PCB_OBJ_SUBC, "subcircuit is not on the netlist and should not interfere with the netlist ", 0, 0},
	{FN(PCB_FLAG_TERMNAME),      N("termname"),      PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_TEXT | PCB_OBJ_PSTK | PCB_OBJ_SUBC, "when set the names of pins are shown.", 0, 0},
	{FN(PCB_FLAG_CLEARPOLYPOLY), N("clearpolypoly"), PCB_OBJ_POLY, "For polygons, apply clearance to nearby polygons", 0, 0},
	{FN(PCB_FLAG_DYNTEXT),       N("dyntext"),       PCB_OBJ_TEXT, "For text: dynamic string (substitute %patterns%)", 0, 0},
	{FN(PCB_FLAG_FLOATER),       N("floater"),       PCB_OBJ_ANY, "subc part can be moved after subc placing", 0, 0}
};
#undef N
#undef FN

const int pcb_object_flagbits_len = PCB_ENTRIES(pcb_object_flagbits);

/* List of old/obsolete flags to silently ignore on load */
static const char *old_flag_ignore[] = { "connected", NULL };

/*
 * This helper function maintains a small list of buffers which are
 * used by pcb_strflg_f2s().  Each buffer is allocated from the heap,
 * but the caller must not free them (they get realloced when they're
 * reused, but never completely freed).
 */

static struct {
	char *ptr;
	int len;
} buffers[10];
static int bufptr = 0;
static char *alloc_buf(int len)
{
#define B buffers[bufptr]
	len++;
	bufptr = (bufptr + 1) % 10;
	if (B.len < len) {
		if (B.ptr)
			B.ptr = (char *) realloc(B.ptr, len);
		else
			B.ptr = (char *) malloc(len);
		B.len = len;
	}
	return B.ptr;
#undef B
}

void pcb_strflg_uninit_buf(void)
{
	int n;
	for(n = 0; n < 10; n++) {
		if (buffers[n].ptr != NULL) {
			free(buffers[n].ptr);
			buffers[n].ptr = NULL;
		}
	}
}

/*
 * This set of routines manages a list of layer-specific flags.
 * Callers should call grow_layer_list(0) to reset the list, and
 * set_layer_list(layer,1) to set bits in the layer list.  The results
 * are stored in layers[], which has num_layers valid entries.
 */

static char *layers = 0;
static int max_layers = 0, num_layers = 0;

static void grow_layer_list(int num)
{
	if (layers == 0) {
		layers = (char *) calloc(num > 0 ? num : 1, 1);
		max_layers = num;
	}
	else if (num > max_layers) {
		max_layers = num;
		layers = (char *) realloc(layers, max_layers);
	}
	if (num > num_layers)
		memset(layers + num_layers, 0, num - num_layers - 1);
	num_layers = num;
	return;
}

void pcb_strflg_uninit_layerlist(void)
{
	if (layers != NULL) {
		free(layers);
		layers = NULL;
		num_layers = max_layers = 0;
	}
}

static inline void set_layer_list(int layer, int v)
{
	if (layer >= num_layers)
		grow_layer_list(layer + 1);
	layers[layer] = v;
}

/*
 * These next two convert between layer lists and strings.
 * parse_layer_list() is passed a pointer to a string, and parses a
 * list of integer which reflect layers to be flagged.  It returns a
 * pointer to the first character following the list.  The syntax of
 * the list is a paren-surrounded, comma-separated list of integers
 * and/or pairs of integers separated by a dash (like "(1,2,3-7)").
 * Spaces and other punctuation are not allowed.  The results are
 * stored in layers[] defined above.
 *
 * print_layer_list() does the opposite - it uses the flags set in
 * layers[] to build a string that represents them, using the syntax
 * above.
 *
 */

/* Returns a pointer to the first character past the list. */
static const char *parse_layer_list(const char *bp, int (*error) (const char *))
{
	const char *orig_bp = bp;
	int l = 0, range = -1;
	int value = 1;

	grow_layer_list(0);
	while (*bp) {
		if (*bp == '+')
			value = 2;
		else if (*bp == 'S')
			value = 3;
		else if (*bp == 'X')
			value = 4;
		else if (*bp == 't')
			value = 5;
		else if (*bp == ')' || *bp == ',' || *bp == '-') {
			if (range == -1)
				range = l;
			while (range <= l)
				set_layer_list(range++, value);
			if (*bp == '-')
				range = l;
			else
				range = -1;
			value = 1;
			l = 0;
		}

		else if (isdigit((int) *bp))
			l = l * 10 + (*bp - '0');

		else if (error) {
			const char *fmt = "Syntax error parsing layer list \"%.*s\" at %c";
			char *msg = alloc_buf(strlen(fmt) + strlen(orig_bp));
			sprintf(msg, fmt, bp - orig_bp + 5, orig_bp, *bp);
			error(msg);
			error = NULL;
		}

		if (*bp == ')')
			return bp + 1;

		bp++;
	}
	return bp;
}

/* Number of character the value "i" requires when printed. */
static int printed_int_length(int i, int j)
{
	int rv;

	if (i < 10)
		return 1 + (j ? 1 : 0);
	if (i < 100)
		return 2 + (j ? 1 : 0);

	for (rv = 1; i >= 10; rv++)
		i /= 10;
	return rv + (j ? 1 : 0);
}

/* Returns a pointer to an internal buffer which is overwritten with
   each new call.  */
static char *print_layer_list()
{
	static char *buf = 0;
	static int buflen = 0;
	int len, i, j;
	char *bp;

	len = 2;
	for (i = 0; i < num_layers; i++)
		if (layers[i])
			len += 1 + printed_int_length(i, layers[i]);
	if (buflen < len) {
		if (buf)
			buf = (char *) realloc(buf, len);
		else
			buf = (char *) malloc(len);
		buflen = len;
	}

	bp = buf;
	*bp++ = '(';

	for (i = 0; i < num_layers; i++)
		if (layers[i]) {
			/* 0 0 1 1 1 0 0 */
			/*     i     j   */
			for (j = i + 1; j < num_layers && layers[j] == 1; j++);
			if (j > i + 2) {
				sprintf(bp, "%d-%d,", i, j - 1);
				i = j - 1;
			}
			else
				switch (layers[i]) {
				case 1:
					sprintf(bp, "%d,", i);
					break;
				case 2:
					sprintf(bp, "%d+,", i);
					break;
				case 3:
					sprintf(bp, "%dS,", i);
					break;
				case 4:
					sprintf(bp, "%dX,", i);
					break;
				case 5:
				default:
					sprintf(bp, "%dt,", i);
					break;
				}
			bp += strlen(bp);
		}
	bp[-1] = ')';
	*bp = 0;
	return buf;
}

static int error_ignore(const char *msg)
{
	return 0;
}

static pcb_flag_t empty_flags;

pcb_flag_t pcb_strflg_common_s2f(const char *flagstring, int (*error) (const char *msg), pcb_flag_bits_t * flagbits, int n_flagbits, unsigned char *intconn, int compat)
{
	const char *fp, *ep;
	int flen;
	FlagHolder rv;
	int i;

	if (intconn != NULL)
		*intconn = 0;
	rv.Flags = empty_flags;

	if (error == 0)
		error = error_ignore;

	if (flagstring == NULL)
		return empty_flags;

	fp = ep = flagstring;

	if (*fp == '"')
		ep = ++fp;

	while (*ep && *ep != '"') {
		int found = 0;

		while (*fp == ',')
			fp++;

		for (ep = fp; *ep && *ep != ',' && *ep != '"' && *ep != '('; ep++);
		flen = ep - fp;
		if (*ep == '(')
			ep = parse_layer_list(ep + 1, error);

		if (flen == 7 && memcmp(fp, "thermal", 7) == 0) {
			for (i = 0; i < PCB_MAX_LAYER && i < num_layers; i++)
				if (layers[i])
					PCB_FLAG_THERM_ASSIGN(i, layers[i], &rv);
		}
		else if (flen == 5 && memcmp(fp, "shape", 5) == 0) {
			rv.Flags.q = atoi(fp + 6);
		}
		else if (intconn != NULL && flen == 7 && memcmp(fp, "intconn", 7) == 0) {
			*intconn = atoi(fp + 8);
		}
		else {
			for (i = 0; i < n_flagbits; i++)
				if (flagbits[i].nlen == flen && memcmp(flagbits[i].name, fp, flen) == 0) {
					found = 1;
					PCB_FLAG_SET(flagbits[i].mask, &rv);
					break;
				}
			if (!found) {
				const char **ign;
				for(ign = old_flag_ignore; *ign != NULL; ign++) {
					if (memcmp(*ign, fp, flen) == 0) {
						found = 1;
						break;
					}
				}
			}
			if (!found) {
				const char *fmt = "Unknown flag: \"%.*s\" ignored";
				pcb_unknown_flag_t *u;
				char *msg;
				const char *s;

				/* include () */
				s = fp + flen;
				if (*s == '(') {
					while (*s != ')') {
						flen++;
						s++;
					}
				}
				if (*s == ')')
					flen++;

				msg = alloc_buf(strlen(fmt) + flen);
				sprintf(msg, fmt, flen, fp);
				error(msg);

				u = malloc(sizeof(pcb_unknown_flag_t));
				u->str = pcb_strndup(fp, flen);
				u->next = NULL;
				/* need to append, to keep order of flags */
				if (rv.Flags.unknowns != NULL) {
					pcb_unknown_flag_t *n;
					for (n = rv.Flags.unknowns; n->next != NULL; n = n->next);
					n->next = u;
				}
				else
					rv.Flags.unknowns = u;
			}
		}
		fp = ep + 1;
	}
	return rv.Flags;
}

pcb_flag_t pcb_strflg_s2f(const char *flagstring, int (*error) (const char *msg), unsigned char *intconn, int compat)
{
	return pcb_strflg_common_s2f(flagstring, error, pcb_object_flagbits, PCB_ENTRIES(pcb_object_flagbits), intconn, compat);
}

char *pcb_strflg_common_f2s(pcb_flag_t flags, int object_type, pcb_flag_bits_t * flagbits, int n_flagbits, unsigned char *intconn, int compat)
{
	int len;
	int i;
	FlagHolder fh, savef;
	char *buf, *bp;
	pcb_unknown_flag_t *u;

	fh.Flags = flags;

#ifndef FLAG_TEST
	switch (object_type) {
	case PCB_OBJ_RAT:
		PCB_FLAG_CLEAR(PCB_FLAG_RAT, &fh);
		break;
	}
#endif

	savef = fh;

	len = 3;											/* for "()\0" */

	for (i = 0; i < n_flagbits; i++) {
		long my_obj_typs = flagbits[i].object_types | (compat ? flagbits[i].compat_types : 0);
		if ((my_obj_typs & object_type) && (PCB_FLAG_TEST(flagbits[i].mask, &fh))) {
			len += flagbits[i].nlen + 1;
			PCB_FLAG_CLEAR(flagbits[i].mask, &fh);
		}
	}

	if (PCB_FLAG_THERM_TEST_ANY(&fh)) {
		len += sizeof("thermal()");
		for (i = 0; i < PCB_MAX_LAYER; i++)
			if (PCB_FLAG_THERM_TEST(i, &fh))
				len += printed_int_length(i, PCB_FLAG_THERM_GET(i, &fh)) + 1;
	}

	if (flags.q > 0) {
		len += sizeof("shape(.)");
		if (flags.q > 9)
			len += 2;
	}

	if ((intconn != NULL) && (*intconn > 0)) {
		len += sizeof("intconn(.)");
		if (*intconn > 9)
			len++;
		if (*intconn > 99)
			len++;
	}

	for (u = flags.unknowns; u != NULL; u = u->next)
		len += strlen(u->str) + 1;

	bp = buf = alloc_buf(len + 2);

	*bp++ = '"';

	fh = savef;
	for (i = 0; i < n_flagbits; i++) {
		long my_obj_typs = flagbits[i].object_types | (compat ? flagbits[i].compat_types : 0);
		if ((my_obj_typs & object_type) && (PCB_FLAG_TEST(flagbits[i].mask, &fh))) {
			if (bp != buf + 1)
				*bp++ = ',';
			strcpy(bp, flagbits[i].name);
			bp += flagbits[i].nlen;
			PCB_FLAG_CLEAR(flagbits[i].mask, &fh);
		}
	}

	if (PCB_FLAG_THERM_TEST_ANY(&fh)) {
		if (bp != buf + 1)
			*bp++ = ',';
		strcpy(bp, "thermal");
		bp += strlen("thermal");
		grow_layer_list(0);
		for (i = 0; i < PCB_MAX_LAYER; i++)
			if (PCB_FLAG_THERM_TEST(i, &fh))
				set_layer_list(i, PCB_FLAG_THERM_GET(i, &fh));
		strcpy(bp, print_layer_list());
		bp += strlen(bp);
	}

	if (flags.q > 0) {
		if (bp != buf + 1)
			*bp++ = ',';
		bp += sprintf(bp, "shape(%d)", flags.q);
	}

	if ((intconn != NULL) && (*intconn > 0)) {
		if (bp != buf + 1)
			*bp++ = ',';
		bp += sprintf(bp, "intconn(%d)", *intconn);
	}

	for (u = flags.unknowns; u != NULL; u = u->next) {
		int len;
		len = strlen(u->str);
		if (bp != buf + 1)
			*bp++ = ',';
		memcpy(bp, u->str, len);
		bp += len;
	}

	*bp++ = '"';
	*bp = 0;
	return buf;
}

char *pcb_strflg_f2s(pcb_flag_t flags, int object_type, unsigned char *intconn, int compat)
{
	return pcb_strflg_common_f2s(flags, object_type, pcb_object_flagbits, PCB_ENTRIES(pcb_object_flagbits), intconn, compat);
}

const pcb_flag_bits_t *pcb_strflg_1bit(unsigned long bit, int object_type)
{
	int i, n_flagbits = PCB_ENTRIES(pcb_object_flagbits);;
	for (i = 0; i < n_flagbits; i++) {
		unsigned long my_obj_typs = pcb_object_flagbits[i].object_types;
		if ((my_obj_typs & object_type) && (!pcb_object_flagbits[i].omit_list) && (pcb_object_flagbits[i].mask & bit))
			return &pcb_object_flagbits[i];
	}
	return NULL;
}

const pcb_flag_bits_t *pcb_strflg_name(const char *name, int object_type)
{
	int i, n_flagbits = PCB_ENTRIES(pcb_object_flagbits);;
	for (i = 0; i < n_flagbits; i++) {
		unsigned long my_obj_typs = pcb_object_flagbits[i].object_types;
		if ((my_obj_typs & object_type) && (!pcb_object_flagbits[i].omit_list) && (strcmp(pcb_object_flagbits[i].name, name) == 0))
			return &pcb_object_flagbits[i];
	}
	return NULL;
}

void pcb_strflg_map(unsigned long fbits, int object_type, void *ctx, void (*cb)(void *ctx, unsigned long flg, const pcb_flag_bits_t *fb))
{
	int n, i, n_flagbits = PCB_ENTRIES(pcb_object_flagbits);
	for(n = 0; n < 32; n++) {
		unsigned long bit = 1 << n;
		if (!(fbits & bit))
			continue;

		for (i = 0; i < n_flagbits; i++) {
			unsigned long my_obj_typs = pcb_object_flagbits[i].object_types /*| pcb_object_flagbits[i].compat_types*/;
			if ((my_obj_typs & object_type) && (!pcb_object_flagbits[i].omit_list) && (pcb_object_flagbits[i].mask & bit)) {
				cb(ctx, bit, &pcb_object_flagbits[i]);
			}
		}
	}
}


#define N(x) x, sizeof(x)-1
#define FN(x) x, #x
static pcb_flag_bits_t pcb_flagbits[] = {
	{FN(PCB_SHOWNUMBERFLAG),        N("shownumber"),   1, "Pinout displays pin numbers instead of pin names." },
	{FN(PCB_LOCALREFFLAG),          N("localref"),     1, "Use local reference for moves, by setting the mark at the beginning of each move." },
	{FN(PCB_CHECKPLANESFLAG),       N("checkplanes"),  1, "When set, only polygons and their clearances are drawn, to see if polygons have isolated regions." },
	{FN(PCB_SHOWPCB_FLAG_DRC),      N("showdrc"),      1, "Display DRC region on crosshair." },
	{FN(PCB_RUBBERBANDFLAG),        N("rubberband"),   1, "Do all move, mirror, rotate with rubberband connections." },
	{FN(PCB_DESCRIPTIONFLAG),       N("description"),  1, "Display descriptions of elements, instead of refdes." },
	{FN(PCB_NAMEONPCBFLAG),         N("nameonpcb"),    1, "Display names of elements, instead of refdes." },
	{FN(PCB_AUTOPCB_FLAG_DRC),      N("autodrc"),      1, "Auto-DRC flag.  When set, PCB doesn't let you place copper that violates DRC." },
	{FN(PCB_ALLDIRECTIONFLAG),      N("alldirection"), 1, "Enable 'all-direction' lines." },
	{FN(PCB_SWAPSTARTDIRFLAG),      N("swapstartdir"), 1, "Switch starting angle after each click." },
	{FN(PCB_UNIQUENAMEFLAG),        N("uniquename"),   1, "Force unique names on board." },
	{FN(PCB_CLEARNEWFLAG),          N("clearnew"),     1, "New lines/arc clear polygons." },
	{FN(PCB_SNAPPCB_FLAG_PIN),      N("snappin"),      1, "pcb_crosshair snaps to pins and pads." },
	{FN(PCB_SHOWMASKFLAG),          N("showmask"),     1, "Show the solder mask layer." },
	{FN(PCB_THINDRAWFLAG),          N("thindraw"),     1, "Draw with thin lines." },
	{FN(PCB_ORTHOMOVEFLAG),         N("orthomove"),    1, "Move items orthogonally." },
	{FN(PCB_LIVEROUTEFLAG),         N("liveroute"),    1, "Draw autoroute paths real-time." },
	{FN(PCB_NEWPCB_FLAG_FULLPOLY),  N("newfullpoly"),  1, "New polygons are full ones." },
	{FN(PCB_THINDRAWPOLYFLAG),      N("thindrawpoly"), 1, "Thin draw polygon contours instead of filling polygons." },
	{FN(PCB_LOCKNAMESFLAG),         N("locknames"),    1, "Names are locked, the mouse cannot select them." },
	{FN(PCB_ONLYNAMESFLAG),         N("onlynames"),    1, "Everything but names are locked, the mouse cannot select anything else." },
	{FN(PCB_HIDENAMESFLAG),         N("hidenames"),    1, "When set, element names are not drawn." },
	{FN(PCB_ENABLEPCB_FLAG_MINCUT), N("enablemincut"), 1, "Use the mincut algorithm for finding shorts." },
};
#undef N
#undef FN

char *pcb_strflg_board_f2s(pcb_flag_t flags)
{
	return pcb_strflg_common_f2s(flags, PCB_OBJ_ANY, pcb_flagbits, PCB_ENTRIES(pcb_flagbits), NULL, 0);
}

pcb_flag_t pcb_strflg_board_s2f(const char *flagstring, int (*error) (const char *msg))
{
	return pcb_strflg_common_s2f(flagstring, error, pcb_flagbits, PCB_ENTRIES(pcb_flagbits), NULL, 0);
}
