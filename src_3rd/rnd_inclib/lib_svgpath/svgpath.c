#include <ctype.h>
#include "svgpath.h"

/*** curve approximations ***/
void svgpath_approx_bezier_cubic(const svgpath_cfg_t *cfg, void *uctx, double sx, double sy, double cx1, double cy1, double cx2, double cy2, double ex, double ey, double apl2)
{
	double step = 0.1, t, lx, ly, x, y;

	if (cfg->line == NULL)
		return;

	lx = sx; ly = sy;

	for(t = step; t < 1; t += step) {
		int retries = 0;

		for(retries = 1; retries < 16; retries++) {
			double mt, a, b, c, d;
			double dx, dy, len2, error;

			/* B(t) = (1-t)^3*P0 + 3*(1-t)^2*t*P1 + 3*(1-t)*t^2*P2 + t^3*P3   @   0 <= t <= 1 */
			mt = 1-t; a = mt*mt*mt; b = 3*mt*mt*t; c = 3*mt*t*t; d = t*t*t;
			x = a*sx + b*cx1 + c*cx2 + d*ex;
			y = a*sy + b*cy1 + c*cy2 + d*ey;

			/* adjust stepping */
			dx = x - lx; dy = y - ly;
			len2 = dx*dx + dy*dy;
			error = len2 / apl2;
			if (error > 1.05) {
				t -= step;
				step *= 0.8;
				t += step;
			}
			else if (error < 0.95) {
				t -= step;
				step *= 1.2;
				t += step;
			}
			else
				break;
		}


		if ((lx != x) || (ly != y)) {
			cfg->line(uctx, lx, ly, x, y);
			lx = x;
			ly = y;
		}
	}

	if ((lx != ex) || (ly != ey))
		cfg->line(uctx, lx, ly, ex, ey);
}

/*** instruction parser ***/

typedef struct ctx_s {
	const svgpath_cfg_t *cfg;
	void *uctx;
	const char *path;
	double approx_len, approx_len2;

	double startx, starty;
	double x, y; /* cursor */

	char last_cmd; /* command byte of the previous command - for _cont() functions to figure if they are continuing the right curve */
	double last_ccx2, last_ccy2; /* last control point2 for cubic Bezier continuation; relative to the endpoint */

	unsigned cursor_valid:1; /* if x;y is set */
	unsigned error:1;        /* path has a fatal error, quit */
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

static void sp_bezier_cubic_common(ctx_t *ctx, double cx1, double cy1, double cx2, double cy2, double ex, double ey)
{
	if (ctx->cfg->bezier_cubic == NULL) {
		if (ctx->approx_len2 == 0)
			ctx->approx_len2 = ctx->approx_len * ctx->approx_len;
		svgpath_approx_bezier_cubic(ctx->cfg, ctx->uctx, ctx->x, ctx->y, cx1, cy1, cx2, cy2, ex, ey, ctx->approx_len2);
	}
	else
		ctx->cfg->bezier_cubic(ctx->uctx, ctx->x, ctx->y, cx1, cy1, cx2, cy2, ex, ey);

	ctx->last_ccx2 = ex - cx2;
	ctx->last_ccy2 = ey - cy2;
	ctx->x = ex;
	ctx->y = ey;
}

static const char *sp_bezier_cubic(ctx_t *ctx, const char *s, int relative)
{
	double cx1, cy1, cx2, cy2, ex, ey;
	int len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before C or c");
		return s;
	}

	convr = sscanf(s, "%lf %lf, %lf %lf, %lf %lf%n", &cx1, &cy1, &cx2, &cy2, &ex, &ey, &len);
	if (convr != 6) {
		sp_error(ctx, s, "Expected six decimals for C or c");
		return s;
	}
	s += len;

	if (relative) {
		cx1 += ctx->x;
		cy1 += ctx->y;
		cx2 += ctx->x;
		cy2 += ctx->y;
		ex += ctx->x;
		ey += ctx->y;
	}

	sp_bezier_cubic_common(ctx, cx1, cy1, cx2, cy2, ex, ey);
	return s;
}

static const char *sp_bezier_cubic_cont(ctx_t *ctx, const char *s, int relative)
{
	double cx1, cy1, cx2, cy2, ex, ey;
	int len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before C or c");
		return s;
	}

	convr = sscanf(s, "%lf %lf, %lf %lf%n", &cx2, &cy2, &ex, &ey, &len);
	if (convr != 4) {
		sp_error(ctx, s, "Expected six decimals for C or c");
		return s;
	}
	s += len;

	if (relative) {
		cx2 += ctx->x;
		cy2 += ctx->y;
		ex += ctx->x;
		ey += ctx->y;
	}

	/* take control point 1 from the previous */
	switch(ctx->last_cmd) {
		case 'C': case 'c':
		case 'S': case 's':
			cx1 = ctx->x + ctx->last_ccx2;
			cy1 = ctx->y + ctx->last_ccy2;
			break;
		default:
			/* the spec says if the previous command was not a curve command, use the starting point for control point 1 */
			cx1 = ctx->x;
			cy1 = ctx->y;
	}
	sp_bezier_cubic_common(ctx, cx1, cy1, cx2, cy2, ex, ey);

	return s;
}

/*** path parser ***/

int svgpath_render(const svgpath_cfg_t *cfg, void *uctx, const char *path)
{
	const char *s;
	ctx_t ctx;

	ctx.approx_len = (cfg->curve_approx_seglen > 0) ? cfg->curve_approx_seglen : 1.0;
	ctx.approx_len2 = 0;
	ctx.uctx = uctx;
	ctx.path = path;
	ctx.cfg = cfg;
	ctx.cursor_valid = 0;

	for(s = path;;) {
		char last_cmd;

		while(isspace(*s)) s++;
		last_cmd = *s;

		switch(*s) {
			case '\0': return 0;
			case 'M': case 'm': s = sp_move(&ctx, s+1, (*s == 'm')); break;
			case 'L': case 'l': s = sp_line(&ctx, s+1, (*s == 'l')); break;
			case 'H': case 'h': s = sp_hvline(&ctx, s+1, (*s == 'h'), 0); break;
			case 'V': case 'v': s = sp_hvline(&ctx, s+1, (*s == 'v'), 1); break;
			case 'Z': case 'z': s = sp_close(&ctx, s+1); break;
			case 'C': case 'c': s = sp_bezier_cubic(&ctx, s+1, (*s == 'c')); break;
			case 'S': case 's': s = sp_bezier_cubic_cont(&ctx, s+1, (*s == 's')); break;
			default:
				sp_error(&ctx, s, "Invalid command");
		}

		if (ctx.error)
			return -1;

		ctx.last_cmd = last_cmd;
	}

	return 0;
}

