#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "cdt.h"

#define MAXP 64
point_t *P[MAXP];
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

static void cmd_init(char *args)
{
	long x1, y1, x2, y2;
	if (sscanf(args, "%ld %ld %ld %ld", &x1, &y1, &x2, &y2) != 4) {
		fprintf(stderr, "syntax error: init requires 4 arguments\n");
		return;
	}
	cdt_init(&cdt, x1, y1, x2, y2);
}

static void cmd_free(char *args)
{
	cdt_free(&cdt);
}

static void cmd_ins_point(char *args)
{
	point_t *p;
	long x, y;
	int bad, id = -1;

	if (*args == 'p') {
		args++;
		bad = (sscanf(args, "%d %ld %ld", &id, &x, &y) != 3);
	}
	else
		bad = (sscanf(args, "%ld %ld", &x, &y) != 2);

	if (bad) {
		fprintf(stderr, "syntax error: ins_point requires 2 or 3 arguments\n");
		return;
	}
	if (id >= MAXP) {
		fprintf(stderr, "syntax error: ins_point id out of range\n");
		return;
	}

	p = cdt_insert_point(&cdt, x, y);
	if (p == NULL) {
		fprintf(stderr, "ins_point: failed to create\n");
		return;
	}
	if (id >= 0)
		P[id] = p;
}

static void cmd_ins_cedge(char *args)
{
	int id1, id2;
	if (sscanf(args, "p%d p%d", &id1, &id2) != 2) {
		fprintf(stderr, "syntax error: ins_cedge requires 2 arguments, p1 and p2\n");
		return;
	}

	if ((id1 < 0) || (id1 >= MAXP)) {
		fprintf(stderr, "syntax error: ins_cedge id1 out of range\n");
		return;
	}
	if ((id2 < 0) || (id2 >= MAXP)) {
		fprintf(stderr, "syntax error: ins_cedge id2 out of range\n");
		return;
	}

	cdt_insert_constrained_edge(&cdt, P[id1], P[id2]);
}

static void cmd_dump_anim(char *args)
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

	cdt_fdump_animator(f, &cdt, 0, NULL, NULL);
	fclose(f);
}

int main(void)
{
	char line[1024], *cmd, *args;
	while((cmd = fgets(line, sizeof(line), stdin)) != NULL) {
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
		else if (strcmp(cmd, "echo") == 0) printf("%s\n", args);
		else if (strcmp(cmd, "init") == 0) cmd_init(args);
		else if (strcmp(cmd, "free") == 0) cmd_free(args);
		else if (strcmp(cmd, "ins_point") == 0) cmd_ins_point(args);
		else if (strcmp(cmd, "ins_cedge") == 0) cmd_ins_cedge(args);
		else if (strcmp(cmd, "dump_anim") == 0) cmd_dump_anim(args);
		else fprintf(stderr, "syntax error: unknown command '%s'\n", cmd);
	}
	return 0;
}

