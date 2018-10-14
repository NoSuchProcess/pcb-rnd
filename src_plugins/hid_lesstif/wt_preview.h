typedef struct pcb_ltf_preview_s {
	void *hid_ctx;
	pcb_hid_attribute_t *attr;
	Window window;
	pcb_coord_t x, y;										/* PCB coordinates of upper right corner of window */
	pcb_coord_t left, right, top, bottom;	/* PCB extents of item */
	double zoom;									/* PCB units per screen pixel */
	int v_width, v_height;				/* pixels */

	pcb_hid_expose_ctx_t ctx;
	pcb_bool (*mouse_ev)(void *widget, void *draw_data, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);
	void (*pre_close)(struct pcb_ltf_preview_s *pd);
	pcb_hid_expose_t overlay_draw;
	unsigned resized:1;
	unsigned pan:1;
	int pan_ox, pan_oy;
	pcb_coord_t pan_opx, pan_opy;
} pcb_ltf_preview_t;

void pcb_ltf_preview_callback(Widget da, pcb_ltf_preview_t *pd, XmDrawingAreaCallbackStruct *cbs);
