typedef struct {
	const char *name;
	double last_run_time;
	double sum_run_time;
	long run_cnt;
	long last_hit_cnt;
	long sum_hit_cnt;
} pcb_drcq_stat_t;

static htsp_t pcb_drcq_stat;

static void pcb_drcq_stat_init(void)
{
	htsp_init(&pcb_drcq_stat, strhash, strkeyeq);
}

static void pcb_drcq_stat_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&pcb_drcq_stat); e != NULL; e = htsp_next(&pcb_drcq_stat, e)) {
		pcb_drcq_stat_t *st = e->value;
		free((char *)st->name);
		free(st);
	}
	htsp_uninit(&pcb_drcq_stat);
}

static pcb_drcq_stat_t *pcb_drcq_stat_get(const char *name)
{
	pcb_drcq_stat_t *st = htsp_get(&pcb_drcq_stat, name);

	if (st != NULL)
		return st;

	st = calloc(sizeof(pcb_drcq_stat_t), 1);
	st->name = rnd_strdup(name);
	htsp_set(&pcb_drcq_stat, (char *)st->name, st);
	return st;
}
