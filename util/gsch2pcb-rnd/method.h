typedef struct method_s  method_t;

struct method_s {
	const char *name;
	const char *desc;
	void (*init)(void);
	void (*go)(void);
	void (*uninit)(void);
	int (*guess_out_name)(void); /* returns 1 if the output file of the format exists for the current project */
	int not_by_guess;            /* if non-zero, never try to use this method automatically, guessing from file names or anything else */
	method_t *next;
};

extern method_t *methods;

void method_register(method_t *fmt);
