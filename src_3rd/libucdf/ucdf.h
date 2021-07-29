#include <stdio.h>

typedef enum {
	UCDF_SECT_FREE = -1,
	UCDF_SECT_EOC  = -2, /* end of chain */
	UCDF_SECT_SAT  = -3, /* sector allocation table */
	UCDF_SECT_MSAT = -4, /* master sector allocation table */
} ucdf_spec_sect_id_t;

typedef enum {
	UCDF_ERR_SUCCESS = 0,
	UCDF_ERR_OPEN,
	UCDF_ERR_READ,
	UCDF_ERR_BAD_ID,
	UCDF_ERR_BAD_HDR
} ucdf_error_t;

extern const char *ucdf_error_str[]; /* indexed by ucdf_error_t */

typedef struct {
	ucdf_error_t error; /* last error experienced in parsing */
	int file_ver, file_rev, sect_size, short_sect_size;

	unsigned litend:1;  /* set if file is little endian */

	/* cache/interanl */
	FILE *f; /* the file we are reading from */
	int ssz, sssz;
	long sat_len, sat_first;
	long ssat_len, ssat_first; /* short allocation table */
	long msat_len, msat_first; /* master allocation table */
} ucdf_file_t;

int ucdf_open(ucdf_file_t *ctx, const char *path);

