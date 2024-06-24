#include <ctype.h>
#include <math.h>
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

void svgpath_approx_bezier_quadratic(const svgpath_cfg_t *cfg, void *uctx, double sx, double sy, double cx, double cy, double ex, double ey, double apl2)
{
	double step = 0.1, t, lx, ly, x, y;

	if (cfg->line == NULL)
		return;

	lx = sx; ly = sy;

	for(t = step; t < 1; t += step) {
		int retries = 0;

		for(retries = 1; retries < 16; retries++) {
			double mt, a, b, c;
			double dx, dy, len2, error;

			/* B(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2   @   0 <= t <= 1 */
			mt = 1-t; a = mt*mt; b = 2*mt*t; c = t*t;
			x = a*sx + b*cx + c*ex;
			y = a*sy + b*cy + c*ey;

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

void svgpath_approx_earc(const svgpath_cfg_t *cfg, void *uctx, double sx, double sy, double cx, double cy, double rx, double ry, double sa, double da, double rot, double ex, double ey, double apl2)
{
	double step, a, ea, lx, ly, x, y, rotcos, rotsin, ada, len;

	if (cfg->line == NULL)
		return;

	lx = sx; ly = sy;
	ea = sa + da;
	if (rot != 0) {
		rot = -rot;
		rotcos = cos(rot);
		rotsin = sin(rot);
	}

	/* initial estimation on step based on length of curve */
	ada = fabs(da);
	len = (rx+ry)/2 * ada;
	step = ada / (len / sqrt(apl2));

	a = sa;
	a += ((da >= 0) ? step : -step);

	for(; ((da >= 0) ? a < ea : a > ea); a += ((da >= 0) ? step : -step)) {
		int retries = 0;

		for(retries = 1; retries < 16; retries++) {
			double dx, dy, len2, error;

			/* update x;y */
			dx = cos(a) * rx;
			dy = sin(a) * ry;
			if (rot == 0) {
				x = cx + dx;
				y = cy + dy;
			}
			else{
				x = cx + dx * rotcos + dy * rotsin;
				y = cy + dy * rotcos - dx * rotsin;
			}

			/* adjust stepping */
			dx = x - lx; dy = y - ly;
			len2 = dx*dx + dy*dy;
			error = len2 / apl2;
			if (error > 1.05) {
				a -= ((da >= 0) ? step : -step);
				step *= 0.8;
				a += ((da >= 0) ? step : -step);
			}
			else if (error < 0.95) {
				a -= ((da >= 0) ? step : -step);
				step *= 1.2;
				a += ((da >= 0) ? step : -step);
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

static void sp_bezier_quadratic_common(ctx_t *ctx, double cx, double cy, double ex, double ey)
{
	if (ctx->cfg->bezier_quadratic == NULL) {
		if (ctx->approx_len2 == 0)
			ctx->approx_len2 = ctx->approx_len * ctx->approx_len;
		svgpath_approx_bezier_quadratic(ctx->cfg, ctx->uctx, ctx->x, ctx->y, cx, cy, ex, ey, ctx->approx_len2);
	}
	else
		ctx->cfg->bezier_quadratic(ctx->uctx, ctx->x, ctx->y, cx, cy, ex, ey);

	ctx->last_ccx2 = ex - cx;
	ctx->last_ccy2 = ey - cy;
	ctx->x = ex;
	ctx->y = ey;
}

static const char *sp_bezier_quadratic(ctx_t *ctx, const char *s, int relative)
{
	double cx, cy, ex, ey;
	int len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before Q or q");
		return s;
	}

	convr = sscanf(s, "%lf %lf %lf %lf%n", &cx, &cy, &ex, &ey, &len);
	if (convr != 4) {
		sp_error(ctx, s, "Expected four decimals for Q or q");
		return s;
	}
	s += len;

	if (relative) {
		cx += ctx->x;
		cy += ctx->y;
		ex += ctx->x;
		ey += ctx->y;
	}

	sp_bezier_quadratic_common(ctx, cx, cy, ex, ey);
	return s;
}

static const char *sp_bezier_quadratic_cont(ctx_t *ctx, const char *s, int relative)
{
	double cx, cy, ex, ey;
	int len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before T or t");
		return s;
	}

	convr = sscanf(s, "%lf %lf%n", &ex, &ey, &len);
	if (convr != 2) {
		sp_error(ctx, s, "Expected six decimals for T or t");
		return s;
	}
	s += len;

	if (relative) {
		ex += ctx->x;
		ey += ctx->y;
	}

	/* take control point 1 from the previous */
	switch(ctx->last_cmd) {
		case 'Q': case 'q':
		case 'T': case 't':
			cx = ctx->x + ctx->last_ccx2;
			cy = ctx->y + ctx->last_ccy2;
			break;
		default:
			/* the spec says if the previous command was not a curve command, use the starting point for control point 1 */
			cx = ctx->x;
			cy = ctx->y;
	}
	sp_bezier_quadratic_common(ctx, cx, cy, ex, ey);

	return s;
}

static const char *sp_earc(ctx_t *ctx, const char *s, int relative)
{
	double rx, ry, rotdeg, sx, sy, ex, ey, cx, cy, sa, da, rot;
	double rotsin, rotcos, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, x1_, y1_, cx_, cy_, gamma;
	int large, sweep, len, convr;

	if (!ctx->cursor_valid) {
		sp_error(ctx, s, "No valid cursor (M) before A or a");
		return s;
	}

	convr = sscanf(s, "%lf %lf %lf %d %d %lf %lf%n", &rx, &ry, &rotdeg, &large, &sweep, &ex, &ey, &len);
	if (convr != 7) {
		sp_error(ctx, s, "Expected seven decimals for A or a");
		return s;
	}
	s += len;

	if (relative) {
		ex += ctx->x;
		ey += ctx->y;
	}

	sx = ctx->x;
	sy = ctx->y;
	rot = rotdeg * M_PI / 180.0;
	rotsin = sin(rot);
	rotcos = cos(rot);

	/* Conversion to centerpoint representation is based on the SVG implementation
	   notes at https://www.w3.org/TR/SVG/implnote.html */

	/* degenerate cases as per B.2.5. */
	if (rx < 0) rx = -rx;
	if (ry < 0) ry = -ry;

	if ((rx < 1e-7) || (ry < 1e-7)) {
		sp_lin(ctx, ctx->x, ctx->y, ex, ry);
		goto fin;
	}

	/* step 3 */
	tmp1 = (sx - ex) / 2;
	tmp2 = (sy - ey) / 2;
	x1_ = rotcos * tmp1 + rotsin * tmp2;
	y1_ = -rotsin * tmp1 + rotcos * tmp2;

	gamma = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
	if (gamma > 1) {
		double sg = sqrt(gamma);
		rx *= sg;
		ry *= sg;
	}


	/* figure center point and end angles as per B.2.4. */
	/* center */
	tmp1 = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;
	if (tmp1 == 0)
		goto fin;

	tmp1 = sqrt(fabs((rx * rx * ry * ry) / tmp1 - 1));
	if (sweep == large)
		tmp1 = -tmp1;

	cx_ = tmp1 * rx * y1_ / ry;
	cy_ = -tmp1 * ry * x1_ / rx;

	cx = rotcos * cx_ - rotsin * cy_ + (sx + ex) / 2;
	cy = rotsin * cx_ + rotcos * cy_ + (sy + ey) / 2;

	/* start angle */
	tmp1 = (x1_ - cx_) / rx;
	tmp2 = (y1_ - cy_) / ry;
	tmp3 = (-x1_ - cx_) / rx;
	tmp4 = (-y1_ - cy_) / ry;

	tmp6 = tmp1 * tmp1 + tmp2 * tmp2;
	tmp5 = sqrt(fabs(tmp6));
	if (tmp5 == 0)
		goto fin;

	tmp5 = tmp1 / tmp5;
	if (tmp5 < -1) tmp5 = -1;
	else if (tmp5 > +1) tmp5 = +1;

	sa = acos(tmp5);
	if (tmp2 < 0)
		sa = -sa;

	/* delta angle */
	tmp5 = sqrt(fabs(tmp6 * (tmp3 * tmp3 + tmp4 * tmp4)));
	if (tmp5 == 0)
		goto fin;

	tmp5 = (tmp1 * tmp3 + tmp2 * tmp4) / tmp5;
	if (tmp5 < -1) tmp5 = -1;
	else if (tmp5 > +1) tmp5 = +1;

	da = acos(tmp5);
	if (tmp1 * tmp4 - tmp3 * tmp2 < 0)
		da = -da;

	if (sweep && da < 0)
		da += M_PI * 2;
	else if (!sweep && da > 0)
		da -= M_PI * 2;

	/* render carc or earc or line approx */
	if ((rx == ry) && (ctx->cfg->carc != NULL)) {
		ctx->cfg->carc(ctx->uctx, cx, cy, rx, sa, da);
	}
	else if (ctx->cfg->earc != NULL) {
		ctx->cfg->earc(ctx->uctx, cx, cy, rx, ry, sa, da, rot);
	}
	else {
		if (ctx->approx_len2 == 0)
			ctx->approx_len2 = ctx->approx_len * ctx->approx_len;
		svgpath_approx_earc(ctx->cfg, ctx->uctx, ctx->x, ctx->y, cx, cy, rx, ry, sa, da, rot, ex, ey, ctx->approx_len2);
	}

	fin:;
	ctx->x = ex;
	ctx->y = ey;

	return s;
}


/* internal instruction dispatch; s points to the args; my_last_cmd is updated
   if there's an explicit instruction */
static const char *svgpath_render_instruction(ctx_t *ctx, char inst, const char *s, const char *errmsg, char *my_last_cmd)
{
	if (my_last_cmd != NULL)
		*my_last_cmd = inst;

	switch(inst) {
		case 'M': case 'm': return sp_move(ctx, s, (inst == 'm'));
		case 'L': case 'l': return sp_line(ctx, s, (inst == 'l'));
		case 'H': case 'h': return sp_hvline(ctx, s, (inst == 'h'), 0);
		case 'V': case 'v': return sp_hvline(ctx, s, (inst == 'v'), 1);
		case 'Z': case 'z': return sp_close(ctx, s);
		case 'C': case 'c': return sp_bezier_cubic(ctx, s, (inst == 'c'));
		case 'S': case 's': return sp_bezier_cubic_cont(ctx, s, (inst == 's'));
		case 'Q': case 'q': return sp_bezier_quadratic(ctx, s, (inst == 'c'));
		case 'T': case 't': return sp_bezier_quadratic_cont(ctx, s, (inst == 's'));
		case 'A': case 'a': return sp_earc(ctx, s, (inst == 'a'));

		/* special case: most commands can be continued by simply
		   going on listing numerics */
		case '-': case '+':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (my_last_cmd != NULL) {
				*my_last_cmd = ctx->last_cmd;
				return svgpath_render_instruction(ctx, ctx->last_cmd, s-1, "Invalid multiple args for this command", NULL);
			}
			else
				sp_error(ctx, s, "internal error: double-continuation");
			break;

		default: sp_error(ctx, s, errmsg);
	}
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

		/* reached end of path after the last fully processed instruction */
		if (*s == '\0')
			return 0;

		s = svgpath_render_instruction(&ctx, *s, s+1, "Invalid command", &last_cmd);

		if (ctx.error)
			return -1;

		ctx.last_cmd = last_cmd;
	}

	return 0;
}

