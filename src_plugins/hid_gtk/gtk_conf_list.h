#include <gtk/gtk.h>

typedef struct gtk_conf_list_s {
	int num_cols;               /* number of visible columns */
	const char **col_names;     /* header for each visible column */
	int col_data, col_src;      /* column index where list data and list source (lihata path) should be automatically added (-1 means off) */
	int reorder;                /* 0 to disable reordering */

	/* callback to fill in non-data, non-src cells */
	char *(*get_misc_col_data)(int row, int col, void *item); /* free()'d after the call; NULL=empty */

	/* if not NULL, allow the file chooser to be invoked on the data cells */
	const char *file_chooser_title; 

	/* if the file chooser ran, filter the name through this function before inserting in the table */
	const char *(*file_chooser_postproc)(char *path);

	/* -- internal -- */
	GtkListStore *l;
	GtkWidget *t;
	int editing;
} gtk_conf_list_t;

GtkWidget *gtk_conf_list_widget(gtk_conf_list_t *cl);
