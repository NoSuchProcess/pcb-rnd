#ifndef PCB_POLYHELP_TRIANGULATE_H
#define PCB_POLYHELP_TRIANGULATE_H

#include <stdlib.h>

typedef unsigned char fp2t_u8_t;
typedef long fp2t_i32_t;
typedef int fp2t_b32_t;
typedef unsigned long fp2t_u32_t;
typedef size_t fp2t_uxx_t;
typedef int fp2t_bxx_t;
typedef size_t fp2t_imm_t;
typedef size_t fp2t_umm_t;

#define FP2T_INTERNAL extern
#include "../src_3rd/fast89-poly2tri/fast89_poly2tri.h"

#endif
