/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "const.h"
#include "flag_str.h"
#include "compat_misc.h"
#include "macro.h"

/* Because all the macros expect it, that's why.  */
typedef struct {
	FlagType Flags;
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
FlagBitsType pcb_object_flagbits[] = {
	{PCB_FLAG_PIN, N("pin"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_VIA, N("via"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_FOUND, N("found"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_HOLE, N("hole"), PCB_TYPEMASK_PIN},
	{PCB_FLAG_RAT, N("rat"), PCB_TYPE_RATLINE},
	{PCB_FLAG_PININPOLY, N("pininpoly"), PCB_TYPEMASK_PIN | PCB_TYPE_PAD},
	{PCB_FLAG_CLEARPOLY, N("clearpoly"), PCB_TYPE_POLYGON},
	{PCB_FLAG_HIDENAME, N("hidename"), PCB_TYPE_ELEMENT},
	{PCB_FLAG_DISPLAYNAME, N("showname"), PCB_TYPE_ELEMENT},
	{PCB_FLAG_CLEARLINE, N("clearline"), PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_TEXT},
	{PCB_FLAG_SELECTED, N("selected"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_ONSOLDER, N("onsolder"), PCB_TYPE_ELEMENT | PCB_TYPE_PAD | PCB_TYPE_TEXT},
	{PCB_FLAG_AUTO, N("auto"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_SQUARE, N("square"), PCB_TYPEMASK_PIN | PCB_TYPE_PAD},
	{PCB_FLAG_RUBBEREND, N("rubberend"), PCB_TYPE_LINE | PCB_TYPE_ARC},
	{PCB_FLAG_WARN, N("warn"), PCB_TYPEMASK_PIN | PCB_TYPE_PAD},
	{PCB_FLAG_USETHERMAL, N("usetherm"), PCB_TYPEMASK_PIN | PCB_TYPE_LINE | PCB_TYPE_ARC},
	{PCB_FLAG_OCTAGON, N("octagon"), PCB_TYPEMASK_PIN | PCB_TYPE_PAD},
	{PCB_FLAG_DRC, N("drc"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_LOCK, N("lock"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_EDGE2, N("edge2"), PCB_TYPEMASK_ALL},
	{PCB_FLAG_FULLPOLY, N("fullpoly"), PCB_TYPE_POLYGON},
	{PCB_FLAG_NOPASTE, N("nopaste"), PCB_TYPE_PAD},
	{PCB_FLAG_NONETLIST, N("nonetlist"), PCB_TYPEMASK_ALL}
};
#undef N

const int pcb_object_flagbits_len = ENTRIES(pcb_object_flagbits);


/*
 * This helper function maintains a small list of buffers which are
 * used by flags_to_string().  Each buffer is allocated from the heap,
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

void uninit_strflags_buf(void)
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

void uninit_strflags_layerlist(void)
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

/*
 * Ok, now the two entry points to this file.  The first, string_to_flags,
 * is passed a string (usually from parse_y.y) and returns a "set of flags".
 * In theory, this can be anything, but for now it's just an integer.  Later
 * it might be a structure, for example.
 *
 * Currently, there is no error handling :-P
 */

static int error_ignore(const char *msg)
{																/* do nothing */
	return 0;
}

static FlagType empty_flags;

FlagType
common_string_to_flags(const char *flagstring, int (*error) (const char *msg), FlagBitsType * flagbits, int n_flagbits)
{
	const char *fp, *ep;
	int flen;
	FlagHolder rv;
	int i;

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

		for (ep = fp; *ep && *ep != ',' && *ep != '"' && *ep != '('; ep++);
		flen = ep - fp;
		if (*ep == '(')
			ep = parse_layer_list(ep + 1, error);

		if (flen == 7 && memcmp(fp, "thermal", 7) == 0) {
			for (i = 0; i < MAX_LAYER && i < num_layers; i++)
				if (layers[i])
					ASSIGN_THERM(i, layers[i], &rv);
		}
		else if (flen == 5 && memcmp(fp, "shape", 5) == 0) {
			rv.Flags.q = atoi(fp + 6);
		}
		else if (flen == 7 && memcmp(fp, "intconn", 7) == 0) {
			rv.Flags.int_conn_grp = atoi(fp + 8);
		}
		else {
			for (i = 0; i < n_flagbits; i++)
				if (flagbits[i].nlen == flen && memcmp(flagbits[i].name, fp, flen) == 0) {
					found = 1;
					SET_FLAG(flagbits[i].mask, &rv);
					break;
				}
			if (!found) {
				const char *fmt = "Unknown flag: \"%.*s\" ignored";
				unknown_flag_t *u;
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

				u = malloc(sizeof(unknown_flag_t));
				u->str = pcb_strndup(fp, flen);
				u->next = NULL;
				/* need to append, to keep order of flags */
				if (rv.Flags.unknowns != NULL) {
					unknown_flag_t *n;
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

FlagType string_to_flags(const char *flagstring, int (*error) (const char *msg))
{
	return common_string_to_flags(flagstring, error, pcb_object_flagbits, ENTRIES(pcb_object_flagbits));
}


/*
 * Given a set of flags for a given type of object, return a string
 * which reflects those flags.  The only requirement is that this
 * string be parseable by string_to_flags.
 *
 * Note that this function knows a little about what kinds of flags
 * will be automatically set by parsing, so it won't (for example)
 * include the "via" flag for PCB_TYPE_VIAs because it knows those get
 * forcibly set when vias are parsed.
 */

char *common_flags_to_string(FlagType flags, int object_type, FlagBitsType * flagbits, int n_flagbits)
{
	int len;
	int i;
	FlagHolder fh, savef;
	char *buf, *bp;
	unknown_flag_t *u;

	fh.Flags = flags;

#ifndef FLAG_TEST
	switch (object_type) {
	case PCB_TYPE_VIA:
		CLEAR_FLAG(PCB_FLAG_VIA, &fh);
		break;
	case PCB_TYPE_RATLINE:
		CLEAR_FLAG(PCB_FLAG_RAT, &fh);
		break;
	case PCB_TYPE_PIN:
		CLEAR_FLAG(PCB_FLAG_PIN, &fh);
		break;
	}
#endif

	savef = fh;

	len = 3;											/* for "()\0" */

	for (i = 0; i < n_flagbits; i++)

		if ((flagbits[i].object_types & object_type)
				&& (TEST_FLAG(flagbits[i].mask, &fh))) {

			len += flagbits[i].nlen + 1;
			CLEAR_FLAG(flagbits[i].mask, &fh);
		}

	if (TEST_ANY_THERMS(&fh)) {
		len += sizeof("thermal()");
		for (i = 0; i < MAX_LAYER; i++)
			if (TEST_THERM(i, &fh))
				len += printed_int_length(i, GET_THERM(i, &fh)) + 1;
	}

	if (flags.q > 0) {
		len += sizeof("shape(.)");
		if (flags.q > 9)
			len += 2;
	}

	if (flags.int_conn_grp > 0) {
		len += sizeof("intconn(.)");
		if (flags.q > 9)
			len++;
		if (flags.q > 99)
			len++;
	}

	for (u = flags.unknowns; u != NULL; u = u->next)
		len += strlen(u->str) + 1;

	bp = buf = alloc_buf(len + 2);

	*bp++ = '"';

	fh = savef;
	for (i = 0; i < n_flagbits; i++)
		if (flagbits[i].object_types & object_type && (TEST_FLAG(flagbits[i].mask, &fh))) {
			if (bp != buf + 1)
				*bp++ = ',';
			strcpy(bp, flagbits[i].name);
			bp += flagbits[i].nlen;
			CLEAR_FLAG(flagbits[i].mask, &fh);
		}

	if (TEST_ANY_THERMS(&fh)) {
		if (bp != buf + 1)
			*bp++ = ',';
		strcpy(bp, "thermal");
		bp += strlen("thermal");
		grow_layer_list(0);
		for (i = 0; i < MAX_LAYER; i++)
			if (TEST_THERM(i, &fh))
				set_layer_list(i, GET_THERM(i, &fh));
		strcpy(bp, print_layer_list());
		bp += strlen(bp);
	}

	if (flags.q > 0) {
		if (bp != buf + 1)
			*bp++ = ',';
		bp += sprintf(bp, "shape(%d)", flags.q);
	}

	if (flags.int_conn_grp > 0) {
		if (bp != buf + 1)
			*bp++ = ',';
		bp += sprintf(bp, "intconn(%d)", flags.int_conn_grp);
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

char *flags_to_string(FlagType flags, int object_type)
{
	return common_flags_to_string(flags, object_type, pcb_object_flagbits, ENTRIES(pcb_object_flagbits));
}
