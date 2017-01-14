/* hbox/vbox creation, similar to gtk2's */
#ifdef PCB_GTK3
static inline GtkWidget *gtkc_hbox_new(gboolean homogenous, gint spacing)
{
	GtkBox *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
	gtk_box_set_homogenous(box, homogenous);
	return GTK_WIDGET(box);
}

static inline GtkWidget *gtkc_vbox_new(gboolean homogenous, gint spacing)
{
	GtkBox *box = gtk_box_new(TK_ORIENTATION_VERTICAL, spacing);
	gtk_box_set_homogenous(box, homogenous);
	return GTK_WIDGET(box);
}

#else
/* gtk2 */

static inline GtkWidget *gtkc_hbox_new(gboolean homogenous, gint spacing)
{
	return gtk_hbox_new(homogenous, spacing);
}

static inline GtkWidget *gtkc_vbox_new(gboolean homogenous, gint spacing)
{
	return gtk_vbox_new(homogenous, spacing);
}

#endif

