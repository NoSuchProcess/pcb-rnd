#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_INTEGER columns;         /* @usage number of columns for found pin report  */
		} report;
	} plugins;
} conf_report_t;

extern conf_report_t conf_report;

#endif
