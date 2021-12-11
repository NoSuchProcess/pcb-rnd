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
