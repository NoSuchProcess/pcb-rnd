/* Because all the macros expect it, that's why.  */
typedef struct {
	pcb_flag_t Flags;
} io_lihata_flag_holder;

/* Convert between thermal style index and flag's textual representation */
const char *io_lihata_thermal_style_old(int idx);
int io_lihata_resolve_thermal_style_old(const char *name, int ver);
