#include <gpmi.h>

int PKG_FUNC(init)()
{
	return 0;
}

void PKG_FUNC(uninit)() {
}

void PKG_FUNC(register)() {
}

int PKG_FUNC(checkver)(int requested) {
	if (requested <= 1)
		return 0;

	return -1;
}
