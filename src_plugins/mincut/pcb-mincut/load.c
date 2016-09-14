/*    pcb-mincut, a prototype project demonstrating how to highlight shorts
 *    Copyright (C) 2012 Tibor 'Igor2' Palinkas
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "genht/htsi.h"
#include "genht/hash.h"
#include "graph.h"
#include "compat_misc.h"

/*#define DEBUG_GR*/

/* maximum number of nodes */
#define MAXNODES 1024

/* maximum length of a name */
#define NAMELEN 128

/* how many edges net nodes are connected with - this would balance
   mincut to avoid cutting such connections */
#define NET_EDGES 8

#define isspc(c) (((c) == ' ') || ((c) == '\t'))
#define isid(c) ((((c) >= 'a') && ((c) <= 'z')) || (((c) >= 'A') && ((c) <= 'Z')) || (((c) >= '0') && ((c) <= '9')) || ((c) == '_'))

htsi_t *name2node = NULL;              /* hash for looking up known node name -> node id pairs */
char *node2name[MAXNODES] = {NULL};
int num_nodes;

static unsigned int keyhash(char *key) {
	unsigned char *p = (unsigned char *)key;
	unsigned int hash = 0;

	while (*p)
		hash += (hash << 2) + *p++;
	return hash;
}

gr_t *load(FILE *f)
{
	int lineno;
	int nets[2][MAXNODES];  /* nodes in nets */
	int neut[MAXNODES];     /* neutral nodes */
	int nn_net[2];          /* number of nodes in nets */
	int nn_neut;            /* number of nodes in nets */
	int nodes_fin;          /* 1 after reading the first conn line */
	gr_t *g = NULL;

/* peek at next char and return if it is a newline */
#define return_at_eol \
	do { \
		char c; \
		c = fgetc(f); \
		if ((c == '\r') || (c == '\n')) \
			return; \
		ungetc(c, f); \
	} while(0)


	/* eat up space and tabs */
	void eat_space(void)
	{
		int c;
		do {
			c = fgetc(f);
		} while(isspc(c));
		ungetc(c, f);
	}

	/* load next token of type id */
	const char *token_id(void)
	{
		static char id[NAMELEN];
		char *s;
		int n;

		n = sizeof(id) - 1;
		s = id-1;
		do {
			s++;
			*s = fgetc(f);
			n--;
			if (n <= 0) {
				*s = '\0';
				fprintf(stderr, "Error: name too long: %s...\n", id);
			}
		} while(isid(*s));
		ungetc(*s, f);
		*s = '\0';
		return id;
	}

	/* read next id token and look it up as a node name; if it does not
	   exist and alloc is 1, allocate it; if it does not exist and
	   alloc is 0, abort with error */
	int token_node(int alloc)
	{
		const char *id = token_id();
		if (htsi_has(name2node, (char *)id))
			return htsi_get(name2node, (char *)id);
		if (alloc) {
			node2name[num_nodes] = pcb_strdup(id);
			htsi_set(name2node, node2name[num_nodes], num_nodes);
			return num_nodes++;
		}
		fprintf(stderr, "Error: unknown node %s\n", id);
		abort();
	}

	/* load a net1 or net2 statement */
	void load_net(int net)
	{
		int node;
		if (nodes_fin) {
			fprintf(stderr, "Error: net%d after conn\n", net);
			abort();
		}
		/* load each node and append */
		while(1) {
			eat_space();
			return_at_eol;
			node = token_node(1);
			nets[net][nn_net[net]] = node;
			nn_net[net]++;
		}
	}

	/* load a neut statement */
	void load_neut(void)
	{
		int node;
		if (nodes_fin) {
			fprintf(stderr, "Error: neut after conn\n");
			abort();
		}
		/* load each node and append */
		while(1) {
			eat_space();
			return_at_eol;
			node = token_node(1);
			neut[nn_neut] = node;
			nn_neut++;
		}
	}

	/* load a list of connections */
	void load_conn(void)
	{
		int n1, n2;
		char c;

		/* if this is the first conn, create the graph */
		if (!nodes_fin) {
			g = gr_alloc(num_nodes);
			g->node2name = node2name;
		}
		nodes_fin = 1;

		/* load each n1-n2 pair */
		while(1) {
			eat_space();
			return_at_eol;
			n1 = token_node(0);
			c = fgetc(f);
			if (c != '-') {
				fprintf(stderr, "Error: expected '-' between nodes in conn list (got '%c' instead)\n", c);
				abort();
			}
			n2 = token_node(0);
			gr_add_(g, n1, n2, 1);
		}
	}

	name2node = htsi_alloc(keyhash, strkeyeq);
	node2name[0] = pcb_strdup("(S)"); htsi_set(name2node, node2name[0], 0);
	node2name[1] = pcb_strdup("(T)"); htsi_set(name2node, node2name[1], 1);
	num_nodes = 2;
	nn_net[0] = 0;
	nn_net[1] = 0;
	nn_neut = 0;

	nodes_fin = 0;
	lineno = 0;
	while(!(feof(f))) {
		char cmd[6];
		lineno++;
		eat_space();
		*cmd = '\0';
		fgets(cmd, 5, f);
		if (*cmd == '#') {
			char s[1024]; /* will break for comment lines longer than 1k */
			fgets(s, sizeof(s), f);
			continue;
		}
		if ((*cmd == '\n') || (*cmd == '\t') || (*cmd == '\0'))
			continue;
		if (strcmp(cmd, "net1") == 0)
			load_net(0);
		else if (strcmp(cmd, "net2") == 0)
			load_net(1);
		else if (strcmp(cmd, "neut") == 0)
			load_neut();
		else if (strcmp(cmd, "conn") == 0)
			load_conn();
	}


#ifdef DEBUG_GR
	gr_print(g, stdout, "(input0) ");
	gr_draw(g, "input0", "png");
#endif

	/* preprocessing nets */
	{
		int net, node;

#if 0
/* this optimization looked like a good idea but fails for O.in because
   it's always cheaper to cut T-> and S-> bounding than anything else */
		/* merge net nodes into (S) and (T) */
		for(net = 0; net < 2; net++)
			for(node = 0; node < nn_net[net]; node++)
				gr_merge_nodes(g, net, nets[net][node]);

		/* make sure (S) and (T) connections are heavy */
		for(net = 0; net < 2; net++)
			for(node = 0; node < g->n; node++)
				if (gr_get_(g, net, node))
					gr_add_(g, net, node, NET_EDGES);
#endif

		/* make sure (S) and (T) connections are heavy */
		for(net = 0; net < 2; net++)
			for(node = 0; node < nn_net[net]; node++)
				gr_add_(g, net, nets[net][node], NET_EDGES);
	}

#ifdef DEBUG_GR
	gr_print(g, stdout, "(input1) ");
	gr_draw(g, "input1", "png");
#endif

	return g;
}

