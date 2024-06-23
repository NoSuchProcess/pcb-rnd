#include <ctype.h>
#include "svgpath.h"

typedef struct ctx_s {
	const svgpath_cfg_t *cfg;
	void *uctx;
	const char *path;
	double approx_len;

	double startx, starty;
	double x, y; /* cursor */
	unsigned cursor_valid:1;
	unsigned error:1;
} ctx_t;

static void sp_error(ctx_t *ctx, const char *s, const char *errmsg)
{
	if (ctx->cfg->error != NULL)
		ctx->cfg->error(ctx->uctx, errmsg, s - ctx->path);
	ctx->error = 1;
}

static void sp_lin(ctx_t *ctx, double x1, double y1, double x2, double y2)
{
	if (ctx->cfg->line == NULL)
		return;

	if ((x1 == x2) && (y1 == y2))
		return;

	ctx->cfg->line(ctx->uctx, x1, y1, x2, y2);
}

static const char *sp_move(ctx_t *ctx, const char *s, int relative)
{
	double x, y;
	int len, convr;

	convr = sscanf(s, "%lf %lf%n", &x, &y, &len);
	if (convr != 2) {
		sp_error(ctx, s, "Expected two decimals for M or m");
		return s;
	}
	s += len;

	if (!ctx->cursor_valid || !relative) {
		ctx->x = x;
		ctx->y = y;
		if (!ctx->cursor_valid) {
			ctx->startx = x;
			ctx->starty = y;
			ctx->cursor_valid = 1;
		}
	}
	else {
		ctx->x += x;
		ctx->y += y;
	}

	return s;
}

static const char *sp_line(ctx_t *ctx, const char *s, int relative)
{
	double ex, ey;
	int len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before L or l");
		return s;
	}

	convr = sscanf(s, "%lf %lf%n", &ex, &ey, &len);
	if (convr != 2) {
		sp_error(ctx, s, "Expected two decimals for L or l");
		return s;
	}
	s += len;

	if (relative) {
		ex += ctx->x;
		ey += ctx->y;
	}

	sp_lin(ctx, ctx->x, ctx->y, ex, ey);

	ctx->x = ex;
	ctx->y = ey;

	return s;
}


static const char *sp_hvline(ctx_t *ctx, const char *s, int relative, int is_vert)
{
	double end, ex, ey;
	int len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before H or h or V or v");
		return s;
	}

	convr = sscanf(s, "%lf%n", &end, &len);
	if (convr != 1) {
		sp_error(ctx, s, "Expected one decimal for H or h or V or v");
		return s;
	}
	s += len;

	ex = ctx->x;
	ey = ctx->y;
	if (relative) {
		if (is_vert)
			ey += end;
		else
			ex += end;
	}
	else {
		if (is_vert)
			ey = end;
		else
			ex =end;
	}

	sp_lin(ctx, ctx->x, ctx->y, ex, ey);

	ctx->x = ex;
	ctx->y = ey;

	return s;
}

static const char *sp_close(ctx_t *ctx, const char *s)
{
	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before Z or z");
		return s;
	}

	/* already closed - nothing to do */
	if ((ctx->x == ctx->startx) && (ctx->y == ctx->starty))
		return s;

	sp_lin(ctx, ctx->x, ctx->y, ctx->startx, ctx->starty);

	ctx->x = ctx->startx;
	ctx->y = ctx->starty;
	return s;
}


int svgpath_render(const svgpath_cfg_t *cfg, void *uctx, const char *path)
{
	const char *s;
	ctx_t ctx;

	ctx.approx_len = (cfg->curve_approx_seglen > 0) ? cfg->curve_approx_seglen : 1.0;
	ctx.uctx = uctx;
	ctx.path = path;
	ctx.cfg = cfg;
	ctx.cursor_valid = 0;

	for(s = path;;) {
		while(isspace(*s)) s++;
		switch(*s) {
			case '\0': return 0;
			case 'M': case 'm': s = sp_move(&ctx, s+1, (*s == 'm')); break;
			case 'L': case 'l': s = sp_line(&ctx, s+1, (*s == 'l')); break;
			case 'H': case 'h': s = sp_hvline(&ctx, s+1, (*s == 'h'), 0); break;
			case 'V': case 'v': s = sp_hvline(&ctx, s+1, (*s == 'v'), 1); break;
			case 'Z': case 'z': s = sp_close(&ctx, s+1); break;
			default:
				sp_error(&ctx, s, "Invalid command");
		}
		if (ctx.error)
			return -1;
	}

	return 0;
}

