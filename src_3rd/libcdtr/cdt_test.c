#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include "cdt.h"

#define MAXP 64
#define MAXE 64
point_t *P[MAXP];
edge_t *E[MAXE];
cdt_t cdt;


static void autotest(void)
{
	point_t *p1, *p2;
	edge_t *ed[500];
	int i, j, e_num;
	pointlist_node_t *p_violations = NULL;

	cdt_init(&cdt, 0, 0, 100000, 100000);

	srand(time(NULL));
	e_num = 0;
	for (i = 0; i < 100; i++) {
		int isect = 0;
		p1 = cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
		p2 = cdt_insert_point(&cdt, rand()%99999 + 1, rand()%99999 + 1);
		for (j = 0; j < e_num; j++)
			if (LINES_INTERSECT(p1, p2, ed[j]->endp[0], ed[j]->endp[1])) {
				isect = 1;
				break;
			}
		if (!isect) {
			ed[e_num] = cdt_insert_constrained_edge(&cdt, p1, p2);
			e_num++;
		}
	}
	for (i = 0; i < e_num; i++) {
		fprintf(stderr, "deleting edge (%d, %d) - (%d, %d)\n", ed[i]->endp[0]->pos.x, ed[i]->endp[0]->pos.y, ed[i]->endp[1]->pos.x, ed[i]->endp[1]->pos.y);
		cdt_delete_constrained_edge(&cdt, ed[i]);
	}


	if (cdt_check_delaunay(&cdt, &p_violations, NULL))
		fprintf(stderr, "delaunay\n");
	else
		fprintf(stderr, "not delaunay\n");
	cdt_dump_animator(&cdt, 0, p_violations, NULL);
	pointlist_free(p_violations);
	cdt_free(&cdt);
}

static int print_events = 0;

static void ev_split_constrained_edge_pre(cdt_t *cdt, edge_t *cedge, point_t *pt)
{
	if (print_events)
		printf("split_constrained_edge_pre at %d;%d\n", pt->pos.x, pt->pos.y);
}

static void ev_split_constrained_edge_post(cdt_t *cdt, edge_t *ne1, edge_t *ne2, point_t *pt)
{
	if (print_events)
		printf("split_constrained_edge_post at %d;%d\n", pt->pos.x, pt->pos.y);
}

static void init_evs(void)
{
	cdt.ev_split_constrained_edge_pre = ev_split_constrained_edge_pre;
	cdt.ev_split_constrained_edge_post = ev_split_constrained_edge_post;
}

static void cmd_init(char *args)
{
	long x1, y1, x2, y2;
	if (sscanf(args, "%ld %ld %ld %ld", &x1, &y1, &x2, &y2) != 4) {
		fprintf(stderr, "syntax error: init requires 4 arguments\n");
		return;
	}
	cdt_init(&cdt, x1, y1, x2, y2);
	init_evs();
}

static void cmd_raw_init(char *args)
{
	cdt_init_(&cdt);
	init_evs();
}

static void cmd_free(char *args)
{
	cdt_free(&cdt);
	memset(P, 0, sizeof(P));
	memset(E, 0, sizeof(E));
}

static void cmd_del_point(char *args)
{
	int id = -1;

	if (*args != 'p') {
		fprintf(stderr, "syntax error: del_point needs a point id (with the p prefix)\n");
		return;
	}
	args++;
	if (sscanf(args, "%d", &id) != 1) {
		fprintf(stderr, "syntax error: del_point an arguments\n");
		return;
	}
	if ((id < 0) || (id >= MAXP)) {
		fprintf(stderr, "syntax error: del_point id out of range\n");
		return;
	}

	cdt_delete_point(&cdt, P[id]);
	P[id] = NULL;
}

static double coord_scale = 1;

static void cmd_scale(char *args)
{
	double tmp;
	if (sscanf(args, "%lf", &tmp) != 1) {
		fprintf(stderr, "syntax error: scale requires a numeric argument\n");
		return;
	}
	coord_scale = tmp;
}

static void cmd_ins_point(char *args, int raw)
{
	point_t *p;
	double x, y;
	int bad, id = -1;

	if (*args == 'p') {
		args++;
		bad = (sscanf(args, "%d %lf %lf", &id, &x, &y) != 3);
	}
	else
		bad = (sscanf(args, "%lf %lf", &x, &y) != 2);

	if (bad) {
		fprintf(stderr, "syntax error: ins_point requires 2 or 3 arguments\n");
		return;
	}
	if (id >= MAXP) {
		fprintf(stderr, "syntax error: ins_point id out of range\n");
		return;
	}

	x *= coord_scale;
	y *= coord_scale;

	if (raw) {
		point_t *p = *vtpoint_alloc_append(&cdt.points, 1);
		p->pos.x = x;
		p->pos.y = y;
		if (x < cdt.bbox_tl.x) cdt.bbox_tl.x = x;
		if (x > cdt.bbox_br.x) cdt.bbox_br.x = x;
		if (y < cdt.bbox_tl.y) cdt.bbox_tl.y = y;
		if (y > cdt.bbox_br.y) cdt.bbox_br.y = y;
	}
	else {
		p = cdt_insert_point(&cdt, x, y);
		if (p == NULL) {
			fprintf(stderr, "ins_point: failed to create\n");
			return;
		}
	}
	if (id >= 0)
		P[id] = p;
}

static void cmd_ins_cedge(char *args)
{
	edge_t *e;
	int eid = -1, pid1, pid2, bad;

	if (*args == 'e') {
		args++;
		bad = (sscanf(args, "%d p%d p%d", &eid, &pid1, &pid2) != 3);
	}
	else
		bad = (sscanf(args, "p%d p%d", &pid1, &pid2) != 2);

	if (bad) {
		fprintf(stderr, "syntax error: ins_cedge requires 2 or 3 arguments: optional edge_id, p1 and p2\n");
		return;
	}

	if (eid >= MAXE) {
		fprintf(stderr, "syntax error: ins_cedge edge id out of range\n");
		return;
	}
	if ((pid1 < 0) || (pid1 >= MAXP)) {
		fprintf(stderr, "syntax error: ins_cedge pid1 out of range\n");
		return;
	}
	if ((pid2 < 0) || (pid2 >= MAXP)) {
		fprintf(stderr, "syntax error: ins_cedge pid2 out of range\n");
		return;
	}

	e = cdt_insert_constrained_edge(&cdt, P[pid1], P[pid2]);
	if (eid >= 0)
		E[eid] = e;
}

static void cmd_del_cedge(char *args)
{
	int eid = -1;

	if (*args != 'e') {
		fprintf(stderr, "syntax error: del_cedge needs an edge id (with the e prefix)\n");
		return;
	}
	args++;
	if (sscanf(args, "%d", &eid) != 1) {
		fprintf(stderr, "syntax error: del_cedge an arguments\n");
		return;
	}
	if ((eid < 0) || (eid >= MAXP)) {
		fprintf(stderr, "syntax error: del_cedge id out of range\n");
		return;
	}

	cdt_delete_constrained_edge(&cdt, E[eid]);
	E[eid] = NULL;
}

static void cmd_raw_edge(char *args)
{
	long pi1, pi2;
	int constrain;

	if (sscanf(args, "P%ld P%ld %d", &pi1, &pi2, &constrain) != 3) {
		fprintf(stderr, "syntax error: raw_edge needs 3 arguments: P1 and P2 (with raw point indices) and constrain (1 or 0)\n");
		return;
	}

	if (cdt_new_edge_(&cdt, cdt.points.array[pi1], cdt.points.array[pi2], constrain) == NULL)
		fprintf(stderr, "raw_edge failed\n");
}

static void cmd_raw_triangle(char *args)
{
	long pi1, pi2, pi3;

	if (sscanf(args, "P%ld P%ld P%ld", &pi1, &pi2, &pi3) != 3) {
		fprintf(stderr, "syntax error: raw_trinagle needs 3 arguments: P1, P2 and P3 (with raw point indices)\n");
		return;
	}

	if (cdt_new_triangle_(&cdt, cdt.points.array[pi1], cdt.points.array[pi2], cdt.points.array[pi3]) == 0)
		fprintf(stderr, "raw_triangle failed\n");
}


static void cmd_dump(char *args, int anim)
{
	FILE *f;
	char *end = strpbrk(args, " \t\r\n");

	if (end != NULL)
		*end = '\0';

	f = fopen(args, "w");
	if (f == NULL) {
		fprintf(stderr, "dump_anim: failed to open '%s' for write\n", args);
		return;
	}

	if (anim)
		cdt_fdump_animator(f, &cdt, 0, NULL, NULL);
	else
		cdt_fdump(f, &cdt);

	fclose(f);
}

static long triangle_id(triangle_t *t)
{
	long tid;
	for(tid = 0; tid < cdt.triangles.used; tid++)
		if (t == cdt.triangles.array[tid])
			return tid;
	return -1;
}

static long edge_id(edge_t *t)
{
	long eid;
	for(eid = 0; eid < cdt.edges.used; eid++)
		if (t == cdt.edges.array[eid])
			return eid;
	return -1;
}

static void cmd_print(char *args)
{
	long tid;

	printf("--- dump %s", args);
	for(tid = 0; tid < cdt.triangles.used; tid++) {
		triangle_t *t = cdt.triangles.array[tid];
		int n;

		printf(" triangle T%ld: %d;%d %d;%d %d;%d\n", tid, t->p[0]->pos.x, t->p[0]->pos.y, t->p[1]->pos.x, t->p[1]->pos.y, t->p[2]->pos.x, t->p[2]->pos.y);

		printf("  adj triangles:");
		for(n = 0; n < 3; n++) {
			long atid = triangle_id(t->adj_t[n]);
			if (atid >= 0)
				printf(" T%ld", atid);
		}
		printf("\n");

		printf("  edges:");
		for(n = 0; n < 3; n++) {
			long eid = edge_id(t->e[n]);
			assert(eid >= 0);
			printf(" E%ld", eid);
		}
		printf("\n");
	}
}


static void cmd_print_events(char *args)
{
	switch(*args) {
		case '1': case 'y': case 'Y': case 't': case 'T': print_events = 1; break;
		case '0': case 'n': case 'N': case 'f': case 'F': print_events = 0; break;
		default:
			fprintf(stderr, "print_events: invalid boolean value '%s'\n", args);
			return;
	}
}

static int parse(FILE *f);

static void cmd_include(char *args)
{
	FILE *f;
	char *end = strpbrk(args, "\r\n");
	if (end != NULL)
		*end = '\0';
	f = fopen(args, "r");
	if (f == NULL) {
		fprintf(stderr, "include: can't open '%s' for read\n", args);
		return;
	}
	parse(f);
	fclose(f);
}


static int parse(FILE *f)
{
	char line[1024], *cmd, *args;
	while((cmd = fgets(line, sizeof(line), f)) != NULL) {
		while(isspace(*cmd)) cmd++;

		if (*cmd != '\0') {
			args = strpbrk(cmd, " \t\r\n");
			if (args == NULL) {
				fprintf(stderr, "premature EOF\n");
				return 1;
			}
			*args = '\0';
			args++;
			while(isspace(*args)) args++;
		}

		if ((*cmd == '\0') || (*cmd == '#')) { /* ignore empty lines and comments */ }
		else if (strcmp(cmd, "exit") == 0) break;
		else if (strcmp(cmd, "quit") == 0) break;
		else if (strcmp(cmd, "auto") == 0) autotest();
		else if (strcmp(cmd, "echo") == 0) printf("%s", args); /* newline is in args already */
		else if (strcmp(cmd, "include") == 0) cmd_include(args);
		else if (strcmp(cmd, "scale") == 0) cmd_scale(args);
		else if (strcmp(cmd, "init") == 0) cmd_init(args);
		else if (strcmp(cmd, "raw_init") == 0) cmd_raw_init(args);
		else if (strcmp(cmd, "free") == 0) cmd_free(args);
		else if (strcmp(cmd, "ins_point") == 0) cmd_ins_point(args, 0);
		else if (strcmp(cmd, "raw_point") == 0) cmd_ins_point(args, 1);
		else if (strcmp(cmd, "del_point") == 0) cmd_del_point(args);
		else if (strcmp(cmd, "ins_cedge") == 0) cmd_ins_cedge(args);
		else if (strcmp(cmd, "del_cedge") == 0) cmd_del_cedge(args);
		else if (strcmp(cmd, "raw_edge") == 0) cmd_raw_edge(args);
		else if (strcmp(cmd, "raw_triangle") == 0) cmd_raw_triangle(args);
		else if (strcmp(cmd, "dump") == 0) cmd_dump(args, 0);
		else if (strcmp(cmd, "dump_anim") == 0) cmd_dump(args, 1);
		else if (strcmp(cmd, "print") == 0) cmd_print(args);
		else if (strcmp(cmd, "print_events") == 0) cmd_print_events(args);
		else fprintf(stderr, "syntax error: unknown command '%s'\n", cmd);
	}
	return 0;
}


int main(void)
{
	return parse(stdin);
}