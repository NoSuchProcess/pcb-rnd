/*				COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2017 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Eagle binary tree parser */

#include <stdio.h>
#include "egb_tree.h"

typedef enum pcb_eagle_binkw_s {
	PCB_EGKW_SECT_START = 0x1000, /*0x1080*/
	PCB_EGKW_SECT_UNKNOWN11 = 0x1100,
	PCB_EGKW_SECT_GRID = 0x1200,
	PCB_EGKW_SECT_LAYER = 0x1300, /*0x1380 */
	PCB_EGKW_SECT_SCHEMA = 0x1400,
	PCB_EGKW_SECT_LIBRARY = 0x1500, /*0x1580 */
	PCB_EGKW_SECT_DEVICES = 0x1700, /*0x1780 */
	PCB_EGKW_SECT_SYMBOLS = 0x1800, /*0x1880 */
	PCB_EGKW_SECT_PACKAGES = 0x1900, /*0x1980, 0x19a0 */
	PCB_EGKW_SECT_SCHEMASHEET = 0x1a00,
	PCB_EGKW_SECT_BOARD = 0x1b00, /*0x1b40, 0x1b80, 0x1b08 */
	PCB_EGKW_SECT_SIGNAL = 0x1c00, /*0x1c04, 0x1c40,0x1c48,0x1c08 */
	PCB_EGKW_SECT_SYMBOL = 0x1d00,
	PCB_EGKW_SECT_PACKAGE = 0x1e00, /*0x1e20*/
	PCB_EGKW_SECT_SCHEMANET = 0x1f00,
	PCB_EGKW_SECT_PATH = 0x2000,
	PCB_EGKW_SECT_POLYGON = 0x2100, /*0x2108 */
	PCB_EGKW_SECT_LINE = 0x2200, /*0x2280,0x228c,0x2288,0x2290,0x229c,0x22a0,0x22a8 */
	PCB_EGKW_SECT_ARC = 0x2400,
	PCB_EGKW_SECT_CIRCLE = 0x2500, /*0x2580, 0x25a0, 0x258c */
	PCB_EGKW_SECT_RECTANGLE = 0x2600,/*0x2680,0x26a0 */
	PCB_EGKW_SECT_JUNCTION = 0x2700,
	PCB_EGKW_SECT_HOLE = 0x2800, /*0x2880,0x28a0,0x288c */
	PCB_EGKW_SECT_VIA = 0x2900, /*0x2980 */
	PCB_EGKW_SECT_PAD = 0x2a00, /*0x2a80,0x2aa0 */
	PCB_EGKW_SECT_SMD = 0x2b00, /*0x2b80 */
	PCB_EGKW_SECT_PIN = 0x2c00, /*0x2c80 */
	PCB_EGKW_SECT_GATE = 0x2d00, /*0x2d80 */
	PCB_EGKW_SECT_ELEMENT = 0x2e00,/*0x2e20,0x2e80,0x2e0c,0x2e28 */
	PCB_EGKW_SECT_ELEMENT2 = 0x2f00,/*0x2f80,0x2fa0 */
	PCB_EGKW_SECT_INSTANCE = 0x3000,
	PCB_EGKW_SECT_TEXT = 0x3100,/*0x3180,0x31a0,0x318c */
	PCB_EGKW_SECT_NETBUSLABEL = 0x3300,
	PCB_EGKW_SECT_SMASHEDNAME = 0x3400, /*0x3480,0x348c,0x3488 */
	PCB_EGKW_SECT_SMASHEDVALUE = 0x3500,/*0x3580,0x3588,0x358c */
	PCB_EGKW_SECT_PACKAGEVARIANT = 0x3600, /*0x3680 */
	PCB_EGKW_SECT_DEVICE = 0x3700, /*0x3780 */
	PCB_EGKW_SECT_PART = 0x3800,
	PCB_EGKW_SECT_SCHEMABUS = 0x3a00,
	PCB_EGKW_SECT_VARIANTCONNECTIONS = 0x3c00, /*0x3c80 */
	PCB_EGKW_SECT_SCHEMACONNECTION = 0x3d00,
	PCB_EGKW_SECT_CONTACTREF = 0x3e00, /*0x3e80,0x3ea8,0x3ea0,0x3e20 */	
	PCB_EGKW_SECT_SMASHEDPART = 0x3f00,
	PCB_EGKW_SECT_SMASHEDGATE = 0x4000,
	PCB_EGKW_SECT_ATTRIBUTE = 0x4100, /*0x4180 */
	PCB_EGKW_SECT_ATTRIBUTEVALUE = 0x4200,
	PCB_EGKW_SECT_FRAME = 0x4300,
	PCB_EGKW_SECT_SMASHEDXREF = 0x4400,
	PCB_EGKW_SECT_FREETEXT = 0x1312,

	/* virtual nodes added in postprocessing */
	PCB_EGKW_SECT_LAYERS = 0x11300,
	PCB_EGKW_SECT_DRC = 0x11100
} pcb_eagle_binkw_t;

int pcb_egle_bin_load(void *ctx, FILE *f, const char *fn, egb_node_t **root);


