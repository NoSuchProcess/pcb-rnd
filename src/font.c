/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

/* font support. Note: glyphs are called symbols here */

#include "config.h"

#include <string.h>
#include <genht/hash.h>

#include <librnd/font/xform_mx.h>
#ifdef PCB_WANT_FONT2
#	include <librnd/font2/font.h>
#else
#	include <librnd/font/font.h>
#endif

#include "font.h"
#include "board.h"
#include "conf_core.h"
#include <librnd/core/error.h>
#include "plug_io.h"
#include <librnd/core/paths.h>
#include <librnd/core/compat_misc.h>
#include "event.h"
#include <librnd/core/file_loaded.h>

#include "font_internal.c"

static int pcb_parse_font_default(rnd_font_t *ptr, const char *filename)
{
	int res = pcb_parse_font(ptr, filename);
	if (res == 0)
		rnd_file_loaded_set_at("font", "default", filename, "original default font");
	return res;
}

/* parses a file with font information and installs it into the provided PCB
   checks directories given as colon separated list by resource fontPath
   if the fonts filename doesn't contain a directory component */
void pcb_font_create_default(pcb_board_t *pcb)
{
	int res = -1;
	pcb_io_err_inhibit_inc();
	rnd_conf_list_foreach_path_first(&pcb->hidlib, res, &conf_core.rc.default_font_file, pcb_parse_font_default(&pcb->fontkit.dflt, __path__));
	pcb_io_err_inhibit_dec();

	if (res != 0) {
		const char *s;
		gds_t buff;
		s = rnd_conf_concat_strlist(&conf_core.rc.default_font_file, &buff, NULL, ':');
		rnd_message(RND_MSG_WARNING, "Can't find font-symbol-file. Searched: '%s'; falling back to the embedded default font\n", s);
		rnd_font_load_internal(&pcb->fontkit.dflt, embf_font, sizeof(embf_font) / sizeof(embf_font[0]), embf_minx, embf_miny, embf_maxx, embf_maxy);
		rnd_file_loaded_set_at("font", "default", "<internal>", "original default font");
		gds_uninit(&buff);
	}
}

static rnd_font_t *pcb_font_(pcb_board_t *pcb, rnd_font_id_t id, int fallback, int unlink)
{
	if (id <= 0) {
		do_default:;
		
		if (unlink) { /* can not unlink the default/system font, rather copy it; called from the font editor */
			rnd_font_t *f = calloc(sizeof(rnd_font_t), 1);
			rnd_font_copy(f, &pcb->fontkit.dflt);
			return f;
		}
		return &pcb->fontkit.dflt;
	}

	if (pcb->fontkit.hash_inited) {
		rnd_font_t *f = htip_get(&pcb->fontkit.fonts, id);
		if (f != NULL) {
			if (unlink)
				htip_popentry(&pcb->fontkit.fonts, id);
			return f;
		}
	}

	if (fallback)
		goto do_default;

	return NULL;
}

rnd_font_t *pcb_font(pcb_board_t *pcb, rnd_font_id_t id, int fallback)
{
	return pcb_font_(pcb, id, fallback, 0);
}

rnd_font_t *pcb_font_unlink(pcb_board_t *pcb, rnd_font_id_t id)
{
	return pcb_font_(pcb, id, 0, 1);
}


static void hash_setup(pcb_fontkit_t *fk)
{
	if (fk->hash_inited)
		return;

	htip_init(&fk->fonts, longhash, longkeyeq);
	fk->hash_inited = 1;
}

rnd_font_t *pcb_new_font(pcb_fontkit_t *fk, rnd_font_id_t id, const char *name)
{
	rnd_font_t *f;

	if (id == 0)
		return NULL;

	if (id < 0)
		id = fk->last_id + 1;

	hash_setup(fk);

	/* do not attempt to overwrite/reuse existing font of the same ID, rather report error */
	f = htip_get(&fk->fonts, id);
	if (f != NULL)
		return NULL;

	f = calloc(sizeof(rnd_font_t), 1);
	htip_set(&fk->fonts, id, f);
	if (name != NULL)
		f->name = rnd_strdup(name);
	f->id = id;

	if (f->id > fk->last_id)
		fk->last_id = f->id;

	return f;
}

void pcb_fontkit_free(pcb_fontkit_t *fk)
{
	rnd_font_free(&fk->dflt);
	if (fk->hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&fk->fonts); e; e = htip_next(&fk->fonts, e)) {
			rnd_font_t *f = e->value;
			rnd_font_free(f);
		}
		htip_uninit(&fk->fonts);
		fk->hash_inited = 0;
	}
	fk->last_id = 0;
}

void pcb_fontkit_reset(pcb_fontkit_t *fk)
{
	if (fk->hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&fk->fonts); e; e = htip_first(&fk->fonts)) {
			rnd_font_t *f = e->value;
			rnd_font_free(f);
			htip_delentry(&fk->fonts, e);
		}
	}
	fk->last_id = 0;
}

static void update_last_id(pcb_fontkit_t *fk)
{
	rnd_font_id_t id;
	for (id = fk->last_id; id > 0; id--) {
		if (htip_has(&fk->fonts, id))
			break;
	}
	fk->last_id = id;
}

int pcb_del_font(pcb_fontkit_t *fk, rnd_font_id_t id)
{
	htip_entry_t *e;
	rnd_font_t *f;

	if ((id == 0) || (!fk->hash_inited) || (htip_get(&fk->fonts, id) == NULL))
		return -1;

	e = htip_popentry(&fk->fonts, id);
	f = e->value;
	rnd_font_free(f);
	rnd_event(&PCB->hidlib, PCB_EVENT_FONT_CHANGED, "i", id);
	if (id == fk->last_id)
		update_last_id(fk);
	return 0;
}

int pcb_move_font(pcb_fontkit_t *fk, rnd_font_id_t src, rnd_font_id_t dst)
{
	htip_entry_t *e;
	rnd_font_t *src_font;
	
	if ((!fk->hash_inited) || (htip_get(&fk->fonts, src) == NULL))
		return -1;
	
	pcb_del_font(fk, dst);
	
	e = htip_popentry(&fk->fonts, src);
	src_font = e->value;
	if (dst == 0) {
		rnd_font_free(&fk->dflt);
		rnd_font_copy(&fk->dflt, src_font);
		rnd_font_free(src_font);
		fk->dflt.id = 0;
	}
	else {
		htip_set(&fk->fonts, dst, src_font);
		src_font->id = dst;
	}
	if (src == fk->last_id)
		update_last_id (fk);
	fk->last_id = MAX(fk->last_id, dst);
	rnd_event(&PCB->hidlib, PCB_EVENT_FONT_CHANGED, "i", src);
	rnd_event(&PCB->hidlib, PCB_EVENT_FONT_CHANGED, "i", dst);

	return 0;
}

