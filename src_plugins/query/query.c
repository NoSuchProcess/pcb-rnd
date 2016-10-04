#include "config.h"
#include "global.h"
#include "conf.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "hid_init.h"
#include "hid_attrib.h"

static const char *query_cookie = "query plugin";

void qry_error(const char *err)
{
	pcb_trace("qry_error: %s\n", err);
}

int qry_wrap()
{
	return 1;
}

/*
static void hid_query_uninit(void)
{
}

*/
pcb_uninit_t hid_query_init(void)
{
/*	return hid_query_uninit;*/
	return NULL;
}
