#include <gtk/gtk.h>


/* frame_border_width - border around outside of frame.
   |  vbox_pad - pad between widgets to be packed in returned vbox.
   |  vbox_border_width - border between returned vbox and frame.
*/
GtkWidget *ghid_framed_vbox(GtkWidget * box, gchar * label, gint frame_border_width,
														gboolean frame_expand, gint vbox_pad, gint vbox_border_width);

GtkWidget *ghid_framed_vbox_end(GtkWidget * box, gchar * label, gint frame_border_width,
																gboolean frame_expand, gint vbox_pad, gint vbox_border_width);

GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header,
															gint header_pad, gint box_pad, gboolean pack_start, gboolean bottom_pad);

GtkWidget *ghid_scrolled_vbox(GtkWidget * box, GtkWidget ** scr, GtkPolicyType h_policy, GtkPolicyType v_policy);
GtkWidget *ghid_scrolled_text_view(GtkWidget * box, GtkWidget ** scr, GtkPolicyType h_policy, GtkPolicyType v_policy);

GtkTreeSelection *ghid_scrolled_selection(GtkTreeView * treeview, GtkWidget * box,
																					GtkSelectionMode s_mode,
																					GtkPolicyType h_policy, GtkPolicyType v_policy,
																					void (*func_cb) (GtkTreeSelection *, gpointer), gpointer data);
