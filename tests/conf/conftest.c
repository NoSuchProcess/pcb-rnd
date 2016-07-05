#include <stdarg.h>
#include "conf.h"

void Message(const char *Format, ...)
{
	va_list args;

	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
}

void hid_notify_conf_changed(void)
{

}

void hid_usage_option(const char *name, const char *help)
{
}

int main()
{
	conf_init();
	conf_core_init();
	conf_core_postproc();

	conf_load_all(NULL, NULL);
	conf_uninit();
}
