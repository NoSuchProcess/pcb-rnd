/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf low level loader
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include <ft2build.h>
#include <freetype.h>
#include <ftmodapi.h>
#include <ftglyph.h>
#include <ftoutln.h>
#include "ttf_load.h"

#	undef __FTERRORS_H__
#	define FT_ERRORDEF( e, v, s )  { e, s },
#	define FT_ERROR_START_LIST     {
#	define FT_ERROR_END_LIST       { 0, 0 } };
	static const struct {
		FT_Error errnum;
		const char *errstr;
	} ft_errtab[] =
#	include FT_ERRORS_H

static const char *int_errtab[] = {
	"success",
	"Need an outline font"
};

int pcb_ttf_load(pcb_ttf_t *ctx, const char *fn)
{
	FT_Error errnum;

	errnum = FT_Init_FreeType(&ctx->library);
	if (errnum == 0)
		errnum = FT_New_Face(ctx->library, fn, 0, &ctx->face);
	return errnum;
}

int pcb_ttf_unload(pcb_ttf_t *ctx)
{
	FT_Done_Face(ctx->face);
	FT_Done_Library(ctx->library);
	memset(&ctx, 0, sizeof(pcb_ttf_t));
}


FT_Error pcb_ttf_trace(pcb_ttf_t *ctx, FT_ULong ttf_chr, FT_ULong out_chr, pcb_ttf_stroke_t *str, unsigned short int scale)
{
	FT_Error err;
	FT_Glyph gly;
	FT_OutlineGlyph ol;
	FT_Matrix mx;

	if (scale > 1) {
		mx.xx = scale << 16;
		mx.xy = 0;
		mx.yy = scale << 16;
		mx.yx = 0;
		FT_Set_Transform(ctx->face, &mx, NULL);
	}
	else
		FT_Set_Transform(ctx->face, NULL, NULL);

	err = FT_Load_Glyph(ctx->face, FT_Get_Char_Index(ctx->face, ttf_chr), FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
	if (err != 0)
		return err;

	FT_Get_Glyph(ctx->face->glyph, &gly);
	ol = (FT_OutlineGlyph)gly;
	if (ctx->face->glyph->format != ft_glyph_format_outline)
		return -1; /* this method supports only outline font */

	str->start(str, out_chr);
	err = FT_Outline_Decompose(&(ol->outline), &str->funcs, str);
	str->finish(str);

	if (err != 0)
		return err;

	return 0;
}

const char *pcb_ttf_errmsg(FT_Error errnum)
{
	if (errnum > 0) {
		if (errnum < sizeof(ft_errtab) / sizeof(ft_errtab[0]))
			return ft_errtab[errnum].errstr;
		return "Invalid freetype2 error code.";
	}

	errnum = -errnum;
	if (errnum < sizeof(int_errtab) / sizeof(int_errtab[0]))
		return int_errtab[errnum];

	return "Invalid internal error code.";
}
