/* Because all the macros expect it, that's why.  */
typedef struct {
	FlagType Flags;
} io_lihata_flag_holder;

/* Convert between thermal style index and textual representation */
const char *io_lihata_thermal_style(int idx);
int io_lihata_resolve_thermal_style(const char *name);
