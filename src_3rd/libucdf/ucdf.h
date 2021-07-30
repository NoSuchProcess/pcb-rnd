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
	UCDF_ERR_BAD_HDR,
	UCDF_ERR_BAD_MSAT,
	UCDF_ERR_BAD_SAT,
	UCDF_ERR_BAD_SSAT,
	UCDF_ERR_BAD_DIRCHAIN,
	UCDF_ERR_BAD_MALLOC
} ucdf_error_t;

extern const char *ucdf_error_str[]; /* indexed by ucdf_error_t */

typedef enum {
	UCDF_DE_EMPTY = 0,
	UCDF_DE_DIR = 1,
	UCDF_DE_FILE = 2,    /* "stream" */
	UCDF_DE_LOCK = 3,
	UCDF_DE_PROP = 4,
	UCDF_DE_ROOT = 5
} ucdf_detype_t;

typedef struct ucdf_direntry_s ucdf_direntry_t;

struct ucdf_direntry_s {
	char name[32];         /* converted to ASCII */
	ucdf_detype_t type;
	long size;
	unsigned is_short:1;   /* if set: file is short, using short sectors */
	long first;            /* sector id (long or short) of the first sector */
	ucdf_direntry_t *parent;
	ucdf_direntry_t *children;
	ucdf_direntry_t *next; /* within children */
};

typedef struct {
	ucdf_error_t error; /* last error experienced in parsing */
	int file_ver, file_rev, sect_size, short_sect_size;

	unsigned litend:1;  /* set if file is little endian */

	ucdf_direntry_t *root;

	/* cache/interanl */
	FILE *f; /* the file we are reading from */
	int ssz, sssz;
	long sat_len;
	long dir_first;            /* first sector ID of the directory stream */
	long ssat_len, ssat_first; /* short allocation table */
	long msat_len, msat_first; /* master allocation table */

	long *msat;                /* the master SAT read into memory */
	long *sat;                 /* the whole SAT assembled and read into memory; entries are indexed by sector ID and contain the next sector ID within the chain */
	long *ssat;                /* the whole Short-SAT assembled and read into memory */
} ucdf_file_t;

int ucdf_open(ucdf_file_t *ctx, const char *path);

