/* Recurring legacy rtree operations for autoroute and autoplace
   DO NOT USE IN NEW CODE */

#define RTREE_EMPTY(rt) (((rt) == NULL) || ((rt)->size == 0))

RND_INLINE void r_free_tree_data(rnd_rtree_t *rtree, void (*free)(void *ptr))
{
	rnd_rtree_it_t it;
	void *o;

	for(o = rnd_rtree_all_first(&it, rtree); o != NULL; o = rnd_rtree_all_next(&it))
		free(o);
}

RND_INLINE void r_insert_array(rnd_rtree_t *rtree, const rnd_box_t *boxlist[], rnd_cardinal_t len)
{
	rnd_cardinal_t n;

	if (len == 0)
		return;

	assert(boxlist != 0);

	for(n = 0; n < len; n++)
		rnd_rtree_insert(rtree, (void *)boxlist[n], (rnd_rtree_box_t *)boxlist[n]);
}
