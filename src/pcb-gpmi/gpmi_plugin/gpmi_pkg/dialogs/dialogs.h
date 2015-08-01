#include <gpmi.h>
#include "src/global.h"
#define FROM_PKG
#include "hid/hid.h"

typedef enum dialog_fileselect_e {
	FS_NONE      = 0,
	FS_READ      = 1, /* HID_FILESELECT_READ */
	FS_NOT_EXIST = 2, /* HID_FILESELECT_MAY_NOT_EXIST */
	FS_TEMPLATE  = 4  /* HID_FILESELECT_IS_TEMPLATE */
} dialog_fileselect_t;
gpmi_keyword *kw_dialog_fileselect_e; /* of dialog_fileselect_t */

/* push a message to the log */
void dialog_log(const char *msg);

/* returns 0 for cancel, 1 for ok */
int dialog_confirm(const char *msg, const char *ok, const char *cancel);

void dialog_report(const char *title, const char *msg);

dynamic char *dialog_prompt(const char *msg, const char *default_);

dynamic char *dialog_fileselect(const char *title, const char *descr, char *default_file_, char *default_ext, const char *history_tag, multiple dialog_fileselect_t flags);

void dialog_beep(void);

int dialog_progress(int so_far, int total, const char *message);

int dialog_attribute(hid_t *hid, const char *title, const char *descr);
