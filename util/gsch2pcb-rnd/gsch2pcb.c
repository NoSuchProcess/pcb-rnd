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

 * Retrieved from the official git (2015.07.15)

 Behavior different from the original:
  - use getenv() instead of g_getenv(): on windows this won't do recursive variable expansion
  - use rnd-specific .scm
  - use popen() instead of glib's spawn (stderr is always printed to stderr)
 */
#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/plug_footprint.h"
#include "../src/paths.h"
#include "../src/conf.h"
#include "../src/conf_core.h"
#include "../src_3rd/genvector/vts0.h"
#include "../src_3rd/genlist/gendlist.h"
#include "../src_3rd/genlist/genadlist.h"
#include "../src_3rd/qparse/qparse.h"
#include "../config.h"
#include "../src/plugins.h"
#include "../src/plug_footprint.h"
#include "help.h"
#include "gsch2pcb_rnd_conf.h"

#define TRUE 1
#define FALSE 0

#define GSCH2PCB_RND_VERSION "1.0.1"

#define DEFAULT_PCB_INC "pcb.inc"

#define SEP_STRING "--------\n"

/* from scconfig str lib: */
char *str_concat(const char *sep, ...);

typedef struct {
	char *refdes, *value, *description, *changed_description, *changed_value;
	char *flags, *tail;
	char *x, *y;
	char *pkg_name_fix;
	char res_char;

	gdl_elem_t all_elems;

	unsigned char still_exists:1;
	unsigned char new_format:1;
	unsigned char hi_res_format:1;
	unsigned char quoted_flags:1;
	unsigned char omit_PKG:1;
	unsigned char nonetlist:1;

} PcbElement;

typedef struct {
	char *part_number, *element_name;
} ElementMap;

gdl_list_t pcb_element_list; /* initialized to 0 */
gadl_list_t schematics, extra_gnetlist_arg_list, extra_gnetlist_list;

static int n_deleted, n_added_ef, n_fixed, n_PKG_removed_new,
           n_PKG_removed_old, n_preserved, n_changed_value, n_not_found,
           n_unknown, n_none, n_empty;

static int bak_done, need_PKG_purge;

conf_gsch2pcb_rnd_t conf_g2pr;

static const char *element_search_path = NULL; /* queried once from the config, when the config is already stable */

static char *loc_strndup(const char *str, size_t len)
{
	char *s;
	int l;

	if (str == NULL)
		return NULL;
	l = strlen(str);
	if (l < len)
		len = l;
	s = malloc(len+1);
	memcpy(s, str, len);
	s[len] = '\0';
	return s;
}

/* Return a pointer to the suffix if inp ends in that suffix */
static char *loc_str_has_suffix(char *inp, const char *suffix, int suff_len)
{
	int len = strlen(inp);
	if ((len >= suff_len) && (strcmp(inp + len - suff_len, suffix) == 0))
		return inp + len - suff_len;
	return NULL;
}

/* Checks if a file exists and is readable */
static int file_exists(const char *fn)
{
	FILE *f;
	f = fopen(fn, "r");
	if (f == NULL)
		return 0;
	fclose(f);
	return 1;
}

void ChdirErrorMessage(char *DirName)
{
	fprintf(stderr, "gsch2pcb-rnd: warning: can't cd to %s\n", DirName);
}

void OpendirErrorMessage(char *DirName)
{
	fprintf(stderr, "gsch2pcb-rnd: warning: can't opendir %s\n", DirName);
}

void PopenErrorMessage(char *cmd)
{
	fprintf(stderr, "gsch2pcb-rnd: warning: can't popen %s\n", cmd);
}

void Message(char *err)
{
	fprintf(stderr, "gsch2pcb-rnd: %s\n", err);
}

void hid_usage_option(const char *name, const char *desc)
{
}

/**
 * Build and run a command. No redirection or error handling is
 * done.  Format string is split on whitespace. Specifiers %l and %s
 * are replaced with contents of positional args. To be recognized,
 * specifiers must be separated from other arguments in the format by
 * whitespace.
 *  - %L expects a gadl_list_t vith char * payload, contents used as separate arguments
 *  - %s expects a char*, contents used as a single argument (omitted if NULL)
 * @param[in] format  used to specify command to be executed
 * @param[in] ...     positional parameters
 */
static int build_and_run_command(const char * format_, ...)
{
	va_list vargs;
	int i, status;
	int result = FALSE;
	vts0_t args;
	char *format, *s, *start;

	/* Translate the format string; args elements point to const char *'s
	   within a copy of the format string. The format string is copied so
	   that these parts can be terminated by overwriting whitepsace with \0 */
	va_start(vargs, format_);
	format = strdup(format_);
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

		f = popen(cmd, "r");
		while(fgets(line, sizeof(line), f) != NULL) {
			if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
				fputs(line, stdout);
		}

		if (pclose(f) == 0)
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

/* Run gnetlist to generate a netlist and a PCB board file.  gnetlist
 * has exit status of 0 even if it's given an invalid arg, so do some
 * stat() hoops to decide if gnetlist successfully generated the PCB
 * board file (only gnetlist >= 20030901 recognizes -m).
 */
static int run_gnetlist(char * pins_file, char * net_file, char * pcb_file, char * basename, gadl_list_t *largs)
{
	struct stat st;
	time_t mtime;
	static const char *gnetlist = NULL;
	gadl_iterator_t it;
	char **sp;
	char *verbose_str = NULL;

	/* Allow the user to specify a full path or a different name for
	 * the gnetlist command.  Especially useful if multiple copies
	 * are installed at once.
	 */
	if (gnetlist == NULL)
		gnetlist = getenv("GNETLIST");
	if (gnetlist == NULL)
		gnetlist = "gnetlist";

	if (!conf_g2pr.utils.gsch2pcb_rnd.verbose)
		verbose_str = "-q";

	if (!build_and_run_command("%s %s -g pcbpins -o %s %L %L", gnetlist, verbose_str, pins_file, &extra_gnetlist_arg_list, largs))
		return FALSE;

	if (!build_and_run_command("%s %s -g PCB -o %s %L %L", gnetlist, verbose_str, net_file, &extra_gnetlist_arg_list, largs))
		return FALSE;

	mtime = (stat(pcb_file, &st) == 0) ? st.st_mtime : 0;

	if (!build_and_run_command("%s %s -L " SCMDIR " -g gsch2pcb-rnd -o %s %L %L",
														 gnetlist, verbose_str, pcb_file, &extra_gnetlist_arg_list, largs)) {
		if (stat(pcb_file, &st) != 0 || mtime == st.st_mtime) {
			fprintf(stderr, "gsch2pcb: gnetlist command failed, `%s' not updated\n", pcb_file);
			return FALSE;
		}
		return FALSE;
	}

	gadl_foreach(&extra_gnetlist_list, &it, sp) {
		const char *s = *sp;
		const char *s2 = strstr(s, " -o ");
		char *out_file;
		char *backend;
		if (!s2) {
			out_file = str_concat(NULL, basename, ".", s, NULL);
			backend = strdup(s);
		}
		else {
			out_file = strdup(s2 + 4);
			backend = loc_strndup(s, s2 - s);
		}

		if (!build_and_run_command("%s %s -g %s -o %s %L %L",
															 gnetlist, verbose_str, backend, out_file, &extra_gnetlist_arg_list, largs))
			return FALSE;
		free(out_file);
		free(backend);
	}

	return TRUE;
}

static char *token(char * string, char ** next, int * quoted_ret, int parenth)
{
	static char *str;
	char *s, *ret;
	int quoted = FALSE;

	if (string)
		str = string;
	if (!str || !*str) {
		if (next)
			*next = str;
		return strdup("");
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
	ret = loc_strndup(str, s - str);
	str = (quoted && *s) ? s + 1 : s;
	if (next)
		*next = str;
	return ret;
}

static char *fix_spaces(char * str)
{
	char *s;

	if (!str)
		return NULL;
	for (s = str; *s; ++s)
		if (*s == ' ' || *s == '\t')
			*s = '_';
	return str;
}

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

	el->tail = strdup(t ? t : "");
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

static void pcb_element_free(PcbElement * el)
{
	if (!el)
		return;
	free(el->flags);
	free(el->description);
	free(el->changed_description);
	free(el->changed_value);
	free(el->refdes);
	free(el->value);
	free(el->x);
	free(el->y);
	free(el->tail);
	free(el->pkg_name_fix);
	free(el);
}

static void get_pcb_element_list(char * pcb_file)
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

static PcbElement *pcb_element_exists(PcbElement * el_test, int record)
{
	PcbElement *el;
	gdl_iterator_t it;

	gdl_foreach(&pcb_element_list, &it, el) {
		if (strcmp(el_test->refdes, el->refdes))
			continue;
		if (strcmp(el_test->description, el->description)) {	/* footprint */
			if (record)
				el->changed_description = strdup(el_test->description);
		}
		else {
			if (record) {
				if (strcmp(el_test->value, el->value))
					el->changed_value = strdup(el_test->value);
				el->still_exists = TRUE;
			}
			return el;
		}
	}
	return NULL;
}

/* A problem is that new PCB 1.7 file elements have the
 * (mark_x,mark_y) value set to wherever the element was created and
 * no equivalent of a gschem translate symbol was done.
 *
 * So, file elements inserted can be scattered over a big area and
 * this is bad when loading a file.new.pcb into an existing PC
 * board.  So, do a simple translate if (mark_x,mark_y) is
 * (arbitrarily) over 1000.  I'll assume that for values < 1000 the
 * element creator was concerned with a sane initial element
 * placement.  Unless someone has a better idea?  Don't bother with
 * pre PCB 1.7 formats as that would require parsing the mark().
 */
static void simple_translate(PcbElement * el)
{

	el->x = strdup("0");
	el->y = strdup("0");
}

static int insert_element(FILE * f_out, FILE * f_elem, char * footprint, char * refdes, char * value)
{
	PcbElement *el;
	char *fmt, *s, buf[1024];
	int retval = FALSE;

	/* Copy the file element lines.  Substitute new parameters into the
	 * Element() or Element[] line and strip comments.
	 */
	while ((fgets(buf, sizeof(buf), f_elem)) != NULL) {
		for (s = buf; *s == ' ' || *s == '\t'; ++s);
		if ((el = pcb_element_line_parse(s)) != NULL) {
			simple_translate(el);
			fmt = el->quoted_flags ? "Element%c\"%s\" \"%s\" \"%s\" \"%s\" %s %s%s\n" : "Element%c%s \"%s\" \"%s\" \"%s\" %s %s%s\n";

			fprintf(f_out, fmt, el->res_char, el->flags, footprint, refdes, value, el->x, el->y, el->tail);
			retval = TRUE;
		}
		else if (*s != '#')
			fputs(buf, f_out);
		pcb_element_free(el);
	}
	return retval;
}


char *search_element(PcbElement * el)
{
	char *elname = NULL, *path = NULL;

	if (!elname)
		elname = strdup(el->description);

	if (!strcmp(elname, "unknown")) {
		free(elname);
		return NULL;
	}
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose > 1)
		printf("\tSearching directories looking for file element: %s\n", elname);
	free(elname);
	return path;
}

/* The gnetlist backend gnet-gsch2pcb-rnd.scm generates PKG lines:
 *
 *        PKG(footprint,refdes,value)
 *
 */
static PcbElement *pkg_to_element(FILE * f, char * pkg_line)
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
		fprintf(stderr, "Bad package line: %s\n", pkg_line);
		return NULL;
	}

	fix_spaces(refdes);
	fix_spaces(value);

	el = calloc(sizeof(PcbElement), 1);
	el->description = strdup(fp);
	el->refdes = strdup(refdes);
	el->value = strdup(value);

// wtf?
//  if ((s = strchr (el->value, (int) ')')) != NULL)
//    *s = '\0';

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

/* Copies the content of fn to fout and returns 0 on success. */
static int CatPCB(FILE * fout, const char *fn)
{
	FILE *fin;
	fin = fopen(fn, "r");
	if (fin == NULL)
		return -1;

	for (;;) {
		char buff[1024];
		int len;

		len = fread(buff, 1, sizeof(buff), fin);
		if (len <= 0)
			break;

		fwrite(buff, len, 1, fout);
	}

	fclose(fin);
	return 0;
}

/* Process the newly created pcb file which is the output from
 *     gnetlist -g gsch2pcb-rnd ...
 *
 * Insert elements for PKG_ lines if they be found by external element query.
 * If there was an existing pcb file, strip out any elements if they are
 * already present so that the new pcb file will only have new elements.
 */
static int add_elements(char * pcb_file)
{
	FILE *f_in, *f_out, *fp;
	PcbElement *el = NULL;
	char *tmp_file, *s, buf[1024];
	int total, paren_level = 0;
	int skipping = FALSE;
	int dpcb;
	fp_fopen_ctx_t fctx;

	if ((f_in = fopen(pcb_file, "r")) == NULL)
		return 0;
	tmp_file = str_concat(NULL, pcb_file, ".tmp", NULL);
	if ((f_out = fopen(tmp_file, "wb")) == NULL) {
		fclose(f_in);
		free(tmp_file);
		return 0;
	}

	if (conf_g2pr.utils.gsch2pcb_rnd.default_pcb == NULL) {
		dpcb = -1;
		conf_list_foreach_path_first(dpcb, &conf_core.rc.default_pcb_file, CatPCB(f_out, __path__));
		if (dpcb != 0) {
			fprintf(stderr, "ERROR: can't load default pcb (using the configured search paths)\n");
			exit(1);
		}
	}
	else {
		if (CatPCB(f_out, conf_g2pr.utils.gsch2pcb_rnd.default_pcb) != 0) {
			fprintf(stderr, "ERROR: can't load default pcb (using user defined %s)\n", conf_g2pr.utils.gsch2pcb_rnd.default_pcb);
			exit(1);
		}
	}

	while ((fgets(buf, sizeof(buf), f_in)) != NULL) {
		for (s = buf; *s == ' ' || *s == '\t'; ++s);
		if (skipping) {
			if (*s == '(')
				++paren_level;
			else if (*s == ')' && --paren_level <= 0)
				skipping = FALSE;
			continue;
		}
		el = pkg_to_element(f_out, s);
		if (el && pcb_element_exists(el, TRUE)) {
			pcb_element_free(el);
			continue;
		}
		if (!el || el->omit_PKG) {
			if (el) {

			}
			else
				fputs(buf, f_out);
			continue;
		}

		{
			if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
				printf("%s: need new element for footprint  %s (value=%s)\n", el->refdes, el->description, el->value);

			fp = fp_fopen(element_search_path, el->description, &fctx);

			if (fp == NULL && conf_g2pr.utils.gsch2pcb_rnd.verbose)
				printf("\tNo file element found.\n");

			if ((fp != NULL) && insert_element(f_out, fp, el->description, el->refdes, el->value)) {
				++n_added_ef;
				if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
					printf("%s: added new element for footprint %s (value=%s)\n", el->refdes, el->description, el->value);
			}
			else {
				fprintf(stderr, "%s: can't find PCB element for footprint %s (value=%s)\n", el->refdes, el->description, el->value);
				if (conf_g2pr.utils.gsch2pcb_rnd.remove_unfound_elements && !conf_g2pr.utils.gsch2pcb_rnd.fix_elements) {
					fprintf(stderr, "So device %s will not be in the layout.\n", el->refdes);
					++n_PKG_removed_new;
				}
				else {
					++n_not_found;
					fputs(buf, f_out);		/* Copy PKG_ line */
				}
			}
			if (fp != NULL)
				fp_fclose(fp, &fctx);
		}

		pcb_element_free(el);
		if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
			printf("----\n");
	}
	fclose(f_in);
	fclose(f_out);

	total = n_added_ef + n_not_found;
	if (total == 0)
		build_and_run_command("rm %s", tmp_file);
	else
		build_and_run_command("mv %s %s", tmp_file, pcb_file);
	free(tmp_file);
	return total;
}

static void update_element_descriptions(char * pcb_file, char * bak)
{
	FILE *f_in, *f_out;
	PcbElement *el, *el_exists;
	char *fmt, *tmp, *s, buf[1024];
	gdl_iterator_t it;

	gdl_foreach(&pcb_element_list, &it, el) {
		if (el->changed_description)
			++n_fixed;
	}
	if (!pcb_element_list.length || n_fixed == 0) {
		fprintf(stderr, "Could not find any elements to fix.\n");
		return;
	}
	if ((f_in = fopen(pcb_file, "r")) == NULL)
		return;
	tmp = str_concat(NULL, pcb_file, ".tmp", NULL);
	if ((f_out = fopen(tmp, "wb")) == NULL) {
		fclose(f_in);
		return;
	}
	while ((fgets(buf, sizeof(buf), f_in)) != NULL) {
		for (s = buf; *s == ' ' || *s == '\t'; ++s);
		if ((el = pcb_element_line_parse(s)) != NULL
				&& (el_exists = pcb_element_exists(el, FALSE)) != NULL && el_exists->changed_description) {
			fmt = el->quoted_flags ? "Element%c\"%s\" \"%s\" \"%s\" \"%s\" %s %s%s\n" : "Element%c%s \"%s\" \"%s\" \"%s\" %s %s%s\n";
			fprintf(f_out, fmt,
							el->res_char, el->flags, el_exists->changed_description, el->refdes, el->value, el->x, el->y, el->tail);
			printf("%s: updating element Description: %s -> %s\n", el->refdes, el->description, el_exists->changed_description);
			el_exists->still_exists = TRUE;
		}
		else
			fputs(buf, f_out);
		pcb_element_free(el);
	}
	fclose(f_in);
	fclose(f_out);

	if (!bak_done) {
		build_and_run_command("mv %s %s", pcb_file, bak);
		bak_done = TRUE;
	}

	build_and_run_command("mv %s %s", tmp, pcb_file);
	free(tmp);
}

static void prune_elements(char * pcb_file, char * bak)
{
	FILE *f_in, *f_out;
	PcbElement *el, *el_exists;
	char *fmt, *tmp, *s, buf[1024];
	int paren_level = 0;
	int skipping = FALSE;
	gdl_iterator_t it;

	gdl_foreach(&pcb_element_list, &it, el) {
		if (!el->still_exists) {
			if ((conf_g2pr.utils.gsch2pcb_rnd.preserve) || (el->nonetlist)) {
				++n_preserved;
				if (conf_g2pr.utils.gsch2pcb_rnd.verbose > 1)
					fprintf(stderr,
									"Preserving PCB element not in the schematic:    %s (element   %s) %s\n",
									el->refdes, el->description, el->nonetlist ? "nonetlist" : "");
			}
			else
				++n_deleted;
		}
		else if (el->changed_value)
			++n_changed_value;
	}
	if ((pcb_element_list.length == 0) || (n_deleted == 0 && !need_PKG_purge && n_changed_value == 0)) {
		return;
	}
	if ((f_in = fopen(pcb_file, "r")) == NULL) {
		fprintf(stderr, "error: can not read %s\n", pcb_file);
		return;
	}
	tmp = str_concat(NULL, pcb_file, ".tmp", NULL);
	if ((f_out = fopen(tmp, "wb")) == NULL) {
		fprintf(stderr, "error: can not write %s\n", tmp);
		fclose(f_in);
		return;
	}

	while ((fgets(buf, sizeof(buf), f_in)) != NULL) {
		for (s = buf; *s == ' ' || *s == '\t'; ++s);
		if (skipping) {
			if (*s == '(')
				++paren_level;
			else if (*s == ')' && --paren_level <= 0)
				skipping = FALSE;
			continue;
		}
		el_exists = NULL;
		if ((el = pcb_element_line_parse(s)) != NULL
				&& (el_exists = pcb_element_exists(el, FALSE)) != NULL && !el_exists->still_exists && !conf_g2pr.utils.gsch2pcb_rnd.preserve && !el->nonetlist) {
			skipping = TRUE;
			if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
				printf("%s: deleted element %s (value=%s)\n", el->refdes, el->description, el->value);
			pcb_element_free(el);
			continue;
		}
		if (el_exists && el_exists->changed_value) {
			fmt = el->quoted_flags ? "Element%c\"%s\" \"%s\" \"%s\" \"%s\" %s %s%s\n" : "Element%c%s \"%s\" \"%s\" \"%s\" %s %s%s\n";
			fprintf(f_out, fmt,
							el->res_char, el->flags, el->description, el->refdes, el_exists->changed_value, el->x, el->y, el->tail);
			if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
				printf("%s: changed element %s value: %s -> %s\n", el->refdes, el->description, el->value, el_exists->changed_value);
		}
		else if (!strncmp(s, "PKG_", 4))
			++n_PKG_removed_old;
		else
			fputs(buf, f_out);
		pcb_element_free(el);
	}
	fclose(f_in);
	fclose(f_out);

	if (!bak_done) {
		build_and_run_command("mv %s %s", pcb_file, bak);
		bak_done = TRUE;
	}

	build_and_run_command("mv %s %s", tmp, pcb_file);
	free(tmp);
}

static void add_schematic(char * sch)
{
	const char *s;
	char **n;
	n = gadl_new(&schematics);
	*n = strdup(sch);
	gadl_append(&schematics, n);
	if (!conf_g2pr.utils.gsch2pcb_rnd.sch_basename) {
		char *suff = loc_str_has_suffix(sch, ".sch", 4);
		if (suff != NULL) {
			char *tmp = loc_strndup(sch, suff - sch);
			conf_set(CFR_CLI, "utils/gsch2pcb_rnd/sch_basename", -1, tmp, POL_OVERWRITE);
			free(tmp);
		}
	}
}

static void add_multiple_schematics(const char * sch)
{
	/* parse the string using shell semantics */
	int count;
	char **args;
	char *tmp = strdup(sch);

	count = qparse(tmp, &args);
	free(tmp);
	if (count > 0) {
		int i;
		for (i = 0; i < count; ++i)
			if (*args[i] != '\0')
				add_schematic(args[i]);
		qparse_free(count, &args);
	}
	else {
		fprintf(stderr, "invalid `schematics' option: %s\n", sch);
	}
}

static int parse_config(char * config, char * arg)
{
	char *s;

	/* remove trailing white space otherwise strange things can happen */
	if ((arg != NULL) && (strlen(arg) >= 1)) {
		s = arg + strlen(arg) - 1;
		while ((*s == ' ' || *s == '\t') && (s != arg))
			s--;
		s++;
		*s = '\0';
	}
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("    %s \"%s\"\n", config, arg ? arg : "");

	if (!strcmp(config, "remove-unfound") || !strcmp(config, "r")) {
		/* This is default behavior set in header section */
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/remove_unfound_elements", -1, "1", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "keep-unfound") || !strcmp(config, "k")) {
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/remove_unfound_elements", -1, "0", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "quiet") || !strcmp(config, "q")) {
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/quiet_mode", -1, "1", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "preserve") || !strcmp(config, "p")) {
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/preserve", -1, "1", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "elements-shell") || !strcmp(config, "s"))
		conf_set(CFR_CLI, "rc/library_shell", -1, arg, POL_OVERWRITE);
	else if (!strcmp(config, "elements-dir") || !strcmp(config, "d"))
		conf_set(CFR_CLI, "rc/library_search_paths", -1, arg, POL_PREPEND);
	else if (!strcmp(config, "output-name") || !strcmp(config, "o"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/sch_base", -1, arg, POL_OVERWRITE);
	else if (!strcmp(config, "default-pcb") || !strcmp(config, "P"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/default_pcb", -1, arg, POL_OVERWRITE);
	else if (!strcmp(config, "schematics"))
		add_multiple_schematics(arg);
	else if (!strcmp(config, "gnetlist")) {
		char **n;
		n = gadl_new(&extra_gnetlist_list);
		*n = strdup(arg);
		gadl_append(&extra_gnetlist_list, n);
	}
	else if (!strcmp(config, "empty-footprint"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/empty_footprint_name", -1, arg, POL_OVERWRITE);
	else
		return -1;

	return 1;
}

static void load_project(char * path)
{
	FILE *f;
	char *s, buf[1024], config[32], arg[768];

	f = fopen(path, "r");
	if (!f)
		return;
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("Reading project file: %s\n", path);
	while (fgets(buf, sizeof(buf), f)) {
		for (s = buf; *s == ' ' || *s == '\t' || *s == '\n'; ++s);
		if (!*s || *s == '#' || *s == '/' || *s == ';')
			continue;
		arg[0] = '\0';
		sscanf(s, "%31s %767[^\n]", config, arg);
		parse_config(config, arg);
	}
	fclose(f);
}

static void load_extra_project_files(void)
{
	char *path;
	static int done = FALSE;

	if (done)
		return;

	load_project("/etc/gsch2pcb");
	load_project("/usr/local/etc/gsch2pcb");

	path = str_concat(PCB_DIR_SEPARATOR_S, conf_core.rc.path.home, ".gEDA", "gsch2pcb", NULL);
	load_project(path);
	free(path);

	done = TRUE;
}

static void get_args(int argc, char ** argv)
{
	char *opt, *arg;
	int i, r;

	for (i = 1; i < argc; ++i) {
		opt = argv[i];
		arg = argv[i + 1];
		if (*opt == '-') {
			++opt;
			if (*opt == '-')
				++opt;
			if (!strcmp(opt, "version") || !strcmp(opt, "V")) {
				printf("gsch2pcb-rnd %s\n", GSCH2PCB_RND_VERSION);
				exit(0);
			}
			else if (!strcmp(opt, "verbose") || !strcmp(opt, "v")) {
				char tmp[16];
				sprintf(tmp, "%d", conf_g2pr.utils.gsch2pcb_rnd.verbose + 1);
				conf_set(CFR_CLI, "utils/gsch2pcb_rnd/verbose", -1, tmp, POL_OVERWRITE);
				continue;
			}
			else if (!strcmp(opt, "c") || !strcmp(opt, "conf")) {
				const char *stmp;
				if (conf_set_from_cli(NULL, arg, NULL, &stmp) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, stmp);
					exit(1);
				}
				i++;
				continue;
			}
			else if (!strcmp(opt, "fix-elements")) {
				conf_set(CFR_CLI, "utils/gsch2pcb_rnd/fix_elements", -1, "1", POL_OVERWRITE);
				continue;
			}
			else if (!strcmp(opt, "gnetlist-arg")) {
				char **n;
				n = gadl_new(&extra_gnetlist_arg_list);
				*n = strdup(arg);
				gadl_append(&extra_gnetlist_arg_list, n);
				i++;
				continue;
			}
			else if (!strcmp(opt, "help") || !strcmp(opt, "h"))
				usage();
			else if (i < argc && ((r = parse_config(opt, (i < argc - 1) ? arg : NULL))
														>= 0)
				) {
				i += r;
				continue;
			}
			printf("gsch2pcb: bad or incomplete arg: %s\n", argv[i]);
			usage();
		}
		else {
			if (loc_str_has_suffix(argv[i], ".sch", 4) == NULL) {
				load_extra_project_files();
				load_project(argv[i]);
			}
			else
				add_schematic(argv[i]);
		}
	}
}

/* Dummy pcb-rnd for the fp lib to work */
int Library;

static void bozo()
{
	fprintf(stderr, "bozo: pcb-rnd footprint plugin compatibility error.\n");
	abort();
}

void *GetLibraryEntryMemory(void *Menu)                { bozo(); }
void * GetLibraryMenuMemory(void *lib, int *idx)       { bozo(); }
void DeleteLibraryMenuMemory(void *lib, int menuidx)   { bozo(); }
void sort_library(void *lib)                           { bozo(); }

void plugin_register(const char *name, const char *path, void *handle, int dynamic_loaded, void (*uninit)(void))
{

}

void hid_notify_conf_changed(void)
{

}


/************************ main ***********************/
char *pcb_file_name, *pcb_new_file_name, *bak_file_name, *pins_file_name, *net_file_name;
int main(int argc, char ** argv)
{
	char *tmp;
	int i;
	int initial_pcb = TRUE;
	int created_pcb_file = TRUE;

	if (argc < 2)
		usage();

	{
		pcb_uninit_t uninit_func;
#		include "fp_init.c"
	}

	conf_init();
	conf_core_init();

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_g2pr, field,isarray,type_name,cpath,cname,desc,flags);
#include "gsch2pcb_rnd_conf_fields.h"

	get_args(argc, argv);

#warning TODO: do not use NULL,NULL here
	conf_load_all(NULL, NULL);
	conf_update(NULL);

	fp_init();

	gadl_list_init(&schematics, sizeof(char *), NULL, NULL);
	gadl_list_init(&extra_gnetlist_arg_list, sizeof(char *), NULL, NULL);
	gadl_list_init(&extra_gnetlist_list, sizeof(char *), NULL, NULL);

	conf_update(NULL); /* because of CLI changes */

	load_extra_project_files();

	element_search_path = fp_default_search_path();

	if (gadl_length(&schematics) == 0)
		usage();

	pins_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".cmd", NULL);
	net_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".net", NULL);
	pcb_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb", NULL);


	{ /* set bak_file_name, finding the first number that results in a non-existing bak */
		int len;
		char *end;

		len = strlen(conf_g2pr.utils.gsch2pcb_rnd.sch_basename);
		bak_file_name = malloc(len+8+64); /* make room for ".pcb.bak" and an integer */
		memcpy(bak_file_name, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, len);
		end = bak_file_name + len;
		strcpy(end, ".pcb.bak");
		end += 8;

		for (i = 0; file_exists(bak_file_name); ++i)
			sprintf(end, "%d", i);
	}

	if (file_exists(pcb_file_name)) {
		initial_pcb = FALSE;
		pcb_new_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".new.pcb", NULL);
		get_pcb_element_list(pcb_file_name);
	}
	else
		pcb_new_file_name = strdup(pcb_file_name);

	if (!run_gnetlist(pins_file_name, net_file_name, pcb_new_file_name, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, &schematics)) {
		fprintf(stderr, "Failed to run gnetlist\n");
		exit(1);
	}

	if (add_elements(pcb_new_file_name) == 0) {
		build_and_run_command("rm %s", pcb_new_file_name);
		if (initial_pcb) {
			printf("No elements found, so nothing to do.\n");
			exit(0);
		}
	}

	if (conf_g2pr.utils.gsch2pcb_rnd.fix_elements)
		update_element_descriptions(pcb_file_name, bak_file_name);
	prune_elements(pcb_file_name, bak_file_name);

	/* Report work done during processing */
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("\n");
	printf("\n----------------------------------\n");
	printf("Done processing.  Work performed:\n");
	if (n_deleted > 0 || n_fixed > 0 || need_PKG_purge || n_changed_value > 0)
		printf("%s is backed up as %s.\n", pcb_file_name, bak_file_name);
	if (pcb_element_list.length && n_deleted > 0)
		printf("%d elements deleted from %s.\n", n_deleted, pcb_file_name);

	if (n_added_ef > 0)
		printf("%d file elements added to %s.\n", n_added_ef, pcb_new_file_name);
	else if (n_not_found == 0) {
		printf("No elements to add so not creating %s\n", pcb_new_file_name);
		created_pcb_file = FALSE;
	}

	if (n_not_found > 0) {
		printf("%d not found elements added to %s.\n", n_not_found, pcb_new_file_name);
	}
	if (n_unknown > 0)
		printf("%d components had no footprint attribute and are omitted.\n", n_unknown);
	if (n_none > 0)
		printf("%d components with footprint \"none\" omitted from %s.\n", n_none, pcb_new_file_name);
	if (n_empty > 0)
		printf("%d components with empty footprint \"%s\" omitted from %s.\n", n_empty, conf_g2pr.utils.gsch2pcb_rnd.empty_footprint_name, pcb_new_file_name);
	if (n_changed_value > 0)
		printf("%d elements had a value change in %s.\n", n_changed_value, pcb_file_name);
	if (n_fixed > 0)
		printf("%d elements fixed in %s.\n", n_fixed, pcb_file_name);
	if (n_PKG_removed_old > 0) {
		printf("%d elements could not be found.", n_PKG_removed_old);
		if (created_pcb_file)
			printf("  So %s is incomplete.\n", pcb_file_name);
		else
			printf("\n");
	}
	if (n_PKG_removed_new > 0) {
		printf("%d elements could not be found.", n_PKG_removed_new);
		if (created_pcb_file)
			printf("  So %s is incomplete.\n", pcb_new_file_name);
		else
			printf("\n");
	}
	if (n_preserved > 0)
		printf("%d elements not in the schematic preserved in %s.\n", n_preserved, pcb_file_name);

	/* Tell user what to do next */
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("\n");

	if (n_added_ef > 0)
		next_steps(initial_pcb, conf_g2pr.utils.gsch2pcb_rnd.quiet_mode);

	free(net_file_name);
	free(pins_file_name);
	free(pcb_file_name);
	free(bak_file_name);

	return 0;
}
