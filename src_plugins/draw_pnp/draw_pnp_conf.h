#ifndef PCB_DRAW_PNP_CONF_H
#define PCB_DRAW_PNP_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			const struct {
				RND_CFT_INTEGER scale;
				RND_CFT_COORD thickness;
			} text;
			RND_CFT_COORD frame_thickness;
			RND_CFT_COORD term1_diameter;
		} draw_pnp;
	} plugins;
} conf_draw_pnp_t;

#endif
