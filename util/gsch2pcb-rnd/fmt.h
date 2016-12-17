typedef struct fmt_s  fmt_t;

struct fmt_s {
	const char *name;
	void (*init)(void);
	void (*go)(void);
	void (*uninit)(void);
	fmt_t *next;
};

void fmt_register(fmt_t *fmt);
