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

/*
 *  Heavily based on the ps HID written by DJ Delorie
 */


/* For photo-mode we need the following layers as monochrome masks:

   top soldermask
   top silk
   copper layers
   drill
*/

static void ts_bs(gdImagePtr im)
{
	int x, y, sx, sy, si;
	for (x = 0; x < gdImageSX(im); x++)
		for (y = 0; y < gdImageSY(im); y++) {
			si = 0;
			for (sx = -2; sx < 3; sx++)
				for (sy = -2; sy < 3; sy++)
					if (!gdImageGetPixel(im, x + sx, y + sy))
						si += shadows[sx + 2][sy + 2];
			if (gdImageGetPixel(im, x, y)) {
				if (si > 1)
					gdImageSetPixel(im, x, y, TOP_SHADOW);
				else if (si < -1)
					gdImageSetPixel(im, x, y, BOTTOM_SHADOW);
			}
		}
}

static void ts_bs_sm(gdImagePtr im)
{
	int x, y, sx, sy, si;
	for (x = 0; x < gdImageSX(im); x++)
		for (y = 0; y < gdImageSY(im); y++) {
			si = 0;
			for (sx = -1; sx < 2; sx++)
				for (sy = -1; sy < 2; sy++)
					if (!gdImageGetPixel(im, x + sx, y + sy))
						si += smshadows[sx + 1][sy + 1];
			if (gdImageGetPixel(im, x, y)) {
				if (si > 1)
					gdImageSetPixel(im, x, y, TOP_SHADOW);
				else if (si < -1)
					gdImageSetPixel(im, x, y, BOTTOM_SHADOW);
			}
		}
}

static void clip(color_struct * dest, color_struct * source)
{
#define CLIP(var) \
  dest->var = source->var;	\
  if (dest->var > 255) dest->var = 255;	\
  if (dest->var < 0)   dest->var = 0;

	CLIP(r);
	CLIP(g);
	CLIP(b);
#undef CLIP
}

static void blend(color_struct * dest, float a_amount, color_struct * a, color_struct * b)
{
	dest->r = a->r * a_amount + b->r * (1 - a_amount);
	dest->g = a->g * a_amount + b->g * (1 - a_amount);
	dest->b = a->b * a_amount + b->b * (1 - a_amount);
}

static void multiply(color_struct * dest, color_struct * a, color_struct * b)
{
	dest->r = (a->r * b->r) / 255;
	dest->g = (a->g * b->g) / 255;
	dest->b = (a->b * b->b) / 255;
}

static void add(color_struct * dest, double a_amount, const color_struct * a, double b_amount, const color_struct * b)
{
	dest->r = a->r * a_amount + b->r * b_amount;
	dest->g = a->g * a_amount + b->g * b_amount;
	dest->b = a->b * a_amount + b->b * b_amount;

	clip(dest, dest);
}

static void subtract(color_struct * dest, double a_amount, const color_struct * a, double b_amount, const color_struct * b)
{
	dest->r = a->r * a_amount - b->r * b_amount;
	dest->g = a->g * a_amount - b->g * b_amount;
	dest->b = a->b * a_amount - b->b * b_amount;

	clip(dest, dest);
}

static int png_set_layer_group_photo(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	/* workaround: the outline layer vs. alpha breaks if set twice and the draw
	   code may set it twice (if there's no mech layer), but always in a row */
	if (group == photo_last_grp)
		return 1;
	photo_last_grp = group;

	is_photo_drill = (PCB_LAYER_IS_DRILL(flags, purpi) || ((flags & PCB_LYT_MECH) && PCB_LAYER_IS_ROUTE(flags, purpi)));
		if (((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) && (flags & PCB_LYT_TOP)) {
			if (photo_flip)
				return 0;
			photo_im = &photo_silk;
		}
		else if (((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) && (flags & PCB_LYT_BOTTOM)) {
			if (!photo_flip)
				return 0;
			photo_im = &photo_silk;
		}
		else if ((flags & PCB_LYT_MASK) && (flags & PCB_LYT_TOP)) {
			if (photo_flip)
				return 0;
			photo_im = &photo_mask;
		}
		else if ((flags & PCB_LYT_MASK) && (flags & PCB_LYT_BOTTOM)) {
			if (!photo_flip)
				return 0;
			photo_im = &photo_mask;
		}
		else if (is_photo_drill) {
			photo_im = &photo_drill;
		}
		else {
			if (PCB_LAYER_IS_OUTLINE(flags, purpi)) {
				doing_outline = 1;
				have_outline = 0;
				photo_im = &photo_outline;
			}
			else if (flags & PCB_LYT_COPPER) {
				photo_im = photo_copper + group;
			}
			else
				return 0;
		}

		if (!*photo_im) {
			static color_struct *black = NULL, *white = NULL;
			*photo_im = gdImageCreate(gdImageSX(im), gdImageSY(im));
			if (photo_im == NULL) {
				pcb_message(PCB_MSG_ERROR, "png_set_layer():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", gdImageSX(im), gdImageSY(im));
				return 0;
			}


			white = (color_struct *) malloc(sizeof(color_struct));
			white->r = white->g = white->b = 255;
			white->a = 0;
			white->c = gdImageColorAllocate(*photo_im, white->r, white->g, white->b);
			if (white->c == BADC) {
				pcb_message(PCB_MSG_ERROR, "png_set_layer():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return 0;
			}

			black = (color_struct *) malloc(sizeof(color_struct));
			black->r = black->g = black->b = black->a = 0;
			black->c = gdImageColorAllocate(*photo_im, black->r, black->g, black->b);
			if (black->c == BADC) {
				pcb_message(PCB_MSG_ERROR, "png_set_layer(): gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return 0;
			}

			if (is_photo_drill)
				gdImageFilledRectangle(*photo_im, 0, 0, gdImageSX(im), gdImageSY(im), black->c);
		}
		im = *photo_im;
		return 1;
}

static void png_photo_head(void)
{
	photo_last_grp = -1;
}

static void png_photo_foot(void)
{
	int x, y, darken, lg;
	color_struct white, black, fr4;

	rgb(&white, 255, 255, 255);
	rgb(&black, 0, 0, 0);
	rgb(&fr4, 70, 70, 70);

	im = master_im;

	ts_bs(photo_copper[photo_groups[0]]);
	if (photo_silk != NULL)
		ts_bs(photo_silk);
	if (photo_mask != NULL)
		ts_bs_sm(photo_mask);

	if (photo_outline && have_outline) {
		int black = gdImageColorResolve(photo_outline, 0x00, 0x00, 0x00);

		/* go all the way around the image, trying to fill the outline */
		for (x = 0; x < gdImageSX(im); x++) {
			gdImageFillToBorder(photo_outline, x, 0, black, black);
			gdImageFillToBorder(photo_outline, x, gdImageSY(im) - 1, black, black);
		}
		for (y = 1; y < gdImageSY(im) - 1; y++) {
			gdImageFillToBorder(photo_outline, 0, y, black, black);
			gdImageFillToBorder(photo_outline, gdImageSX(im) - 1, y, black, black);

		}
	}


	for (x = 0; x < gdImageSX(im); x++) {
		for (y = 0; y < gdImageSY(im); y++) {
			color_struct p, cop;
			color_struct mask_colour, silk_colour;
			int cc, mask, silk;
			int transparent;

			if (photo_outline && have_outline) {
				transparent = gdImageGetPixel(photo_outline, x, y);
			}
			else {
				transparent = 0;
			}

			mask = photo_mask ? gdImageGetPixel(photo_mask, x, y) : 0;
			silk = photo_silk ? gdImageGetPixel(photo_silk, x, y) : 0;

			darken = 0;
			for(lg = 1; lg < photo_ngroups; lg++) {
				if (photo_copper[photo_groups[lg]] && gdImageGetPixel(photo_copper[photo_groups[lg]], x, y)) {
					darken = 1;
					break;
				}
			}

			if (darken)
				rgb(&cop, 40, 40, 40);
			else
				rgb(&cop, 100, 100, 110);

			blend(&cop, 0.3, &cop, &fr4);

			cc = gdImageGetPixel(photo_copper[photo_groups[0]], x, y);
			if (cc) {
				int r;

				if (mask)
					rgb(&cop, 220, 145, 230);
				else {

					if (png_options[HA_photo_plating].lng == PLATING_GOLD) {
						/* ENIG */
						rgb(&cop, 185, 146, 52);

						/* increase top shadow to increase shininess */
						if (cc == TOP_SHADOW)
							blend(&cop, 0.7, &cop, &white);
					}
					else if (png_options[HA_photo_plating].lng == PLATING_TIN) {
						/* tinned */
						rgb(&cop, 140, 150, 160);

						/* add some variation to make it look more matte */
						r = (rand() % 5 - 2) * 2;
						cop.r += r;
						cop.g += r;
						cop.b += r;
					}
					else if (png_options[HA_photo_plating].lng == PLATING_SILVER) {
						/* silver */
						rgb(&cop, 192, 192, 185);

						/* increase top shadow to increase shininess */
						if (cc == TOP_SHADOW)
							blend(&cop, 0.7, &cop, &white);
					}
					else if (png_options[HA_photo_plating].lng == PLATING_COPPER) {
						/* copper */
						rgb(&cop, 184, 115, 51);

						/* increase top shadow to increase shininess */
						if (cc == TOP_SHADOW)
							blend(&cop, 0.7, &cop, &white);
					}
					/*FIXME: old code...can be removed after validation.   rgb(&cop, 140, 150, 160);
					   r = (pcb_rand() % 5 - 2) * 2;
					   cop.r += r;
					   cop.g += r;
					   cop.b += r; */
				}

				if (cc == TOP_SHADOW) {
					cop.r = 255 - (255 - cop.r) * 0.7;
					cop.g = 255 - (255 - cop.g) * 0.7;
					cop.b = 255 - (255 - cop.b) * 0.7;
				}
				if (cc == BOTTOM_SHADOW) {
					cop.r *= 0.7;
					cop.g *= 0.7;
					cop.b *= 0.7;
				}
			}

			if (photo_drill && !gdImageGetPixel(photo_drill, x, y)) {
				rgb(&p, 0, 0, 0);
				transparent = 1;
			}
			else if (silk) {
				silk_colour = silk_colours[png_options[HA_photo_silk_colour].lng];
				blend(&p, 1.0, &silk_colour, &silk_colour);

				if (silk == TOP_SHADOW)
					add(&p, 1.0, &p, 1.0, &silk_top_shadow);
				else if (silk == BOTTOM_SHADOW)
					subtract(&p, 1.0, &p, 1.0, &silk_bottom_shadow);
			}
			else if (mask) {
				p = cop;
				mask_colour = mask_colours[png_options[HA_photo_mask_colour].lng];
				multiply(&p, &p, &mask_colour);
				add(&p, 1, &p, 0.2, &mask_colour);
				if (mask == TOP_SHADOW)
					blend(&p, 0.7, &p, &white);
				if (mask == BOTTOM_SHADOW)
					blend(&p, 0.7, &p, &black);
			}
			else
				p = cop;

			if (png_options[HA_use_alpha].lng)
				cc = (transparent) ? gdImageColorResolveAlpha(im, 0, 0, 0, 127) : gdImageColorResolveAlpha(im, p.r, p.g, p.b, 0);
			else
				cc = (transparent) ? gdImageColorResolve(im, 0, 0, 0) : gdImageColorResolve(im, p.r, p.g, p.b);

			if (photo_flip == PHOTO_FLIP_X)
				gdImageSetPixel(im, gdImageSX(im) - x - 1, y, cc);
			else if (photo_flip == PHOTO_FLIP_Y)
				gdImageSetPixel(im, x, gdImageSY(im) - y - 1, cc);
			else
				gdImageSetPixel(im, x, y, cc);
		}
	}
}

static void png_photo_as_shown()
{
	int i, n = 0;
	pcb_layergrp_id_t solder_layer = -1, comp_layer = -1;

	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder_layer, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_layer, 1);
	assert(solder_layer >= 0);
	assert(comp_layer >= 0);

	photo_has_inners = 0;
	if (comp_layer < solder_layer)
		for (i = comp_layer; i <= solder_layer; i++) {
			photo_groups[n++] = i;
			if (i != comp_layer && i != solder_layer && !pcb_layergrp_is_empty(PCB, i))
				photo_has_inners = 1;
		}
	else
		for (i = comp_layer; i >= solder_layer; i--) {
			photo_groups[n++] = i;
			if (i != comp_layer && i != solder_layer && !pcb_layergrp_is_empty(PCB, i))
				photo_has_inners = 1;
		}
	if (!photo_has_inners) {
		photo_groups[1] = photo_groups[n - 1];
		n = 2;
	}
	photo_ngroups = n;

	if (photo_flip) {
		for (i = 0, n = photo_ngroups - 1; i < n; i++, n--) {
			int tmp = photo_groups[i];
			photo_groups[i] = photo_groups[n];
			photo_groups[n] = tmp;
		}
	}
}

static void png_photo_do_export(pcb_hid_attr_val_t *options)
{
	options[HA_mono].lng = 1;
	options[HA_as_shown].lng = 0;
	memset(photo_copper, 0, sizeof(photo_copper));
	photo_silk = photo_mask = photo_drill = 0;
	photo_outline = 0;
	if (options[HA_photo_flip_x].lng)
		photo_flip = PHOTO_FLIP_X;
	else if (options[HA_photo_flip_y].lng)
		photo_flip = PHOTO_FLIP_Y;
	else
		photo_flip = 0;
}

static void png_photo_post_export()
{
	int i;
	if (photo_silk != NULL) {
		gdImageDestroy(photo_silk);
		photo_silk = NULL;
	}
	if (photo_mask != NULL) {
		gdImageDestroy(photo_mask);
		photo_mask = NULL;
	}
	if (photo_drill != NULL) {
		gdImageDestroy(photo_drill);
		photo_drill = NULL;
	}
	if (photo_outline != NULL) {
		gdImageDestroy(photo_outline);
		photo_outline = NULL;
	}
	for(i = 0; i < PCB_MAX_LAYER; i++) {
		if (photo_copper[i] != NULL) {
			gdImageDestroy(photo_copper[i]);
			photo_copper[i] = NULL;
		}
	}
}
