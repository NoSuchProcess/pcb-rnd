typedef struct {
	const char *name;
	pcb_coord_t Width, Height;
	pcb_coord_t MarginX, MarginY;
} MediaType, *MediaTypePtr;

extern MediaType media_data[];
