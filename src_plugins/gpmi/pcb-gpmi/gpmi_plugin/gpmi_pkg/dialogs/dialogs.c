#include "src/hid_attrib.h"
#include "src/error.h"
#include "dialogs.h"

extern pcb_hid_t *pcb_gui;

void dialog_log(const char *msg)
{
	if (pcb_gui == NULL)
		fprintf(stderr, "couldn't find gui for log: \"%s\"\n", msg);
	else
		pcb_message(PCB_MSG_DEFAULT, "%s", msg);
}

#define empty(s) (((s) == NULL) || ((*s) == '\0'))
int dialog_confirm(const char *msg, const char *ok, const char *cancel)
{
	if (pcb_gui == NULL) {
		fprintf(stderr, "couldn't find gui for dialog_confirm: \"%s\"\n", msg);
		return -1;
	}

	if (empty(ok))
		ok = NULL;
	if (empty(cancel))
		cancel = NULL;

	return pcb_gui->confirm_dialog(msg, cancel, ok, NULL);
}
#undef empty

void dialog_report(const char *title, const char *msg)
{
	if (pcb_gui == NULL)
		fprintf(stderr, "couldn't find gui for dialog_report: \"%s\" \"%s\"\n", title, msg);
	else
		pcb_gui->report_dialog(title, msg);
}

dynamic char *dialog_prompt(const char *msg, const char *default_)
{
	if (pcb_gui == NULL) {
		fprintf(stderr, "couldn't find gui for dialog_prompt: \"%s\" \"%s\"\n", msg, default_);
		return NULL;
	}
	else
		return pcb_gui->prompt_for(msg, default_);
}

dynamic char *dialog_fileselect(const char *title, const char *descr, char *default_file, char *default_ext, const char *history_tag, multiple dialog_fileselect_t flags)
{
	if (pcb_gui == NULL) {
		fprintf(stderr, "couldn't find gui for dialog_fileselect\n");
		return NULL;
	}
	else
		return pcb_gui->fileselect(title, descr, default_file, default_ext, history_tag, flags);
}

void dialog_beep(void)
{
	if (pcb_gui == NULL)
		fprintf(stderr, "couldn't find gui for dialog_beep\n");
	else
		pcb_gui->beep();
}

int dialog_progress(int so_far, int total, const char *message)
{
	if (pcb_gui == NULL) {
		fprintf(stderr, "couldn't find gui for dialog_process: %d/%d \"%s\"\n", so_far, total, message);
		return -1;
	}
	return pcb_gui->progress(so_far, total, message);
}

int dialog_attribute(gpmi_hid_t *hid, const char *title, const char *descr)
{
	if (pcb_gui == NULL) {
		fprintf(stderr, "couldn't find gui for dialog_attribute: \"%s\" \"%s\"\n", title, descr);
		return -1;
	}

	if (hid->result != NULL) {
		/* TODO: free string fields to avoid memleaks */
	}
	else
		hid->result = calloc(sizeof(pcb_hid_attribute_t), hid->attr_num);

	return pcb_gui->attribute_dialog(hid->attr, hid->attr_num, hid->result, title, descr);
}
