Cursor ltf_cursor_override, ltf_cursor_unknown, ltf_cursor_clock;

typedef struct {
	int named_shape;
	Pixmap pixel, mask;
	Cursor cursor;
} ltf_cursor_t;

#include <genvector/genvector_undef.h>
#define GVT(x) vtlmc_ ## x
#define GVT_ELEM_TYPE ltf_cursor_t
#define GVT_SIZE_TYPE int
#define GVT_DOUBLING_THRS 256
#define GVT_START_SIZE 8
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

#include <genvector/genvector_impl.c>

typedef struct {
	const char *name;
	int shape;
} named_cursor_t;

static const named_cursor_t named_cursors[] = {
	{"question_arrow", XC_question_arrow},
	{"left_ptr", XC_left_ptr},
	{"hand", XC_hand1},
	{"crosshair", XC_crosshair},
	{"dotbox", XC_dotbox},
	{"pencil", XC_pencil},
	{"up_arrow", XC_sb_up_arrow},
	{"ul_angle", XC_ul_angle},
	{"pirate", XC_pirate},
	{"xterm", XC_xterm},
	{"iron_cross", XC_iron_cross},
	{NULL, 0}
};

static vtlmc_t ltf_mouse_cursors;

static void ltf_reg_mouse_cursor(pcb_hid_t *hid, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask)
{
	ltf_cursor_t *mc = vtlmc_get(&ltf_mouse_cursors, idx, 1);

	memset(mc, 0, sizeof(ltf_cursor_t));
	mc->named_shape = -1;

	if (pixel == NULL) {
		const named_cursor_t *c;

		if (name == NULL) {
			mc->named_shape = XC_man;
			mc->cursor = XCreateFontCursor(display, XC_man);
			return;
		}

		for(c = named_cursors; c->name != NULL; c++) {
			if (strcmp(c->name, name) == 0) {
				mc->named_shape = c->shape;
				mc->cursor = XCreateFontCursor(display, c->shape);
				return;
			}
		}
		rnd_message(PCB_MSG_ERROR, "Failed to register named mouse cursor for tool: '%s' is unknown name\n", name);
	}
	else {
		XColor fg, bg;

		mc->named_shape = -1;
		mc->pixel = XCreateBitmapFromData(display, window, (const char *)pixel, 16, 16);
		mc->mask = XCreateBitmapFromData(display, window, (const char *)mask, 16, 16);

		fg.red = fg.green = fg.blue = 65535;
		bg.red = bg.green = bg.blue = 0;
		fg.flags = bg.flags = DoRed | DoGreen | DoBlue;
		mc->cursor = XCreatePixmapCursor(display, mc->pixel, mc->mask, &fg, &bg, 0, 0);
	}
}

static void ltf_set_mouse_cursor(pcb_hid_t *hid, int idx)
{
	if (!lesstif_hid_inited)
		return;

	if (!ltf_cursor_override) {
		ltf_cursor_t *mc = vtlmc_get(&ltf_mouse_cursors, idx, 0);
		if (mc == NULL) {
			if (ltf_cursor_unknown == 0)
				ltf_cursor_unknown = XCreateFontCursor(display, XC_mouse);
			XDefineCursor(display, window, ltf_cursor_unknown);
		}
		else
			XDefineCursor(display, window, mc->cursor);
	}
	else
		XDefineCursor(display, window, ltf_cursor_override);
}

static void ltf_mouse_uninit(void)
{
	if (ltf_cursor_unknown != 0)
		XFreeCursor(display, ltf_cursor_unknown);
	if (ltf_cursor_clock != 0)
		XFreeCursor(display, ltf_cursor_clock);
}

#include <genvector/genvector_undef.h>
