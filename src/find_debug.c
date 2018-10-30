/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* find-related debug functions */

/* writes the several names of a subcircuit to a file */
static void PrintElementNameList(pcb_subc_t *subc, FILE * FP)
{
	fputc('(', FP);
	pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&subc->Attributes, "footprint")));
	fputc(' ', FP);
	pcb_print_quoted_string(FP, (char *) PCB_EMPTY(subc->refdes));
	fputc(' ', FP);
	pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&subc->Attributes, "value")));
	fputc(')', FP);
	fputc('\n', FP);
}
static void pcb_print_conn_subc_name(pcb_subc_t *subc, FILE * FP)
{
	fputs("Element", FP);
	PrintElementNameList(subc, FP);
	fputs("{\n", FP);
}

/* ---------------------------------------------------------------------------
 * prints one {pin,pad,via}/element entry of connection lists
 */
static void pcb_print_conn_list_entry(char *ObjName, pcb_subc_t *subc, pcb_bool FirstOne, FILE * FP)
{
	if (FirstOne) {
		fputc('\t', FP);
		pcb_print_quoted_string(FP, ObjName);
		fprintf(FP, "\n\t{\n");
	}
	else {
		fprintf(FP, "\t\t");
		pcb_print_quoted_string(FP, ObjName);
		fputc(' ', FP);
		if (subc)
			PrintElementNameList(subc, FP);
		else
			fputs("(__VIA__)\n", FP);
	}
}
