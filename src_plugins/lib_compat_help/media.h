typedef struct {
	const char *name;
	rnd_coord_t width, height; /* orientation: landscape */
	rnd_coord_t margin_x, margin_y;
} pcb_media_t;

extern pcb_media_t pcb_media_data[];
extern const char *pcb_medias[];

