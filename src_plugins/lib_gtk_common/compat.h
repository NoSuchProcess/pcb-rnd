/* hbox/vbox creation, similar to gtk2's, but we don't use homogenous */
#ifdef PCB_GTK3
#	define GTKC_HBOX_NEW(spacing) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)
#	define GTKC_VBOX_NEW(spacing) gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)
#else
#	define GTKC_HBOX_NEW(spacing) gtk_hbox_new(false, spacing)
#	define GTKC_VBOX_NEW(spacing) gtk_vbox_new(false, spacing)
#endif

