/* Has to be a separate compilation unit to firewall all the extra #defines
   in librnd/config.h */

#include <stdlib.h>
#include "log.h"
#include <librnd/config.h>

unsigned long librnd_ver_get(int *major, int *minor, int *patch)
{
	if (major != NULL) *major = ((unsigned long)RND_API_VER & 0xFF0000UL) >> 16;
	if (minor != NULL) *minor = ((unsigned long)RND_API_VER & 0x00FF00UL) >> 8;
	if (patch != NULL) *patch = ((unsigned long)RND_API_VER & 0x0000FFUL);
	return RND_API_VER;
}

void librnd_ver_req_min(int exact_major, int min_minor)
{
	int major, minor, patch;
	librnd_ver_get(&major, &minor, &patch);
	if (major != exact_major) {
		report("\n\nERROR: Your currently installed librnd version is %d.%d.%d but %d.x.x (>= %d.%d.0 and < %d.0.0) is required (major version mismatch)\n\n", major, minor, patch, exact_major, exact_major, min_minor, exact_major+1);
		exit(1);
	}
	if (minor < min_minor) {
		report("\n\nERROR: Your currently installed librnd version is %d.%d.%d but %d.%d.x (>= %d.%d.0 and < %d.0.0) is required (minor version mismatch)\n\n", major, minor, patch, exact_major, min_minor, exact_major, min_minor, exact_major+1);
		exit(1);
	}
}
