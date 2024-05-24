#ifndef PCB_DRAW_PNP_CONF_H
#define PCB_DRAW_PNP_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			const struct {
				RND_CFT_INTEGER scale;            /* Refdes text scale in integer percent (100 being normal text size) */
				RND_CFT_COORD thickness;          /* Refdes text stroke thickness; 0 means auto */
			} text;
			RND_CFT_COORD frame_thickness;      /* Thickness of the rectangular frame drawn around subcircuits; 0 means do not draw a frame */
			RND_CFT_COORD term1_diameter;       /* Diameter of the dot placed on terminal 1; 0 means no dot is placed */
		} draw_pnp;
	} plugins;
} conf_draw_pnp_t;

#endif
