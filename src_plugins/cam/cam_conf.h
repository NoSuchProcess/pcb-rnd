#ifndef PCB_CAM_CONF_H
#define PCB_CAM_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			CFT_LIST jobs;         /* named cam scripts */
		} cam;
	} plugins;
} conf_cam_t;

#endif
