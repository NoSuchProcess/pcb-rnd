typedef struct fmt_s  fmt_t;

struct fmt_s {
	const char *name;
	void (*init)(void);
	void (*go)(void);
	void (*uninit)(void);
	int (*guess_out_name)(void); /* returns 1 if the output file of the format exists for the current project */
	fmt_t *next;
};

void fmt_register(fmt_t *fmt);
