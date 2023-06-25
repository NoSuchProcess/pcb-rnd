
/*** formats ***/

static void bom_free_fmts(void)
{
	int n;
	for(n = 0; n < bom_fmt_ids.used; n++) {
		free(bom_fmt_ids.array[n]);
		bom_fmt_ids.array[n] = NULL;
	}
	bom_fmt_names.used = 0;
	bom_fmt_ids.used = 0;
}

static void bom_fmt_init(void)
{
	vts0_init(&bom_fmt_names);
	vts0_init(&bom_fmt_ids);
}

static void bom_fmt_uninit(void)
{
	vts0_uninit(&bom_fmt_names);
	vts0_uninit(&bom_fmt_ids);
}

static void bom_build_fmts(const rnd_conflist_t *templates)
{
	rnd_conf_listitem_t *li;
	int idx;

	bom_free_fmts();

	rnd_conf_loop_list(templates, li, idx) {
		char id[MAX_TEMP_NAME_LEN];
		const char *sep = strchr(li->name, '.');
		int len;

		if (sep == NULL) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring invalid template name (missing period): '%s'\n", li->name);
			continue;
		}
		if (strcmp(sep+1, "name") != 0)
			continue;
		len = sep - li->name;
		if (len > sizeof(id)-1) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring invalid template name (too long): '%s'\n", li->name);
			continue;
		}
		memcpy(id, li->name, len);
		id[len] = '\0';
		vts0_append(&bom_fmt_names, (char *)li->payload);
		vts0_append(&bom_fmt_ids, rnd_strdup(id));
	}
}

static void bom_gather_templates(void)
{
	rnd_conf_listitem_t *i;
	int n;

	rnd_conf_loop_list(&conf_bom2.plugins.export_bom2.templates, i, n) {
		char buff[256], *id, *sect;
		int nl = strlen(i->name);
		if (nl > sizeof(buff)-1) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring template '%s': name too long\n", i->name);
			continue;
		}
		memcpy(buff, i->name, nl+1);
		id = buff;
		sect = strchr(id, '.');
		if (sect == NULL) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring template '%s': does not have a .section suffix\n", i->name);
			continue;
		}
		*sect = '\0';
		sect++;
	}
}

static const char *get_templ(const char *tid, const char *type)
{
	char path[MAX_TEMP_NAME_LEN + 16];
	rnd_conf_listitem_t *li;
	int idx;

	sprintf(path, "%s.%s", tid, type); /* safe: tid's length is checked before it was put in the vector, type is hardwired in code and is never longer than a few chars */
	rnd_conf_loop_list(&conf_bom2.plugins.export_bom2.templates, li, idx)
		if (strcmp(li->name, path) == 0)
			return li->payload;
	return NULL;
}

static void bom_init_template(bom_template_t *templ, const char *tid)
{
	templ->header       = get_templ(tid, "header");
	templ->item         = get_templ(tid, "item");
	templ->footer       = get_templ(tid, "footer");
	templ->sort_id      = get_templ(tid, "sort_id");
	templ->escape       = get_templ(tid, "escape");
	templ->needs_escape = get_templ(tid, "needs_escape");
}

/*** subst ***/

typedef struct bom_item_s {
	pcb_subc_t *subc; /* one of the subcircuits picked randomly, for the attributes */
	char *id; /* key for sorting */
	gds_t refdes_list;
	long cnt;
} bom_item_t;

static void append_clean(bom_subst_ctx_t *ctx, int escape, gds_t *dst, const char *text)
{
	const char *s;

	if (!escape) {
		gds_append_str(dst, text);
		return;
	}

	for(s = text; *s != '\0'; s++) {
		switch(*s) {
			case '\n': gds_append_str(dst, "\\n"); break;
			case '\r': gds_append_str(dst, "\\r"); break;
			case '\t': gds_append_str(dst, "\\t"); break;
			default:
				if ((ctx->needs_escape != NULL) && (strchr(ctx->needs_escape, *s) != NULL)) {
					if ((ctx->escape != NULL) && (*ctx->escape != '\0')) {
						gds_append(dst, *ctx->escape);
						gds_append(dst, *s);
					}
					else
						gds_append(dst, '_');
				}
				else
					gds_append(dst, *s);
				break;
		}
	}
}

static int is_val_true(const char *val)
{
	if (val == NULL) return 0;
	if (strcmp(val, "yes") == 0) return 1;
	if (strcmp(val, "on") == 0) return 1;
	if (strcmp(val, "true") == 0) return 1;
	if (strcmp(val, "1") == 0) return 1;
	return 0;
}

static int subst_cb(void *ctx_, gds_t *s, const char **input)
{
	bom_subst_ctx_t *ctx = ctx_;
	int escape = 0, ternary = 0;
	char aname[1024], unk_buf[1024], *nope = NULL;
	char tmp[32];
	const char *str, *end;
	const char *unk = ""; /* what to print on empty/NULL string if there's no ? or | in the template */
	long len;

	if (strncmp(*input, "escape.", 7) == 0) {
		*input += 7;
		escape = 1;
	}

	/* subc attribute print:
	    subc.a.attribute            - print the attribute if exists, "n/a" if not
	    subc.a.attribute|unk        - print the attribute if exists, unk if not
	    subc.a.attribute?yes        - print yes if attribute is true, "n/a" if not
	    subc.a.attribute?yes:nope   - print yes if attribute is true, nope if not
	*/
	end = strpbrk(*input, "?|%");
	len = end - *input;
	if (len >= sizeof(aname) - 1) {
		rnd_message(RND_MSG_ERROR, "bom2 tempalte error: attribute name '%s' too long\n", *input);
		return 1;
	}
	memcpy(aname, *input, len);
	aname[len] = '\0';
	if (*end == '|') { /* "or unknown" */
		*input = end+1;
		end = strchr(*input, '%');
		len = end - *input;
		if (len >= sizeof(unk_buf) - 1) {
			rnd_message(RND_MSG_ERROR, "bom2 tempalte error: elem atribute '|unknown' field '%s' too long\n", *input);
			return 1;
		}
		memcpy(unk_buf, *input, len);
		unk_buf[len] = '\0';
		unk = unk_buf;
		*input = end+1;
	}
	else if (*end == '?') { /* trenary */
		*input = end+1;
		ternary = 1;
		end = strchr(*input, '%');
		len = end - *input;
		if (len >= sizeof(unk_buf) - 1) {
			rnd_message(RND_MSG_ERROR, "bom2 tempalte error: elem atribute trenary field '%s' too long\n", *input);
			return 1;
		}

		memcpy(unk_buf, *input, len);
		unk_buf[len] = '\0';
		*input = end+1;

		nope = strchr(unk_buf, ':');
		if (nope != NULL) {
			*nope = '\0';
			nope++;
		}
		else /* only '?' is given, no ':' */
			nope = "n/a";
	}
	else /* plain '%' */
		*input = end+1;

	/* get the actual string; first a few generic ones then the app-specific callback */
	if (strcmp(aname, "count") == 0) {
		sprintf(tmp, "%ld", ctx->count);
		str = tmp;
	}
	else if (strcmp(aname, "UTC") == 0) str = ctx->utcTime;
	else if (strcmp(aname, "names") == 0) str = ctx->name;
	else str = subst_user(ctx, aname);

	/* render output */
	if (ternary) {
		if (is_val_true(str))
			append_clean(ctx, escape, s, unk_buf);
		else
			append_clean(ctx, escape, s, nope);
		return 0;
	}

	if ((str == NULL) || (*str == '\0'))
		str = unk;
	append_clean(ctx, escape, s, str);

	return 0;
}

static void fprintf_templ(FILE *f, bom_subst_ctx_t *ctx, const char *templ)
{
	if (templ != NULL) {
		char *tmp = rnd_strdup_subst(templ, subst_cb, ctx, RND_SUBST_PERCENT);
		fprintf(f, "%s", tmp);
		free(tmp);
	}
}

static char *render_templ(bom_subst_ctx_t *ctx, const char *templ)
{
	if (templ != NULL)
		return rnd_strdup_subst(templ, subst_cb, ctx, RND_SUBST_PERCENT);
	return NULL;
}

static int item_cmp(const void *item1, const void *item2)
{
	const bom_item_t * const *i1 = item1;
	const bom_item_t * const *i2 = item2;
	return strcmp((*i1)->id, (*i2)->id);
}

static void bom_print_begin(bom_subst_ctx_t *ctx, FILE *f, const bom_template_t *templ)
{
	gds_init(&ctx->tmp);

	rnd_print_utc(ctx->utcTime, sizeof(ctx->utcTime), 0);

	fprintf_templ(f, ctx, templ->header);

	htsp_init(&ctx->tbl, strhash, strkeyeq);
	vtp0_init(&ctx->arr);

	ctx->escape = templ->escape;
	ctx->needs_escape = templ->needs_escape;
	ctx->templ = templ;
	ctx->f = f;
}

static void bom_print_add(bom_subst_ctx_t *ctx, pcb_subc_t *subc, const char *name)
{
	char *id, *freeme;
	bom_item_t *i;

	ctx->subc = subc;
	ctx->name = (char *)name;

	id = freeme = render_templ(ctx, ctx->templ->sort_id);
	i = htsp_get(&ctx->tbl, id);
	if (i == NULL) {
		i = malloc(sizeof(bom_item_t));
		i->id = id;
		i->subc = subc;
		i->cnt = 1;
		gds_init(&i->refdes_list);

		htsp_set(&ctx->tbl, id, i);
		vtp0_append(&ctx->arr, i);
		freeme = NULL;
	}
	else {
		i->cnt++;
		gds_append(&i->refdes_list, ' ');
	}

	gds_append_str(&i->refdes_list, name);
	rnd_trace("id='%s' %ld\n", id, i->cnt);

	free(freeme);
}

static void bom_print_all(bom_subst_ctx_t *ctx)
{
	long n;

	/* clean up and sort the array */
	ctx->subc = NULL;
	qsort(ctx->arr.array, ctx->arr.used, sizeof(bom_item_t *), item_cmp);

	/* produce the actual output from the sorted array */
	for(n = 0; n < ctx->arr.used; n++) {
		bom_item_t *i = ctx->arr.array[n];
		ctx->subc = i->subc;
		ctx->name = i->refdes_list.array;
		ctx->count = i->cnt;
		fprintf_templ(ctx->f, ctx, ctx->templ->item);
	}
}

static void bom_print_end(bom_subst_ctx_t *ctx)
{
	fprintf_templ(ctx->f, ctx, ctx->templ->footer);

	gds_uninit(&ctx->tmp);

	genht_uninit_deep(htsp, &ctx->tbl, {
		bom_item_t *i = htent->value;
		free(i->id);
		gds_uninit(&i->refdes_list);
		free(i);
	});
	vtp0_uninit(&ctx->arr);
}

