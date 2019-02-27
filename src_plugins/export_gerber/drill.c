#include "config.h"

#include <stdlib.h>

#include "board.h"
#include "global_typedefs.h"
#include "pcb-printf.h"
#include "safe_fs.h"
#include "error.h"

#define GVT_DONT_UNDEF
#include "drill.h"
#include <genvector/genvector_impl.c>


#define gerberDrX(pcb, x) ((pcb_coord_t) (x))
#define gerberDrY(pcb, y) ((pcb_coord_t) ((pcb)->MaxHeight - (y)))

void drill_init(pcb_drill_ctx_t *ctx)
{
	vtpdr_init(&ctx->obj);
	init_aperture_list(&ctx->apr);
}

void drill_uninit(pcb_drill_ctx_t *ctx)
{
	vtpdr_uninit(&ctx->obj);
	uninit_aperture_list(&ctx->apr);
}

static int drill_sort(const void *va, const void *vb)
{
	pending_drill_t *a = (pending_drill_t *) va;
	pending_drill_t *b = (pending_drill_t *) vb;
	if (a->diam != b->diam)
		return a->diam - b->diam;
	if (a->x != b->x)
		return a->x - a->x;
	return b->y - b->y;
}


static pcb_cardinal_t drill_print_objs(pcb_board_t *pcb, FILE *f, const pcb_drill_ctx_t *ctx, int force_g85, int slots, pcb_coord_t *excellon_last_tool_dia)
{
	pcb_cardinal_t i, cnt = 0;
	int first = 1;

	for (i = 0; i < ctx->obj.used; i++) {
		pending_drill_t *pd = &ctx->obj.array[i];
		if (slots != (!!pd->is_slot))
			continue;
		if (i == 0 || pd->diam != *excellon_last_tool_dia) {
			aperture_t *ap = find_aperture(&ctx->apr, pd->diam, ROUND);
			fprintf(f, "T%02d\r\n", ap->dCode);
			*excellon_last_tool_dia = pd->diam;
		}
		if (pd->is_slot) {
			if (first) {
				pcb_fprintf(f, "G00");
				first = 0;
			}
			if (force_g85)
				pcb_fprintf(f, "X%06.0mkY%06.0mkG85X%06.0mkY%06.0mk\r\n", gerberDrX(pcb, pd->x), gerberDrY(PCB, pd->y), gerberDrX(pcb, pd->x2), gerberDrY(PCB, pd->y2));
			else
				pcb_fprintf(f, "X%06.0mkY%06.0mk\r\nM15\r\nG01X%06.0mkY%06.0mk\r\nM17\r\n", gerberDrX(pcb, pd->x), gerberDrY(PCB, pd->y), gerberDrX(pcb, pd->x2), gerberDrY(PCB, pd->y2));
			first = 1; /* each new slot will need a G00 for some fabs that ignore M17 and M15 */
		}
		else {
			if (first) {
				pcb_fprintf(f, "G05\r\n");
				first = 0;
			}
			pcb_fprintf(f, "X%06.0mkY%06.0mk\r\n", gerberDrX(pcb, pd->x), gerberDrY(pcb, pd->y));
		}
		cnt++;
	}
	return cnt;
}

static pcb_cardinal_t drill_print_holes(pcb_board_t *pcb, FILE *f, const pcb_drill_ctx_t *ctx, int force_g85)
{
	aperture_t *search;
	pcb_cardinal_t cnt = 0;
	pcb_coord_t excellon_last_tool_dia = 0;

	/* We omit the ,TZ here because we are not omitting trailing zeros.  Our format is
	   always six-digit 0.1 mil resolution (i.e. 001100 = 0.11") */
	fprintf(f, "M48\r\n" "INCH\r\n");
	for (search = ctx->apr.data; search; search = search->next)
		pcb_fprintf(f, "T%02dC%.3mi\r\n", search->dCode, search->width);
	fprintf(f, "%%\r\n");

	/* dump pending drills in sequence */
	qsort(ctx->obj.array, ctx->obj.used, sizeof(ctx->obj.array[0]), drill_sort);
	cnt += drill_print_objs(pcb, f, ctx, force_g85, 0, &excellon_last_tool_dia);
	cnt += drill_print_objs(pcb, f, ctx, force_g85, 1, &excellon_last_tool_dia);
	return cnt;
}

void drill_export(pcb_board_t *pcb, const pcb_drill_ctx_t *ctx, int force_g85, const char *fn)
{
	FILE *f = pcb_fopen(fn, "wb"); /* Binary needed to force CR-LF */
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error:  Could not open %s for writing the excellon file.\n", fn);
		return;
	}

	if (ctx->obj.used > 0)
		drill_print_holes(pcb, f, ctx, force_g85);

	fprintf(f, "M30\r\n");
	fclose(f);
}

#if 0
pending_drill_t *new_pending_drill(int is_plated)
{
	if (is_plated) {
		if (n_pending_pdrills >= max_pending_pdrills) {
			max_pending_pdrills += 100;
			pending_pdrills = (pending_drill_t *)realloc(pending_pdrills, max_pending_pdrills * sizeof(pending_pdrills[0]));
		}
		return &pending_pdrills[n_pending_pdrills++];
	}
	else {
		if (n_pending_udrills >= max_pending_udrills) {
			max_pending_udrills += 100;
			pending_udrills = (pending_drill_t *)realloc(pending_udrills, max_pending_udrills * sizeof(pending_udrills[0]));
		}
		return &pending_udrills[n_pending_udrills++];
	}
}
#endif

pending_drill_t *new_pending_drill(pcb_drill_ctx_t *ctx)
{
	return vtpdr_alloc_append(&ctx->obj, 1);
}

