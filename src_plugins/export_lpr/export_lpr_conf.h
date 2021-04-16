#ifndef PCB_EXPORT_LPR_CONF_H
#define PCB_EXPORT_LPR_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_REAL default_xcalib;   /* Initialize xcalib to this value on start - should be a printer specific calibration value */
			RND_CFT_REAL default_ycalib;   /* Initialize ycalib to this value on start - should be a printer specific calibration value */
		} export_lpr;
	} plugins;
} conf_export_lpr_t;

#endif
