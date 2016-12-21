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
#include <ctype.h>
#include "gsch2pcb.h"
#include "gsch2pcb_rnd_conf.h"
#include "method_pcb.h"
#include "run.h"
#include "netlister.h"
#include "method.h"
#include "../src/plug_footprint.h"
#include "../src/paths.h"
#include "../src/conf.h"
#include "../src/conf_core.h"
#include "../src/compat_misc.h"
#include "../src/compat_fs.h"
#include "../src/plugins.h"
#include "../src/plug_footprint.h"

static const char *element_search_path = NULL; /* queried once from the config, when the config is already stable */

static int insert_element(FILE * f_out, FILE * f_elem, char * footprint, char * refdes, char * value);

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

static PcbElement *pcb_element_exists(PcbElement * el_test, int record)
{
	PcbElement *el;
	gdl_iterator_t it;

	gdl_foreach(&pcb_element_list, &it, el) {
		if (strcmp(el_test->refdes, el->refdes))
			continue;
		if (strcmp(el_test->description, el->description)) {	/* footprint */
			if (record)
				el->changed_description = pcb_strdup(el_test->description);
		}
		else {
			if (record) {
				if (strcmp(el_test->value, el->value))
					el->changed_value = pcb_strdup(el_test->value);
				el->still_exists = TRUE;
			}
			return el;
		}
	}
	return NULL;
}

/*
static char *search_element(PcbElement * el)
{
	char *elname = NULL, *path = NULL;

	if (!elname)
		elname = pcb_strdup(el->description);

	if (!strcmp(elname, "unknown")) {
		free(elname);
		return NULL;
	}
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose > 1)
		printf("\tSearching directories looking for file element: %s\n", elname);
	free(elname);
	return path;
}
*/

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
static PcbElement *pcb_element_line_parse(char * line)
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
	pcb_fp_fopen_ctx_t fctx;

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

			fp = pcb_fp_fopen(element_search_path, el->description, &fctx);

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
					fprintf(f_out, "# gsch2pcb-rnd: element not found: %s\n", buf); /* Copy PKG_ line as comment */
				}
			}
			if (fp != NULL)
				pcb_fp_fclose(fp, &fctx);
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
	if (el->x != NULL)
		free(el->x);
	if (el->y != NULL)
		free(el->y);
	el->x = pcb_strdup("0");
	el->y = pcb_strdup("0");
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

/********************** The actual actions we perform ************************/

static char *pcb_file_name, *pcb_new_file_name, *bak_file_name, *pins_file_name, *net_file_name;

static void method_pcb_init(void)
{
	pcb_fp_init();
	pins_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".cmd", NULL);
	net_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".net", NULL);
	pcb_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb", NULL);
	local_project_pcb_name = pcb_file_name;
}

static void next_steps(int initial_pcb, int quiet_mode)
{
	if (initial_pcb) {
		printf("\nNext step:\n");
		printf("1.  Run pcb on your file %s.\n", pcb_file_name);
		printf("    You will find all your footprints in a bundle ready for you to place\n");
		printf("    or disperse with \"Select -> Disperse all elements\" in PCB.\n\n");
		printf("2.  From within PCB, select \"File -> Load netlist file\" and select \n");
		printf("    %s to load the netlist.\n\n", net_file_name);
		printf("3.  From within PCB, enter\n\n");
		printf("           :ExecuteFile(%s)\n\n", pins_file_name);
		printf("    to propagate the pin names of all footprints to the layout.\n\n");
	}
	else if (!quiet_mode) {
		printf("\nNext steps:\n");
		printf("1.  Run pcb on your file %s.\n", pcb_file_name);
		printf("2.  From within PCB, select \"File -> Load layout data to paste buffer\"\n");
		printf("    and select %s to load the new footprints into your existing layout.\n", pcb_new_file_name);
		printf("3.  From within PCB, select \"File -> Load netlist file\" and select \n");
		printf("    %s to load the updated netlist.\n\n", net_file_name);
		printf("4.  From within PCB, enter\n\n");
		printf("           :ExecuteFile(%s)\n\n", pins_file_name);
		printf("    to update the pin names of all footprints.\n\n");
	}
}

static void method_pcb_go()
{
	int initial_pcb = TRUE;
	int created_pcb_file = TRUE;

	if (!conf_g2pr.utils.gsch2pcb_rnd.quiet_mode)
		fprintf(stderr, "gsch2pcb-rnd: WARNING: Please consider switching from -m pcb to -m import because -m pcb is deprecated. (This warning can be suppressed with -q).\n");

	element_search_path = pcb_fp_default_search_path();

	{ /* set bak_file_name, finding the first number that results in a non-existing bak */
		int len;
		char *end;
		int i;

		len = strlen(conf_g2pr.utils.gsch2pcb_rnd.sch_basename);
		bak_file_name = malloc(len+8+64); /* make room for ".pcb.bak" and an integer */
		memcpy(bak_file_name, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, len);
		end = bak_file_name + len;
		strcpy(end, ".pcb.bak");
		end += 8;

		for (i = 0; pcb_file_readable(bak_file_name); ++i)
			sprintf(end, "%d", i);
	}

	if (pcb_file_readable(pcb_file_name)) {
		initial_pcb = FALSE;
		pcb_new_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".new.pcb", NULL);
		get_pcb_element_list(pcb_file_name);
	}
	else
		pcb_new_file_name = pcb_strdup(pcb_file_name);

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
}

static int method_pcb_guess_out_name(void)
{
	int res;
	char *name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb", NULL);
	res = pcb_file_readable(name);
	free(name);
	return res;
}


void method_pcb_uninit(void)
{
	if (pcb_new_file_name != NULL)
		free(pcb_new_file_name);
}

static method_t method_pcb;

void method_pcb_register(void)
{
	method_pcb.name = "pcb";
	method_pcb.desc = "traditional: load footprints and edit the .pcb files";
	method_pcb.init = method_pcb_init;
	method_pcb.go = method_pcb_go;
	method_pcb.uninit = method_pcb_uninit;
	method_pcb.guess_out_name = method_pcb_guess_out_name;
	method_register(&method_pcb);
}
