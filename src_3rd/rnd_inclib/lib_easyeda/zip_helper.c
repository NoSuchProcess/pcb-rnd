/*
    easyeda file format parser - helpers for the pro format
    Copyright (C) 2024 Tibor 'Igor2' Palinkas

    (Supported by NLnet NGI0 Entrust Fund in 2024)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Lazy heuristics to figure if a file looks like a zip; this is not
   100% correct: there's no guarantee the file starts with a PK block,
   but one from the end would need to be picked (but that may have a
   max 64k comment so it would need to be searched backward... */
static int easypro_is_file_zip(FILE *f)
{
	char buf[4];
	if (fread(buf, 1, 4, f) != 4)
		return 0;
	return (buf[0] == 'P') && (buf[1] == 'K') && (buf[2] == 3) && (buf[3] == 4);
}

/* allocate memory and print a zip command line: first prefix (if not NULL),
   then template with %s substituted wuth path */
static char *easypro_zip_cmd(const char **prefix, const char *template, const char *path)
{
	gds_t tmp = {0};
	const char *s;

	if (prefix != NULL) {
		const char **p;
		for(p = prefix; *p != NULL; p++)
			gds_append_str(&tmp, *p);
	}

	for(s = template; *s != '\0'; s++) {
		if ((s[0] == '%') && (s[1] == 's')) {
			gds_append_str(&tmp, path);
			s++;
		}
		else
			gds_append(&tmp, *s);
	}

	return tmp.array;
}
