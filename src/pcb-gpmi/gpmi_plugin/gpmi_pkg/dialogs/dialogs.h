#include <gpmi.h>
#include "src/global.h"
#define FROM_PKG
#include "hid/hid.h"

/* Filter on what files a file select dialog should list */
typedef enum dialog_fileselect_e {
	FS_NONE      = 0, /* none of the below */
	FS_READ      = 1, /* when the selected file will be read, not written (HID_FILESELECT_READ) */
	FS_NOT_EXIST = 2, /* the function calling hid->fileselect will deal with the case when the selected file already exists.  If not given, then the gui will prompt with an "overwrite?" prompt.  Only used when writing. (HID_FILESELECT_MAY_NOT_EXIST)  */
	FS_TEMPLATE  = 4  /* the call is supposed to return a file template (for gerber output for example) instead of an actual file.  Only used when writing. (HID_FILESELECT_IS_TEMPLATE) */
} dialog_fileselect_t;
gpmi_keyword *kw_dialog_fileselect_e; /* of dialog_fileselect_t */

/* Append a msg to the log (log window and/or stderr). */
void dialog_log(const char *msg);

/* Ask the user for confirmation (usually using a popup). Returns 0 for
   cancel and 1 for ok.
   Arguments:
     msg: message to the user
     ok: label of the OK button
     cancel: label of the cancel button
  Arguments "ok" and "cancel" may be empty (or NULL) in which
  case the GUI will use the default (perhaps localized) labels for
  those buttons. */
int dialog_confirm(const char *msg, const char *ok, const char *cancel);


/* Pop up a report dialog.
   Arguments:
     title: title of the window
     msg: message */
void dialog_report(const char *title, const char *msg);

/* Ask the user to input a string (usually in a popup).
   Arguments:
     msg: message or question text
     default_: default answer (this may be filled in on start)
   Returns the answer. */
dynamic char *dialog_prompt(const char *msg, const char *default_);

/* Pops up a file selection dialog.
   Arguments:
     title: window title
     descr: description
     default_file_
     default_ext: default file name extension
     history_tag
     flags: one or more flags (see below)
   Returns the selected file or NULL (empty). */
dynamic char *dialog_fileselect(const char *title, const char *descr, char *default_file_, char *default_ext, const char *history_tag, multiple dialog_fileselect_t flags);

/* Audible beep */
void dialog_beep(void);

/* Request the GUI hid to draw a progress bar.
   Arguments:
     int so_far: achieved state
     int total: maximum state
     const char *message: informs the users what they are waiting for
   If so_far is bigger than total, the progress bar is closed.
   Returns nonzero if the user wishes to cancel the operation.
*/
int dialog_progress(int so_far, int total, const char *message);


/* Pop up an attribute dialog; content (widgets) of the dialog box are coming
   from hid (see the hid package).
   Arguments:
     hid: widgets
     title: title of the window
     descr: descripting printed in the dialog */
int dialog_attribute(hid_t *hid, const char *title, const char *descr);
