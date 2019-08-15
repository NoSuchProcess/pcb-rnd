 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  (this file is based on PCB, interactive printed circuit board design)
  *  Copyright (C) 2006 Dan McMahill
  *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#define PHOTO_FLIP_X 1
#define PHOTO_FLIP_Y 2

static int photo_mode, photo_flip;
static gdImagePtr photo_copper[PCB_MAX_LAYER];
static gdImagePtr photo_silk, photo_mask, photo_drill, *photo_im;
static gdImagePtr photo_outline;
static int photo_groups[PCB_MAX_LAYERGRP + 2], photo_ngroups;
static int photo_has_inners;
static pcb_layergrp_id_t photo_last_grp;
static int is_photo_drill;

static const char *mask_colour_names[] = {
	"green",
	"red",
	"blue",
	"purple",
	"black",
	"white",
	NULL
};

/*  These values were arrived at through trial and error.
    One potential improvement (especially for white) is
    to use separate color_structs for the multiplication
    and addition parts of the mask math.
 */
static const color_struct mask_colours[] = {
#define MASK_COLOUR_GREEN 0
	{0x3CA03CFF, 60, 160, 60, 255},
#define MASK_COLOUR_RED 1
	{0x8C1919FF, 140, 25, 25, 255},
#define MASK_COLOUR_BLUE 2
	{0x3232A0FF, 50, 50, 160, 255},
#define MASK_COLOUR_PURPLE 3
	{0x3C1446FF, 60, 20, 70, 255},
#define MASK_COLOUR_BLACK 4
	{0x141414FF, 20, 20, 20, 255},
#define MASK_COLOUR_WHITE 5
	{0xA7E6A2FF, 167, 230, 162, 255}	/* <-- needs improvement over FR4 */
};

static const char *plating_type_names[] = {
#define PLATING_TIN 0
	"tinned",
#define PLATING_GOLD 1
	"gold",
#define PLATING_SILVER 2
	"silver",
#define PLATING_COPPER 3
	"copper",
	NULL
};

static const char *silk_colour_names[] = {
	"white",
	"black",
	"yellow",
	NULL
};

static const color_struct silk_colours[] = {
#define SILK_COLOUR_WHITE 0
	{0xE0E0E0FF, 224, 224, 224, 255},
#define SILK_COLOUR_BLACK 1
	{0x0E0E0EFF, 14, 14, 14, 255},
#define SILK_COLOUR_YELLOW 2
	{0xB9B90AFF, 185, 185, 10, 255}
};

static const color_struct silk_top_shadow = {0x151515FF, 21, 21, 21, 255};
static const color_struct silk_bottom_shadow = {0x0E0E0EFF, 14, 14, 14, 255};

static int smshadows[3][3] = {
	{1, 0, -1},
	{0, 0, -1},
	{-1, -1, -1},
};

static int shadows[5][5] = {
	{1, 1, 0, 0, -1},
	{1, 1, 1, -1, 0},
	{0, 1, 0, -1, 0},
	{0, -1, -1, -1, 0},
	{-1, 0, 0, 0, -1},
};

/* black and white are 0 and 1 */
#define TOP_SHADOW 2
#define BOTTOM_SHADOW 3

static void png_photo_as_shown();

