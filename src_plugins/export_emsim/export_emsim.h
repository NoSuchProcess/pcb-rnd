typedef struct emsim_lumped_s emsim_lumped_t;

struct emsim_lumped_s {
	enum { PORT, RESISTOR, VSRC } type;
	union {
		struct {
			char *name, *refdes, *term;
		} port;
		struct {
			char *port1, *port2, *value;
		} comp;
	} data;
	emsim_lumped_t *next;
};

typedef struct emsim_env_s {
	emsim_lumped_t *head, *tail; /* singly linked ordered list of ports and lumped components */
} emsim_env_t;

void emsim_lumped_link(emsim_env_t *ctx, emsim_lumped_t *lump);
