#ifndef OPC89_H_INCLUDED
#define OPC89_H_INCLUDED

#include <librnd/core/config.h>
#define G2D_INLINE RND_INLINE

#ifdef __OPC89__

/* Being compiled with opc89: use attributes to mark types and functions */
#	define funcops __attribute__((opc89_ops))
#	define opfunc __attribute__((opc89_opfunc))

/* make sure asserts are kept as is */
#define assert(a)

#else

/* Being compiled with a c compiler: make opc marks invisible */
#	define funcops
#	define opfunc

#endif

#endif
