#include <genht/htss.h>
#include <genht/hash.h>

typedef struct epro_s {
	pcb_board_t *pcb;
	gds_t zipdir; /* where the zip is unpacked */

	int want_pcb_id;
	char *want_pcb_name;

	htss_t pcb2fn; /* key: pcb human readable name; value: UID (file name) */
	htss_t fp2fn; /* key: footprint name as referenced from pcb; value: UID (file name) */
} epro_t;

static int epro_load_pcbs(epro_t *epro, gdom_node_t *pcbs)
{
	int n;

	if (pcbs == NULL) {
		rnd_message(RND_MSG_ERROR, "epro's project.json doesn't contain any pcb\n");
		return -1;
	}

	if (pcbs->type != GDOM_ARRAY) {
		rnd_message(RND_MSG_ERROR, "epro's project.json: wrong pcbs node type\n");
		return -1;
	}

	for(n = 0; n < pcbs->value.array.used; n++) {
		gdom_node_t *nd = pcbs->value.array.child[n];
		if (nd->type != GDOM_STRING) {
			rnd_message(RND_MSG_ERROR, "epro's project.json: invalid pcb name node type\n");
			return -1;
		}
		if (htss_has(&epro->pcb2fn, nd->value.str)) {
			rnd_message(RND_MSG_ERROR, "epro's project.json: duplicate pcb name %s\n", nd->value.str);
			return -1;
		}
		htss_set(&epro->pcb2fn, rnd_strdup(nd->value.str), rnd_strdup(nd->name_str));
		rnd_trace("project json pcb: %s -> %s\n", nd->value.str, nd->name_str);
	}

	return 0;
}

static int epro_load_fps(epro_t *epro, gdom_node_t *fps)
{
	int n, m;

	if (fps == NULL)
		return 0;

	if (fps->type != GDOM_ARRAY) {
		rnd_message(RND_MSG_ERROR, "epro's project.json: wrong footprints node type\n");
		return -1;
	}

	for(n = 0; n < fps->value.array.used; n++) {
		gdom_node_t *nd = fps->value.array.child[n];
		const char *fn = nd->name_str, *title = NULL;
		if (nd->type != GDOM_ARRAY) {
			rnd_message(RND_MSG_ERROR, "epro's project.json: invalid footprint node type\n");
			return -1;
		}

		for(m = 0; m < nd->value.array.used; m++) {
			gdom_node_t *md = nd->value.array.child[m];
			if ((md->type == GDOM_STRING) && (strcmp(md->name_str, "title") == 0)) {
				title = md->value.str;
				break;
			}
		}

		if (title == NULL) continue;

		if (htss_has(&epro->fp2fn, title)) {
			rnd_message(RND_MSG_ERROR, "epro's project.json: duplicate footprint title %s - picked one of them randomly\n", title);
			continue;
		}
		htss_set(&epro->fp2fn, rnd_strdup(title), rnd_strdup(fn));
		rnd_trace("project json FP: %s -> %s\n", title, fn);
	}

	return 0;
}


static int epro_load_project_json(epro_t *epro)
{
	gdom_node_t *root, *nd_pcbs = NULL, *nd_fps = NULL;
	long save, n;
	int res = -1;
	FILE *f;

	save = epro->zipdir.used;
	gds_append_str(&epro->zipdir, "/project.json");
	f = rnd_fopen(&epro->pcb->hidlib, epro->zipdir.array, "r");
	if (f == NULL)
		rnd_message(RND_MSG_ERROR, "failed to open epro file's project.json at %s\n", epro->zipdir.array);
	epro->zipdir.used = save;
	epro->zipdir.array[save] = '\0';
	if (f == NULL)
		return -1;

	root = gdom_json_parse(f, NULL);
	fclose(f);

	if (root == NULL) {
		rnd_message(RND_MSG_ERROR, "failed to parse epro's project.json\n");
		return -1;
	}

	htss_init(&epro->pcb2fn, strhash, strkeyeq);
	htss_init(&epro->fp2fn, strhash, strkeyeq);

	if (root->type != GDOM_ARRAY) {
		rnd_message(RND_MSG_ERROR, "failed to parse epro's project.json\n");
		goto error;
	}

	for(n = 0; n < root->value.array.used; n++) {
		gdom_node_t *nd = root->value.array.child[n];
		if (strcmp(nd->name_str, "footprints") == 0) nd_fps = nd;
		else if (strcmp(nd->name_str, "pcbs") == 0) nd_pcbs = nd;
	}


	if (epro_load_pcbs(epro, nd_pcbs) != 0)
		goto error;

	if (epro_load_fps(epro, nd_fps) != 0)
		goto error;

	res = 0; /* success */

	error:;
	gdom_free(root);
	return res;
}

static void epro_uninit(epro_t *epro)
{
	htss_entry_t *e;
	for(e = htss_first(&epro->pcb2fn); e != NULL; e = htss_next(&epro->pcb2fn, e)) {
		free(e->key);
		free(e->value);
	}
	for(e = htss_first(&epro->fp2fn); e != NULL; e = htss_next(&epro->fp2fn, e)) {
		free(e->key);
		free(e->value);
	}
	htss_uninit(&epro->pcb2fn);
	htss_uninit(&epro->fp2fn);

	TODO("clean up the temp dir");
	gds_uninit(&epro->zipdir);
}

