typedef struct pcb_smart_label_s pcb_smart_label_t;

struct pcb_smart_label_s {
	rnd_box_t bbox;
	rnd_coord_t x;
	rnd_coord_t y;
	double scale;
	double rot;
	rnd_bool mirror;
	rnd_coord_t w;
	rnd_coord_t h;
	pcb_smart_label_t *next;
	char label[1]; /* dynamic length array */
};

static pcb_smart_label_t *smart_labels = NULL;

RND_INLINE void pcb_label_smart_add(pcb_draw_info_t *info, rnd_coord_t x, rnd_coord_t y, double scale, double rot, rnd_bool mirror, rnd_coord_t w, rnd_coord_t h, const char *label)
{
	int len = strlen(label);
	pcb_smart_label_t *l = malloc(sizeof(pcb_smart_label_t) + len + 1);

	l->x = x; l->y = y;
	l->scale = scale;
	l->rot = rot;
	l->mirror = mirror;
	l->w = w; l->h = h;
	memcpy(l->label, label, len+1);

	l->next = smart_labels;
	smart_labels = l;
}

/*RND_INLINE void lsm_place()
{

}*/


RND_INLINE void pcb_label_smart_flush(pcb_draw_info_t *info)
{
	pcb_smart_label_t *l, *next;
	pcb_font_t *font;

	if (smart_labels == NULL) return;

	font = pcb_font(PCB, 0, 0);
	rnd_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.pin_name);
	if (rnd_render->gui)
		pcb_draw_force_termlab++;

	for(l = smart_labels; l != NULL; l = next) {
		next = l->next;
		pcb_text_draw_string(info, font, (unsigned const char *)l->label, l->x, l->y, l->scale, l->scale, l->rot, l->mirror, 1, 0, 0, 0, 0, PCB_TXT_TINY_HIDE);
		free(l);
	}
	smart_labels = NULL;

	if (rnd_render->gui)
		pcb_draw_force_termlab--;
}
