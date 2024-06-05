#define HT(x) htendp_ ## x
#include <genht/ht.c>
#include <genht/hash.h>
#undef HT

static unsigned endphash(rnd_cheap_point_t k)
{
	rnd_coord_t c = k.X ^ -k.Y;
	if (sizeof(rnd_coord_t) == sizeof(long))
		return longhash((long)c);
	return jenhash(&c, sizeof(c));
}

static int endpkeyeq(rnd_cheap_point_t a, rnd_cheap_point_t b)
{
	return (a.X == b.X) && (a.Y == b.Y);
}



