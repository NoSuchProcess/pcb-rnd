#include <gtk/gtk.h>

/* Builds a vertical box, surrounded by a labeled frame
    frame_border_width: border around outside of frame.
    vbox_pad: pad between widgets to be packed in returned vbox.
    vbox_border_width: border between returned vbox and frame. */
GtkWidget *ghid_framed_vbox(GtkWidget * box, gchar * label, gint frame_border_width,
														gboolean frame_expand, gint vbox_pad, gint vbox_border_width);

/* Same as ghid_framed_vbox(), but packing at the end */
GtkWidget *ghid_framed_vbox_end(GtkWidget * box, gchar * label, gint frame_border_width,
																gboolean frame_expand, gint vbox_pad, gint vbox_border_width);

GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header,
															gint header_pad, gint box_pad, gboolean pack_start, gboolean bottom_pad);

GtkTreeSelection *ghid_scrolled_selection(GtkTreeView * treeview, GtkWidget * box,
																					GtkSelectionMode s_mode,
																					GtkPolicyType h_policy, GtkPolicyType v_policy,
																					void (*func_cb) (GtkTreeSelection *, gpointer), gpointer data);
