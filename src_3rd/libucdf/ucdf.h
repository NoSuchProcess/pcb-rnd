#include <stdio.h>

typedef enum {
	UCDF_SECT_FREE = -1,
	UCDF_SECT_EOC  = -2, /* end of chain */
	UCDF_SECT_SAT  = -3, /* sector allocation table */
	UCDF_SECT_MSAT = -4  /* master sector allocation table */
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

extern const char *ucdf_error_str[]; /* indexed by ucdf_error_t (typically ctx->error) */

typedef enum {
	UCDF_DE_EMPTY = 0,
	UCDF_DE_DIR = 1,
	UCDF_DE_FILE = 2,    /* "stream" */
	UCDF_DE_LOCK = 3,
	UCDF_DE_PROP = 4,
	UCDF_DE_ROOT = 5
} ucdf_detype_t;

typedef struct ucdf_ctx_s ucdf_ctx_t;
typedef struct ucdf_direntry_s ucdf_direntry_t;
typedef struct ucdf_file_s ucdf_file_t;

struct ucdf_direntry_s {
	char name[32];         /* converted to ASCII */
	ucdf_detype_t type;
	long size;
	unsigned is_short:1;   /* if set: file is short, using short sectors */
	long first;            /* sector id (long or short) of the first sector */
	ucdf_direntry_t *parent;
	ucdf_direntry_t *children;
	ucdf_direntry_t *next; /* within children */

	void *user_data;
};

struct ucdf_file_s {
	ucdf_ctx_t *ctx;
	ucdf_direntry_t *de;
	long stream_offs;          /* byte offset within the file */
	long sect_id;              /* long: the sector id (absolute sector address) we are in */
	long sect_offs;            /* byte offset within the sector */
};

struct ucdf_ctx_s {
	ucdf_error_t error; /* last error experienced in parsing */
	int file_ver, file_rev, sect_size, short_sect_size;

	unsigned litend:1;  /* set if file is little endian */

	ucdf_direntry_t *root;

	void *user_data;

	/* cache/interanl */
	FILE *f; /* the file we are reading from */
	int ssz, sssz;
	long sat_len;
	long dir_first;            /* first sector ID of the directory stream */
	long ssat_len, ssat_first; /* short allocation table */
	long msat_len, msat_first; /* master allocation table */
	long long_stream_min_size;

	long *msat;                /* the master SAT read into memory */
	long *sat;                 /* the whole SAT assembled and read into memory; entries are indexed by sector ID and contain the next sector ID within the chain */
	long *ssat;                /* the whole Short-SAT assembled and read into memory */

	/* short sector data is really stored in a long stream; keep a file open on it */
	ucdf_direntry_t ssd_de;
	ucdf_file_t ssd_f;
};

/* Look at a file, try to read some headers (cheap) to decide if path is a
   valid CDF file. Returns 0 on no error (file is CDF), -1 on error. */
int ucdf_test_parse(const char *path);

/* Open and map a CDF file. Returns -1 on error, error code is in ctx->error */
int ucdf_open(ucdf_ctx_t *ctx, const char *path);

/* Free all memory used by ctx */
void ucdf_close(ucdf_ctx_t *ctx);

/* Open a file for read; returns 0 on success */
int ucdf_fopen(ucdf_ctx_t *ctx, ucdf_file_t *fp, ucdf_direntry_t *de);

/* Read at most len bytes from dst. Returns number of bytes read. If stream
   ends short, returns less than len. If stream is already at EOF, returns 0 */
long ucdf_fread(ucdf_file_t *fp, char *dst, long len);

/* Seek to a given byte offset from the beginning of the stream. Returns 0
   on success or -1 on error (in which case no seek is done). At the moment
   only long files are supported. */
int ucdf_fseek(ucdf_file_t *fp, long offs);


