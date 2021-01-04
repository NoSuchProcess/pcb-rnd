#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "cdt.h"

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
		else fprintf(stderr, "syntax error: unknown command '%s'\n", cmd);
	}
	return 0;
}

