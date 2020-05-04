#include <stdio.h>
#include <string.h>

#include <librnd/core/base64.h>

int verbose = 0;

int encode(const char *str)
{
	size_t ilen, olen, len;
	char bin[512];
	unsigned char plain[256];

	memset(plain, '#', sizeof(plain));

	ilen = strlen(str);
	len = rnd_base64_bin2str(bin, sizeof(bin), (unsigned char *)str, ilen);
	if (len >= 0) {
		bin[len] = '\0';
		if (verbose) printf("'%s'/%ld -> '%s'/%ld\n", str, (long)ilen, bin, (long)len);
		olen = rnd_base64_str2bin(plain, sizeof(plain), bin, len);
		if (olen >= 0) {
			plain[olen] = '\0';
			if (verbose) printf(" -> '%s'/%ld\n", plain, (long)olen);
		}
	}

	if ((strcmp((char *)plain, str) != 0) || (ilen != olen))
		return -1;
	return 0;
}

int main()
{
	int err = 0;

	err |= encode("Man");


/* Example from wikipedia:
20 any carnal pleasure. 28 YW55IGNhcm5hbCBwbGVhc3VyZS4= 1
19 any carnal pleasure 28 YW55IGNhcm5hbCBwbGVhc3VyZQ== 2
18 any carnal pleasur 24 YW55IGNhcm5hbCBwbGVhc3Vy 0
17 any carnal pleasu 24 YW55IGNhcm5hbCBwbGVhc3U= 1
16 any carnal pleas 24 YW55IGNhcm5hbCBwbGVhcw== 2
*/

	err |= encode("any carnal pleasure.");
	err |= encode("any carnal pleasure");
	err |= encode("any carnal pleasur");
	err |= encode("any carnal pleasu");
	err |= encode("any carnal pleas");

	if (err != 0)
		fprintf(stderr, "ERROR: base64 test failed\n");

	return err;
}
