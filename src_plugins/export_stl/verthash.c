#include <genvector/vtl0.h>
#include <librnd/core/vtc0.h>

typedef struct {
	rnd_coord_t x, y, z;
} vertex_t;

typedef vertex_t htvx_key_t;
typedef long int htvx_value_t;
#define HT(x) htvx_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT


typedef struct {
	htvx_t vxhash;
	vtc0_t vxcoords;
	vtl0_t triangles;
	long next_id;
	long next_obj;
} verthash_t;


unsigned vxkeyhash(vertex_t key)
{
	return key.x ^ key.y ^ key.z;
}

int vxkeyeq(const vertex_t a, const vertex_t b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

static void verthash_init(verthash_t *vh)
{
	htvx_init(&vh->vxhash, vxkeyhash, vxkeyeq);
	vtc0_init(&vh->vxcoords);
	vtl0_init(&vh->triangles);
	vh->next_id = 0;
	vh->next_obj = 2;
}

static void verthash_uninit(verthash_t *vh)
{
	vtl0_uninit(&vh->triangles);
	vtc0_uninit(&vh->vxcoords);
	htvx_uninit(&vh->vxhash);
}

static long verthash_add_vertex(verthash_t *vh, rnd_coord_t x, rnd_coord_t y, rnd_coord_t z)
{
	long id;
	vertex_t vx;
	htvx_entry_t *e;
	
	vx.x = x; vx.y = y; vx.z = z;
	e = htvx_getentry(&vh->vxhash, vx);
	if (e != 0)
		return e->value;

	id = vh->next_id;
	vh->next_id++;
	htvx_set(&vh->vxhash, vx, id);
	vtc0_append(&vh->vxcoords, x);
	vtc0_append(&vh->vxcoords, y);
	vtc0_append(&vh->vxcoords, z);
	return id;
}

static void verthash_add_triangle(verthash_t *vh, long v1, long v2, long v3)
{
	vtl0_append(&vh->triangles, v1);
	vtl0_append(&vh->triangles, v2);
	vtl0_append(&vh->triangles, v3);
}

static void verthash_add_triangle_coord(verthash_t *vh, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t z1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t z2, rnd_coord_t x3, rnd_coord_t y3, rnd_coord_t z3)
{
	long v1, v2, v3;
	v1 = verthash_add_vertex(vh, x1, y1, z1);
	v2 = verthash_add_vertex(vh, x2, y2, z2);
	v3 = verthash_add_vertex(vh, x3, y3, z3);
	verthash_add_triangle(vh, v1, v2, v3);
}

static void verthash_new_obj(verthash_t *vh, float r, float g, float b)
{
	vtl0_append(&vh->triangles, -vh->next_obj);
	vh->next_obj++;

	vtl0_append(&vh->triangles, rnd_round(r*1000000));
	vtl0_append(&vh->triangles, rnd_round(g*1000000));
	vtl0_append(&vh->triangles, rnd_round(b*1000000));
}

/* The following functions implement the standard API of export_stl for building
   a verthash so that the export format needs to deal with verthash->output
   conversion only */

static verthash_t verthash;

static void vhs_print_horiz_tri(FILE *f, fp2t_triangle_t *t, int up, rnd_coord_t z)
{
	if (up) {
		verthash_add_triangle_coord(&verthash,
			t->Points[0]->X, t->Points[0]->Y, z,
			t->Points[1]->X, t->Points[1]->Y, z,
			t->Points[2]->X, t->Points[2]->Y, z
			);
	}
	else {
		verthash_add_triangle_coord(&verthash,
			t->Points[2]->X, (rnd_coord_t)t->Points[2]->Y, z,
			t->Points[1]->X, (rnd_coord_t)t->Points[1]->Y, z,
			t->Points[0]->X, (rnd_coord_t)t->Points[0]->Y, z
			);
	}
}

static void vhs_print_vert_tri(FILE *f, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t z0, rnd_coord_t z1)
{
	verthash_add_triangle_coord(&verthash,
		x2, y2, z1,
		x1, y1, z1,
		x1, y1, z0
		);

	verthash_add_triangle_coord(&verthash,
		x2, y2, z1,
		x1, y1, z0,
		x2, y2, z0
		);
}


static void vhs_print_facet(FILE *f, stl_facet_t *head, double mx[16], double mxn[16])
{
	double v[3], p[3];
	long vert[3];
	int n;

	for(n = 0; n < 3; n++) {
		p[0] = head->vx[n]; p[1] = head->vy[n]; p[2] = head->vz[n];
		v_transform(v, p, mx);
		vert[n] = verthash_add_vertex(&verthash,
			(rnd_coord_t)RND_MM_TO_COORD(v[0]),
			(rnd_coord_t)RND_MM_TO_COORD(v[1]),
			(rnd_coord_t)RND_MM_TO_COORD(v[2])
			);
	}

	verthash_add_triangle(&verthash, vert[0], vert[1], vert[2]);
}

static void vhs_new_obj(float r, float g, float b)
{
	verthash_new_obj(&verthash, r, g, b);
}
