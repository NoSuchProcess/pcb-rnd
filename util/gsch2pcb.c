/* gsch2pcb
 *
 *  Original version: Bill Wilson    billw@wt.net
 *  rnd-version: (C) 2015, Tibor 'Igor2' Palinkas
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

 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/libpcb_fp.h"
#include "../config.h"

#define GSC2PCB_VERSION "1.6"

#define DEFAULT_PCB_INC "pcb.inc"

#define SEP_STRING "--------\n"

typedef struct
{
  gchar *refdes, *value, *description, *changed_description, *changed_value;
  gchar *flags, *tail;
  gchar *x, *y;
  gchar *pkg_name_fix;
  gchar res_char;

  gboolean still_exists, new_format, hi_res_format, quoted_flags, omit_PKG, nonetlist;
}
PcbElement;


typedef struct
{
  gchar *part_number, *element_name;
}
ElementMap;

static GList *pcb_element_list,
  *extra_gnetlist_list, *extra_gnetlist_arg_list;

static gchar *sch_basename;

static GList *schematics;

static gchar *empty_footprint_name;

static gint verbose,
  n_deleted,
  n_added_ef,
  n_fixed,
  n_PKG_removed_new,
  n_PKG_removed_old,
  n_preserved, n_changed_value, n_not_found, n_unknown, n_none, n_empty;

static gboolean remove_unfound_elements = TRUE,
  quiet_mode = FALSE,
  preserve, fix_elements, bak_done, need_PKG_purge;


/**
 * Build and run a command. No redirection or error handling is
 * done.  Format string is split on whitespace. Specifiers %l and %s
 * are replaced with contents of positional args. To be recognized,
 * specifiers must be separated from other arguments in the format by
 * whitespace.
 *  - %l expects a GList, contents used as separate arguments
 *  - %s expects a gchar*, contents used as a single argument
 * @param[in] format  used to specify command to be executed
 * @param[in] ...     positional parameters
 */
static gboolean
build_and_run_command (const gchar *format, ...)
{
  va_list vargs;
  gchar ** split;
  GList *tmp = NULL;
  gint num_split;
  gint i;
  gint status;
  gboolean result = FALSE;
  gboolean spawn_result;
  gchar *standard_output = NULL;
  gchar *standard_error = NULL;
  GError * error = NULL;

  va_start (vargs, format);
  split = g_strsplit_set (format, " \t\n\v", 0);
  num_split = g_strv_length (split);
  for (i = 0; i < num_split; ++i) {
    gchar *chunk = split[i];
    if (strcmp (chunk, "%l") == 0) {
      /* append contents of list into command args - shared data */
      tmp = g_list_concat (tmp, g_list_copy (va_arg (vargs, GList*)));
    } else if (strcmp (chunk, "%s") == 0) {
      /* insert contents of string into output */
      tmp = g_list_append (tmp, va_arg (vargs, gchar*));
    } else {
      /* bare string, use as is */
      tmp = g_list_append (tmp, chunk);
    }
  }
  va_end (vargs);

  if (tmp) {
    /* we have something in the list, build & call command */
    GList *p;
    gint i = 0;
    gchar ** args = g_new0 (gchar*, g_list_length (tmp) + 1/* NULL terminate the list */);

    if (verbose)
      printf ("Running command:\n\t");

    for (p = tmp; p; p = g_list_next (p)) {
      args[i++] = (gchar*) p->data;
      if (verbose)
        printf ("%s ", (char*)p->data);
    }

    if (verbose)
      printf ("\n%s", SEP_STRING);
    
    if (g_spawn_sync (".",                  /* Working directory */
                      args,                 /* argv */
                      NULL,                 /* envp */
                      G_SPAWN_SEARCH_PATH,  /* flags */
                      NULL,                 /* child_setup */
                      NULL,                 /* user data */
                      &standard_output,     /* standard output */
                      &standard_error,      /* standard error */
                      &status,              /* exit status return */
                      &error)) {            /* GError return */
      if (verbose)
        fputs(standard_output, stdout);
      if (status == 0)
        result = TRUE;
      else {
        if (standard_error)
          fputs(standard_error, stderr);
      }
    }
    else {
      fprintf(stderr, "Failed to execute external program: %s\n", error->message);
      g_error_free(error);
    }

    if (verbose)
      printf ("\n%s", SEP_STRING);

    g_free(standard_error);
    g_free (standard_output);
    
    g_free (args);
    /* free the list, but leave data untouched */
    g_list_free (tmp);
  }

  g_strfreev (split);

  return result;
}

/* Run gnetlist to generate a netlist and a PCB board file.  gnetlist
 * has exit status of 0 even if it's given an invalid arg, so do some
 * stat() hoops to decide if gnetlist successfully generated the PCB
 * board file (only gnetlist >= 20030901 recognizes -m).
 */
static gboolean
run_gnetlist (gchar * pins_file, gchar * net_file, gchar * pcb_file,
              gchar * basename, GList * largs)
{
  struct stat st;
  time_t mtime;
  static const gchar *gnetlist = NULL;
  GList *list = NULL;
  GList *verboseList = NULL;
  GList *args1 = NULL;

  /* Allow the user to specify a full path or a different name for
   * the gnetlist command.  Especially useful if multiple copies
   * are installed at once.
   */
  if (gnetlist == NULL)
    gnetlist = g_getenv ("GNETLIST");
  if (gnetlist == NULL)
    gnetlist = "gnetlist";

  if (!verbose)
    verboseList = g_list_append (verboseList, "-q");

  if (!build_and_run_command ("%s %l -g pcbpins -o %s %l %l",
			      gnetlist,
			      verboseList,
			      pins_file,
			      extra_gnetlist_arg_list,
			      largs))
    return FALSE;

  if (!build_and_run_command ("%s %l -g PCB -o %s %l %l",
			      gnetlist,
			      verboseList,
			      net_file,
			      extra_gnetlist_arg_list,
			      largs))
    return FALSE;

  mtime = (stat (pcb_file, &st) == 0) ? st.st_mtime : 0;

  if (!build_and_run_command ("%s %l -g gsch2pcb-rnd -o %s %l %l %l",
			      gnetlist,
			      verboseList,
			      pcb_file,
			      args1,
			      extra_gnetlist_arg_list,
                  largs)) {
      if (stat (pcb_file, &st) != 0 || mtime == st.st_mtime) {
          fprintf (stderr,
                   "gsch2pcb: gnetlist command failed, `%s' not updated\n",
                   pcb_file
                   );
          return FALSE;
      }
      return FALSE;
  }

  for (list = extra_gnetlist_list; list; list = g_list_next (list)) {
    const gchar *s = (gchar *) list->data;
    const gchar *s2 = strstr (s, " -o ");
    gchar *out_file;
    gchar *backend;
    if (!s2) {
      out_file = g_strconcat (basename, ".", s, NULL);
      backend = g_strdup (s);
    } else {
      out_file = g_strdup (s2 + 4);
      backend = g_strndup (s, s2 - s);
    }

    if (!build_and_run_command ("%s %l -g %s -o %s %l %l",
				gnetlist,
				verboseList,
				backend,
				out_file,
				extra_gnetlist_arg_list,
				largs))
      return FALSE;
    g_free (out_file);
    g_free (backend);
  }

  g_list_free (args1);
  g_list_free (verboseList);

  return TRUE;
}

static gchar *
token (gchar * string, gchar ** next, gboolean * quoted_ret, gboolean parenth)
{
  static gchar *str;
  gchar *s, *ret;
  gboolean quoted = FALSE;

  if (string)
    str = string;
  if (!str || !*str) {
    if (next)
      *next = str;
    return g_strdup ("");
  }
  while (*str == ' ' || *str == '\t' || *str == ',' || *str == '\n')
    ++str;

  if (*str == '"') {
    quoted = TRUE;
    if (quoted_ret)
      *quoted_ret = TRUE;
    ++str;
    for (s = str; *s && *s != '"' && *s != '\n'; ++s);
  } else {
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
  ret = g_strndup (str, s - str);
  str = (quoted && *s) ? s + 1 : s;
  if (next)
    *next = str;
  return ret;
}

static gchar *
fix_spaces (gchar * str)
{
  gchar *s;

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
PcbElement *
pcb_element_line_parse (gchar * line)
{
  PcbElement *el = NULL;
  gchar *s, *t, close_char;
  gint state = 0, elcount = 0;

  if (strncmp (line, "Element", 7))
    return NULL;

  el = g_new0 (PcbElement, 1);

  s = line + 7;
  while (*s == ' ' || *s == '\t')
    ++s;

  if (*s == '[')
    el->hi_res_format = TRUE;
  else if (*s != '(') {
    g_free (el);
    return NULL;
  }

  el->res_char = el->hi_res_format ? '[' : '(';
  close_char = el->hi_res_format ? ']' : ')';

  el->flags = token (s + 1, NULL, &el->quoted_flags, 0);
  el->description = token (NULL, NULL, NULL, 0);
  el->refdes = token (NULL, NULL, NULL, 0);
  el->value = token (NULL, NULL, NULL, 0);

  el->x = token (NULL, NULL, NULL, 0);
  el->y = token (NULL, &t, NULL, 0);

  el->tail = g_strdup (t ? t : "");
  if ((s = strrchr (el->tail, (gint) '\n')) != NULL)
    *s = '\0';

  /* Count the tokens in tail to decide if it's new or old format.
   * Old format will have 3 tokens, new format will have 5 tokens.
   */
  for (s = el->tail; *s && *s != close_char; ++s) {
    if (*s != ' ') {
      if (state == 0)
        ++elcount;
      state = 1;
    } else
      state = 0;
  }
  el->nonetlist = 0;
  if (elcount > 4) {
    el->new_format = TRUE;
    if (strstr(el->tail, "nonetlist") != NULL)
      el->nonetlist = 1;
  }

  fix_spaces (el->description);
  fix_spaces (el->refdes);
  fix_spaces (el->value);

  /* Don't allow elements with no refdes to ever be deleted because
   * they may be desired pc board elements not in schematics.  So
   * initialize still_exists to TRUE if empty or non-alphanumeric
   * refdes.
   */
  if (!*el->refdes || !isalnum ((gint) (*el->refdes)))
    el->still_exists = TRUE;

  return el;
}

static void
pcb_element_free (PcbElement * el)
{
  if (!el)
    return;
  g_free (el->flags);
  g_free (el->description);
  g_free (el->changed_description);
  g_free (el->changed_value);
  g_free (el->refdes);
  g_free (el->value);
  g_free (el->x);
  g_free (el->y);
  g_free (el->tail);
  g_free (el->pkg_name_fix);
  g_free (el);
}

static void
get_pcb_element_list (gchar * pcb_file)
{
  FILE *f;
  PcbElement *el;
  gchar *s, buf[1024];

  if ((f = fopen (pcb_file, "r")) == NULL)
    return;
  while ((fgets (buf, sizeof (buf), f)) != NULL) {
    for (s = buf; *s == ' ' || *s == '\t'; ++s);
    if (!strncmp (s, "PKG_", 4)) {
      need_PKG_purge = TRUE;
      continue;
    }
    if ((el = pcb_element_line_parse (s)) == NULL)
      continue;
    pcb_element_list = g_list_append (pcb_element_list, el);
  }
  fclose (f);
}

static PcbElement *
pcb_element_exists (PcbElement * el_test, gboolean record)
{
  GList *list;
  PcbElement *el;

  for (list = pcb_element_list; list; list = g_list_next (list)) {
    el = (PcbElement *) list->data;

    if (strcmp (el_test->refdes, el->refdes))
      continue;
    if (strcmp (el_test->description, el->description)) { /* footprint */
      if (record)
        el->changed_description = g_strdup (el_test->description);
    } else {
      if (record) {
        if (strcmp (el_test->value, el->value))
          el->changed_value = g_strdup (el_test->value);
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
static void
simple_translate (PcbElement * el)
{

  el->x=strdup("0");
  el->y=strdup("0");
}

static gboolean
insert_element (FILE * f_out, gchar * element_file,
                gchar * footprint, gchar * refdes, gchar * value)
{
  FILE *f_in;
  PcbElement *el;
  gchar *fmt, *s, buf[1024];
  gboolean retval = FALSE;

  if ((f_in = fopen (element_file, "r")) == NULL) {
    s = g_strdup_printf ("insert_element() can't open %s", element_file);
    perror (s);
    g_free (s);
    return FALSE;
  }
  /* Scan the file to detect whether it's actually a PCB
   * layout. Assumes that a PCB layout will have a "PCB" line. */
  while ((fgets (buf, sizeof (buf), f_in)) != NULL) {
    for (s = buf; *s == ' ' || *s == '\t'; ++s);
    s[3] = 0;                   /* Truncate line */
    if (strncmp ("PCB", s, sizeof (buf)) == 0) {
      printf ("Warning: %s appears to be a PCB layout file. Skipping.\n",
              element_file);
      fclose (f_in);
      return FALSE;
    }
  }
  rewind (f_in);

  /* Copy the file element lines.  Substitute new parameters into the
   * Element() or Element[] line and strip comments.
   */
  while ((fgets (buf, sizeof (buf), f_in)) != NULL) {
    for (s = buf; *s == ' ' || *s == '\t'; ++s);
    if ((el = pcb_element_line_parse (s)) != NULL) {
      simple_translate (el);
      fmt = el->quoted_flags ?
        "Element%c\"%s\" \"%s\" \"%s\" \"%s\" %s %s%s\n" :
        "Element%c%s \"%s\" \"%s\" \"%s\" %s %s%s\n";

      fprintf (f_out, fmt,
               el->res_char, el->flags, footprint, refdes, value,
               el->x, el->y, el->tail);
      retval = TRUE;
    } else if (*s != '#')
      fputs (buf, f_out);
    pcb_element_free (el);
  }
  fclose (f_in);
  return retval;
}


gchar *
search_element (PcbElement * el)
{
  GList *list;
  gchar *s, *elname = NULL, *dir_path, *path = NULL;
  gint n1, n2;

  if (!elname)
    elname = g_strdup (el->description);

  if (!strcmp (elname, "unknown")) {
    g_free (elname);
    return NULL;
  }
  if (verbose > 1)
    printf ("\tSearching directories looking for file element: %s\n", elname);
  g_free (elname);
  return path;
}

/* The gnetlist backend gnet-gsch2pcb-rnd.scm generates PKG lines:
 *
 *        PKG(footprint,refdes,value)
 *
 */
static PcbElement *
pkg_to_element (FILE * f, gchar * pkg_line)
{
  PcbElement *el;
  gchar *s, *end, *refdes, *fp, *value;
  gint n, n_extra_args, n_dashes;

/*fprintf(stderr, "--- %s\n", pkg_line);*/

  if (strncmp (pkg_line, "PKG", 3)
      || (s = strchr (pkg_line, (gint) '(')) == NULL)
    return NULL;

/* remove trailing ")" */
  end = s+strlen(s)-2;
  if (*end == ')') *end = '\0';

/* tokenize the line keeping () */
  fp = token(s+1, NULL, NULL, 1);
  refdes = token(NULL, NULL, NULL, 1);
  value = token(NULL, NULL, NULL, 1);


/*fprintf(stderr, "refdes: %s\n", refdes);
fprintf(stderr, "    fp: %s\n", fp);
fprintf(stderr, "   val: %s\n", value);*/


  if (!refdes || !fp || !value) {
    fprintf (stderr, "Bad package line: %s\n", pkg_line);
    return NULL;
  }

  fix_spaces (refdes);
  fix_spaces (value);

  el = g_new0 (PcbElement, 1);
  el->description = g_strdup (fp);
  el->refdes = g_strdup (refdes);
  el->value = g_strdup (value);

// wtf?
//  if ((s = strchr (el->value, (gint) ')')) != NULL)
//    *s = '\0';

  if (empty_footprint_name && !strcmp (el->description, empty_footprint_name)) {
    if (verbose)
      printf
        ("%s: has the empty footprint attribute \"%s\" so won't be in the layout.\n",
         el->refdes, el->description);
    n_empty += 1;
    el->omit_PKG = TRUE;
  } else if (!strcmp (el->description, "none")) {
    fprintf (stderr,
             "WARNING: %s has a footprint attribute \"%s\" so won't be in the layout.\n",
             el->refdes, el->description);
    n_none += 1;
    el->omit_PKG = TRUE;
  } else if (!strcmp (el->description, "unknown")) {
    fprintf (stderr,
             "WARNING: %s has no footprint attribute so won't be in the layout.\n",
             el->refdes);
    n_unknown += 1;
    el->omit_PKG = TRUE;
  }
  return el;
}

/* Process the newly created pcb file which is the output from
 *     gnetlist -g gsch2pcb-rnd ...
 *
 * Insert elements for PKG_ lines if they be found by external element query.
 * If there was an existing pcb file, strip out any elements if they are
 * already present so that the new pcb file will only have new elements.
 */
static gint
add_elements (gchar * pcb_file)
{
  FILE *f_in, *f_out;
  PcbElement *el = NULL;
  gchar *command, *p, *tmp_file, *s, buf[1024];
  gint total, paren_level = 0;
  gboolean skipping = FALSE;

  if ((f_in = fopen (pcb_file, "r")) == NULL)
    return 0;
  tmp_file = g_strconcat (pcb_file, ".tmp", NULL);
  if ((f_out = fopen (tmp_file, "wb")) == NULL) {
    fclose (f_in);
    g_free (tmp_file);
    return 0;
  }
  while ((fgets (buf, sizeof (buf), f_in)) != NULL) {
    for (s = buf; *s == ' ' || *s == '\t'; ++s);
    if (skipping) {
      if (*s == '(')
        ++paren_level;
      else if (*s == ')' && --paren_level <= 0)
        skipping = FALSE;
      continue;
    }
    el = pkg_to_element (f_out, s);
    if (el && pcb_element_exists (el, TRUE)) {
      pcb_element_free (el);
      continue;
    }
    if (!el || el->omit_PKG) {
      if (el) {

      } else
        fputs (buf, f_out);
      continue;
    }

/* TODO */
    {
      if (verbose)
        printf ("%s: need new element for footprint  %s (value=%s)\n",
                el->refdes, el->description, el->value);

      p = search_element (el);
      if (!p && verbose)
        printf ("\tNo file element found.\n");

      if (p && insert_element (f_out, p,
                               el->description, el->refdes, el->value)) {
        ++n_added_ef;
        if (verbose)
          printf ("%s: added new file element for footprint %s (value=%s)\n",
                  el->refdes, el->description, el->value);
      } else {
        fprintf (stderr,
                 "%s: can't find PCB element for footprint %s (value=%s)\n",
                 el->refdes, el->description, el->value);
        if (remove_unfound_elements && !fix_elements) {
          fprintf (stderr,
                   "So device %s will not be in the layout.\n", el->refdes);
          ++n_PKG_removed_new;
        } else {
          ++n_not_found;
          fputs (buf, f_out);   /* Copy PKG_ line */
        }
      }
      g_free (p);
    }

    pcb_element_free (el);
    if (verbose)
      printf ("----\n");
  }
  fclose (f_in);
  fclose (f_out);

  total = n_added_ef + n_not_found;
  if (total == 0)
    build_and_run_command ("rm %s", tmp_file);
  else
    build_and_run_command ("mv %s %s", tmp_file, pcb_file);
  g_free (tmp_file);
  return total;
}

static void
update_element_descriptions (gchar * pcb_file, gchar * bak)
{
  FILE *f_in, *f_out;
  GList *list;
  PcbElement *el, *el_exists;
  gchar *fmt, *command, *tmp, *s, buf[1024];

  for (list = pcb_element_list; list; list = g_list_next (list)) {
    el = (PcbElement *) list->data;
    if (el->changed_description)
      ++n_fixed;
  }
  if (!pcb_element_list || n_fixed == 0) {
    fprintf (stderr, "Could not find any elements to fix.\n");
    return;
  }
  if ((f_in = fopen (pcb_file, "r")) == NULL)
    return;
  tmp = g_strconcat (pcb_file, ".tmp", NULL);
  if ((f_out = fopen (tmp, "wb")) == NULL) {
    fclose (f_in);
    return;
  }
  while ((fgets (buf, sizeof (buf), f_in)) != NULL) {
    for (s = buf; *s == ' ' || *s == '\t'; ++s);
    if ((el = pcb_element_line_parse (s)) != NULL
        && (el_exists = pcb_element_exists (el, FALSE)) != NULL
        && el_exists->changed_description) {
      fmt = el->quoted_flags ?
        "Element%c\"%s\" \"%s\" \"%s\" \"%s\" %s %s%s\n" :
        "Element%c%s \"%s\" \"%s\" \"%s\" %s %s%s\n";
      fprintf (f_out, fmt,
               el->res_char,
               el->flags, el_exists->changed_description,
               el->refdes, el->value, el->x, el->y, el->tail);
      printf ("%s: updating element Description: %s -> %s\n",
              el->refdes, el->description, el_exists->changed_description);
      el_exists->still_exists = TRUE;
    } else
      fputs (buf, f_out);
    pcb_element_free (el);
  }
  fclose (f_in);
  fclose (f_out);

  if (!bak_done) {
    build_and_run_command ("mv %s %s", pcb_file, bak);
    bak_done = TRUE;
  }

  build_and_run_command ("mv %s %s", tmp, pcb_file);
  g_free (tmp);
}

static void
prune_elements (gchar * pcb_file, gchar * bak)
{
  FILE *f_in, *f_out;
  GList *list;
  PcbElement *el, *el_exists;
  gchar *fmt, *command, *tmp, *s, buf[1024];
  gint paren_level = 0;
  gboolean skipping = FALSE;

  for (list = pcb_element_list; list; list = g_list_next (list)) {
    el = (PcbElement *) list->data;
    if (!el->still_exists) {
      if ((preserve) || (el->nonetlist)) {
        ++n_preserved;
        if (verbose > 1)
          fprintf (stderr,
                 "Preserving PCB element not in the schematic:    %s (element   %s) %s\n",
                 el->refdes, el->description, el->nonetlist ? "nonetlist" : "");
      } else
        ++n_deleted;
    } else if (el->changed_value)
      ++n_changed_value;
  }
  if (!pcb_element_list
      || (n_deleted == 0 && !need_PKG_purge && n_changed_value == 0)
    )
    return;
  if ((f_in = fopen (pcb_file, "r")) == NULL)
    return;
  tmp = g_strconcat (pcb_file, ".tmp", NULL);
  if ((f_out = fopen (tmp, "wb")) == NULL) {
    fclose (f_in);
    return;
  }
  while ((fgets (buf, sizeof (buf), f_in)) != NULL) {
    for (s = buf; *s == ' ' || *s == '\t'; ++s);
    if (skipping) {
      if (*s == '(')
        ++paren_level;
      else if (*s == ')' && --paren_level <= 0)
        skipping = FALSE;
      continue;
    }
    el_exists = NULL;
    if ((el = pcb_element_line_parse (s)) != NULL
        && (el_exists = pcb_element_exists (el, FALSE)) != NULL
        && !el_exists->still_exists && !preserve && !el->nonetlist) {
      skipping = TRUE;
      if (verbose)
        printf ("%s: deleted element %s (value=%s)\n",
                el->refdes, el->description, el->value);
      pcb_element_free (el);
      continue;
    }
    if (el_exists && el_exists->changed_value) {
      fmt = el->quoted_flags ?
        "Element%c\"%s\" \"%s\" \"%s\" \"%s\" %s %s%s\n" :
        "Element%c%s \"%s\" \"%s\" \"%s\" %s %s%s\n";
      fprintf (f_out, fmt,
               el->res_char, el->flags, el->description, el->refdes,
               el_exists->changed_value, el->x, el->y, el->tail);
      if (verbose)
        printf ("%s: changed element %s value: %s -> %s\n",
                el->refdes, el->description,
                el->value, el_exists->changed_value);
    } else if (!strncmp (s, "PKG_", 4))
      ++n_PKG_removed_old;
    else
      fputs (buf, f_out);
    pcb_element_free (el);
  }
  fclose (f_in);
  fclose (f_out);

  if (!bak_done) {
    build_and_run_command ("mv %s %s", pcb_file, bak);
    bak_done = TRUE;
  }

  build_and_run_command ("mv %s %s", tmp, pcb_file);
  g_free (tmp);
}

static gchar *
expand_dir (gchar * dir)
{
  gchar *s;

  if (*dir == '~')
    s = g_build_filename ((gchar *) g_get_home_dir (), dir + 1, NULL);
  else
    s = g_strdup (dir);
  return s;
}

static void
add_schematic (gchar * sch)
{
  const gchar* s;
  schematics = g_list_append (schematics, g_strdup (sch));
  if (!sch_basename && (s = g_strrstr (sch, ".sch")) != NULL && strlen(s) == 4)
    sch_basename = g_strndup (sch, s - sch);
}

static void
add_multiple_schematics (gchar * sch)
{
  /* parse the string using shell semantics */
  gint count;
  gchar** args = NULL;
  GError* error = NULL;

  if (g_shell_parse_argv (sch, &count, &args, &error)) {
    int i;
    for (i = 0; i < count; ++i)
    {
      add_schematic (args[i]);
    }
    g_strfreev (args);
  } else {
    fprintf (stderr,
             "invalid `schematics' option: %s\n",
             error->message);
    g_error_free (error);
  }
}

static gint
parse_config (gchar * config, gchar * arg)
{
  gchar *s;

  /* remove trailing white space otherwise strange things can happen */
  if ((arg != NULL) && (strlen (arg) >= 1)) {
    s = arg + strlen (arg) - 1;
    while ((*s == ' ' || *s == '\t') && (s != arg))
      s--;
    s++;
    *s = '\0';
  }
  if (verbose)
    printf ("    %s \"%s\"\n", config, arg ? arg : "");

  if (!strcmp (config, "remove-unfound") || !strcmp (config, "r")) {
    /* This is default behavior set in header section */
    remove_unfound_elements = TRUE;
    return 0;
  }
  if (!strcmp (config, "keep-unfound") || !strcmp (config, "k")) {
    remove_unfound_elements = FALSE;
    return 0;
  }
  if (!strcmp (config, "quiet") || !strcmp (config, "q")) {
    quiet_mode = TRUE;
    return 0;
  }
  if (!strcmp (config, "preserve") || !strcmp (config, "p")) {
    preserve = TRUE;
    return 0;
  }
  if (!strcmp (config, "elements-dir") || !strcmp (config, "d")) {
/* TODO */
  } else if (!strcmp (config, "output-name") || !strcmp (config, "o"))
    sch_basename = g_strdup (arg);
  else if (!strcmp (config, "schematics"))
    add_multiple_schematics (arg);
  else if (!strcmp (config, "gnetlist"))
    extra_gnetlist_list = g_list_append (extra_gnetlist_list, g_strdup (arg));
  else if (!strcmp (config, "empty-footprint"))
    empty_footprint_name = g_strdup (arg);
  else
    return -1;

  return 1;
}

static void
load_project (gchar * path)
{
  FILE *f;
  gchar *s, buf[1024], config[32], arg[768];

  f = fopen (path, "r");
  if (!f)
    return;
  if (verbose)
    printf ("Reading project file: %s\n", path);
  while (fgets (buf, sizeof (buf), f)) {
    for (s = buf; *s == ' ' || *s == '\t' || *s == '\n'; ++s);
    if (!*s || *s == '#' || *s == '/' || *s == ';')
      continue;
    arg[0] = '\0';
    sscanf (s, "%31s %767[^\n]", config, arg);
    parse_config (config, arg);
  }
  fclose (f);
}

static void
load_extra_project_files (void)
{
  gchar *path;
  static gboolean done = FALSE;

  if (done)
    return;

  load_project ("/etc/gsch2pcb");
  load_project ("/usr/local/etc/gsch2pcb");

  path = g_build_filename ((gchar *) g_get_home_dir (), ".gEDA",
                           "gsch2pcb", NULL);
  load_project (path);
  g_free (path);

  done = TRUE;
}

static gchar *usage_string0 =
  "usage: gsch2pcb [options] {project | foo.sch [foo1.sch ...]}\n"
  "\n"
  "Generate a PCB layout file from a set of gschem schematics.\n"
  "   gnetlist -g PCB is run to generate foo.net from the schematics.\n"
  "\n"
/* TODO */
/*  "   gnetlist -g gsch2pcb is run to get PCB elements which\n"
  "   match schematic footprints.  For schematic footprints which don't match\n"
  "   any PCB layout elements, search a set of file element directories in\n"
  "   an attempt to find matching PCB file elements.\n"*/
  "   Output to foo.pcb if it doesn't exist.  If there is a current foo.pcb,\n"
  "   output only new elements to foo.new.pcb.\n"
  "   If any elements with a non-empty element name in the current foo.pcb\n"
  "   have no matching schematic component, then remove those elements from\n"
  "   foo.pcb and rename foo.pcb to a foo.pcb.bak sequence.\n"
  "\n"
  "   gnetlist -g pcbpins is run to get a PCB actions file which will rename all\n"
  "   of the pins in a .pcb file to match pin names from the schematic.\n"
  "\n"
  "   \"project\" is a file (not ending in .sch) containing a list of\n"
  "   schematics to process and some options.  A schematics line is like:\n"
  "       schematics foo1.sch foo2.sch ...\n"
  "   Options in a project file are like command line args without the \"-\":\n"
  "       output-name myproject\n"
  "\n"
  "options (may be included in a project file):\n"
  "   -d, --elements-dir D  Search D for PCB file elements.  These defaults\n"
  "                         are searched if they exist: ./packages,\n"
  "                         /usr/local/share/pcb/newlib, /usr/share/pcb/newlib,\n"
  "                         (old pcb) /usr/local/lib/pcb_lib, /usr/lib/pcb_lib,\n"
  "                         (old pcb) /usr/local/pcb_lib\n"
  "   -o, --output-name N   Use output file names N.net, N.pcb, and N.new.pcb\n"
  "                         instead of foo.net, ... where foo is the basename\n"
  "                         of the first command line .sch file.\n"
  "   -r, --remove-unfound  Don't include references to unfound elements in\n"
  "                         the generated .pcb files.  Use if you want PCB to\n"
  "                         be able to load the (incomplete) .pcb file.\n"
  "                         This is the default behavior.\n"
  "   -k, --keep-unfound    Keep include references to unfound elements in\n"
  "                         the generated .pcb files.  Use if you want to hand\n"
  "                         edit or otherwise preprocess the generated .pcb file\n"
  "                         before running pcb.\n"
  "   -p, --preserve        Preserve elements in PCB files which are not found\n"
  "                         in the schematics.  Note that elements with an empty\n"
  "                         element name (schematic refdes) are never deleted,\n"
  "                         so you really shouldn't need this option.\n"
  "   -q, --quiet           Don't tell the user what to do next after running gsch2pcb.\n"
  "\n";

static gchar *usage_string1 =
  "   --gnetlist backend    A convenience run of extra gnetlist -g commands.\n"
  "                         Example:  gnetlist partslist3\n"
  "                         Creates:  myproject.partslist3\n"
  " --empty-footprint name  See the project.sample file.\n"
  "\n"
  "options (not recognized in a project file):\n"
  "   --gnetlist-arg arg    Allows additional arguments to be passed to gnetlist.\n"
  "       --fix-elements    If a schematic component footprint is not equal\n"
  "                         to its PCB element Description, update the\n"
  "                         Description instead of replacing the element.\n"
  "                         Do this the first time gsch2pcb is used with\n"
  "                         PCB files originally created with gschem2pcb.\n"
  "   -v, --verbose         Use -v -v for additional file element debugging.\n"
  "   -V, --version\n\n"
  "environment variables:\n"
  "   GNETLIST              If set, this specifies the name of the gnetlist program\n"
  "                         to execute.\n"
  "\n"
  "Additional Resources:\n"
  "\n"
  "  gnetlist user guide:  http://wiki.geda-project.org/geda:gnetlist_ug\n"
  "  gEDA homepage:        http://www.geda-project.org\n"
  "  PCB homepage:         http://pcb.geda-project.org\n"  "\n";

static void
usage ()
{
  puts (usage_string0);
  puts (usage_string1);
  exit (0);
}

static void
get_args (gint argc, gchar ** argv)
{
  gchar *opt, *arg, *s;
  gint i, r;

  for (i = 1; i < argc; ++i) {
    opt = argv[i];
    arg = argv[i + 1];
    if (*opt == '-') {
      ++opt;
      if (*opt == '-')
        ++opt;
      if (!strcmp (opt, "version") || !strcmp (opt, "V")) {
        printf ("gsch2pcb %s\n", GSC2PCB_VERSION);
        exit (0);
      } else if (!strcmp (opt, "verbose") || !strcmp (opt, "v")) {
        verbose += 1;
        continue;
      } else if (!strcmp (opt, "fix-elements")) {
        fix_elements = TRUE;
        continue;
      } else if (!strcmp (opt, "gnetlist-arg")) {
        extra_gnetlist_arg_list =
          g_list_append (extra_gnetlist_arg_list, g_strdup (arg));
        i++;
        continue;
      } else if (!strcmp (opt, "help") || !strcmp (opt, "h"))
        usage ();
      else if (i < argc
               && ((r = parse_config (opt, (i < argc - 1) ? arg : NULL))
                   >= 0)
        ) {
        i += r;
        continue;
      }
      printf ("gsch2pcb: bad or incomplete arg: %s\n", argv[i]);
      usage ();
    } else {
      if (!g_str_has_suffix (argv[i], ".sch")) {
        load_extra_project_files ();
        load_project (argv[i]);
      } else
        add_schematic (argv[i]);
    }
  }
}

gint
main (gint argc, gchar ** argv)
{
  gchar *pcb_file_name,
    *pcb_new_file_name, *bak_file_name, *pins_file_name, *net_file_name, *tmp;
  gint i;
  gboolean initial_pcb = TRUE;
  gboolean created_pcb_file = TRUE;
  char *path, *p;

  if (argc < 2)
    usage ();

  get_args (argc, argv);

  load_extra_project_files ();

  if (!schematics)
    usage ();

  pins_file_name = g_strconcat (sch_basename, ".cmd", NULL);
  net_file_name = g_strconcat (sch_basename, ".net", NULL);
  pcb_file_name = g_strconcat (sch_basename, ".pcb", NULL);
  bak_file_name = g_strconcat (sch_basename, ".pcb.bak", NULL);
  tmp = g_strdup (bak_file_name);

  for (i = 0; g_file_test (bak_file_name, G_FILE_TEST_EXISTS); ++i) {
    g_free (bak_file_name);
    bak_file_name = g_strdup_printf ("%s%d", tmp, i);
  }
  g_free (tmp);

  if (g_file_test (pcb_file_name, G_FILE_TEST_EXISTS)) {
    initial_pcb = FALSE;
    pcb_new_file_name = g_strconcat (sch_basename, ".new.pcb", NULL);
    get_pcb_element_list (pcb_file_name);
  } else
    pcb_new_file_name = g_strdup (pcb_file_name);

  if (!run_gnetlist (pins_file_name, net_file_name, pcb_new_file_name,
		     sch_basename, schematics)) {
    fprintf(stderr, "Failed to run gnetlist\n");
    exit (1);
  }

  if (add_elements (pcb_new_file_name) == 0) {
    build_and_run_command ("rm %s", pcb_new_file_name);
    if (initial_pcb) {
      printf ("No elements found, so nothing to do.\n");
      exit (0);
    }
  }

  if (fix_elements)
    update_element_descriptions (pcb_file_name, bak_file_name);
  prune_elements (pcb_file_name, bak_file_name);

  /* Report work done during processing */
  if (verbose)
    printf ("\n");
  printf ("\n----------------------------------\n");
  printf ("Done processing.  Work performed:\n");
  if (n_deleted > 0 || n_fixed > 0 || need_PKG_purge || n_changed_value > 0)
    printf ("%s is backed up as %s.\n", pcb_file_name, bak_file_name);
  if (pcb_element_list && n_deleted > 0)
    printf ("%d elements deleted from %s.\n", n_deleted, pcb_file_name);

  if (n_added_ef > 0)
    printf ("%d file elements added to %s.\n",
            n_added_ef, pcb_new_file_name);
  else if (n_not_found == 0) {
    printf ("No elements to add so not creating %s\n", pcb_new_file_name);
    created_pcb_file = FALSE;
  }

  if (n_not_found > 0) {
    printf ("%d not found elements added to %s.\n",
            n_not_found, pcb_new_file_name);
  }
  if (n_unknown > 0)
    printf ("%d components had no footprint attribute and are omitted.\n",
            n_unknown);
  if (n_none > 0)
    printf ("%d components with footprint \"none\" omitted from %s.\n",
            n_none, pcb_new_file_name);
  if (n_empty > 0)
    printf ("%d components with empty footprint \"%s\" omitted from %s.\n",
            n_empty, empty_footprint_name, pcb_new_file_name);
  if (n_changed_value > 0)
    printf ("%d elements had a value change in %s.\n",
            n_changed_value, pcb_file_name);
  if (n_fixed > 0)
    printf ("%d elements fixed in %s.\n", n_fixed, pcb_file_name);
  if (n_PKG_removed_old > 0) {
    printf ("%d elements could not be found.", n_PKG_removed_old);
    if (created_pcb_file)
      printf ("  So %s is incomplete.\n", pcb_file_name);
    else
      printf ("\n");
  }
  if (n_PKG_removed_new > 0) {
    printf ("%d elements could not be found.", n_PKG_removed_new);
    if (created_pcb_file)
      printf ("  So %s is incomplete.\n", pcb_new_file_name);
    else
      printf ("\n");
  }
  if (n_preserved > 0)
    printf ("%d elements not in the schematic preserved in %s.\n",
            n_preserved, pcb_file_name);

  /* Tell user what to do next */
  if (verbose)
    printf ("\n");

  if (n_added_ef > 0) {
    if (initial_pcb) {
      printf ("\nNext step:\n");
      printf ("1.  Run pcb on your file %s.\n", pcb_file_name);
      printf
        ("    You will find all your footprints in a bundle ready for you to place\n");
      printf
        ("    or disperse with \"Select -> Disperse all elements\" in PCB.\n\n");
      printf
        ("2.  From within PCB, select \"File -> Load netlist file\" and select \n");
      printf ("    %s to load the netlist.\n\n", net_file_name);
      printf ("3.  From within PCB, enter\n\n");
      printf ("           :ExecuteFile(%s)\n\n", pins_file_name);
      printf
        ("    to propagate the pin names of all footprints to the layout.\n\n");
    } else if (quiet_mode == FALSE) {
      printf ("\nNext steps:\n");
      printf ("1.  Run pcb on your file %s.\n", pcb_file_name);
      printf
        ("2.  From within PCB, select \"File -> Load layout data to paste buffer\"\n");
      printf
        ("    and select %s to load the new footprints into your existing layout.\n",
         pcb_new_file_name);
      printf
        ("3.  From within PCB, select \"File -> Load netlist file\" and select \n");
      printf ("    %s to load the updated netlist.\n\n", net_file_name);
      printf ("4.  From within PCB, enter\n\n");
      printf ("           :ExecuteFile(%s)\n\n", pins_file_name);
      printf ("    to update the pin names of all footprints.\n\n");
    }
  }

  g_free (net_file_name);
  g_free (pins_file_name);
  g_free (pcb_file_name);
  g_free (bak_file_name);

  return 0;
}
