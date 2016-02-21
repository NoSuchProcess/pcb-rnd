/*  gui hid/gtk */
/*  gui hid/lesstif */
/*  gui hid/batch */
/*  export hid/ps */
/*  export hid/png */
/*  export hid/gcode */
/*  export hid/nelma */
/* hid/png (export) */
REGISTER_ATTRIBUTES(png_attribute_list)

/* move.c () */
REGISTER_ACTIONS(move_action_list)

/* select_act.c () */
REGISTER_ACTIONS(select_action_list)

/* remove_act.c () */
REGISTER_ACTIONS(remove_action_list)

/* file_act.c () */
REGISTER_ACTIONS(file_action_list)

/* main.c () */
REGISTER_ATTRIBUTES(main_attribute_list)

/* command.c () */
REGISTER_ACTIONS(command_action_list)

/* change_act.c () */
REGISTER_ACTIONS(change_action_list)

/* action.c () */
REGISTER_ACTIONS(action_action_list)

/* hid/ps (export) */
REGISTER_ATTRIBUTES(ps_attribute_list)
REGISTER_ACTIONS(hidps_action_list)
REGISTER_ATTRIBUTES(eps_attribute_list)

/* hid/gtk (gui) */
if ((gui != NULL) && (strcmp(gui->name, "gtk") == 0)) {
REGISTER_ATTRIBUTES(ghid_attribute_list)
REGISTER_ACTIONS(gtk_topwindow_action_list)
REGISTER_ACTIONS(ghid_menu_action_list)
REGISTER_ACTIONS(ghid_netlist_action_list)
REGISTER_ACTIONS(ghid_log_action_list)
REGISTER_ACTIONS(ghid_main_action_list)
REGISTER_FLAGS(ghid_main_flag_list)

}
/* hid/nelma (export) */
REGISTER_ATTRIBUTES(nelma_attribute_list)

/* gui_act.c () */
REGISTER_ACTIONS(gui_action_list)

/* report.c () */
REGISTER_ACTIONS(report_action_list)

/* netlist.c () */
REGISTER_ACTIONS(netlist_action_list)

/* misc.c () */
REGISTER_ACTIONS(misc_action_list)

/* flags.c () */
REGISTER_FLAGS(flags_flag_list)

/* import_sch.c () */
REGISTER_ACTIONS(import_sch_action_list)

/* buffer.c () */
REGISTER_ACTIONS(rotate_action_list)

/* hid/gcode (export) */
REGISTER_ATTRIBUTES(gcode_attribute_list)

/* hid/batch (gui) */
if ((gui != NULL) && (strcmp(gui->name, "batch") == 0)) {
REGISTER_ACTIONS(batch_action_list)

}
/* hid/lesstif (gui) */
if ((gui != NULL) && (strcmp(gui->name, "lesstif") == 0)) {
REGISTER_ACTIONS(lesstif_library_action_list)
REGISTER_ACTIONS(lesstif_menu_action_list)
REGISTER_FLAGS(lesstif_main_flag_list)
REGISTER_ATTRIBUTES(lesstif_attribute_list)
REGISTER_ACTIONS(lesstif_main_action_list)
REGISTER_ACTIONS(lesstif_styles_action_list)
REGISTER_ACTIONS(lesstif_dialog_action_list)
REGISTER_ACTIONS(lesstif_netlist_action_list)

}
/* rats_act.c () */
REGISTER_ACTIONS(rats_action_list)

/* plugins.c () */
REGISTER_ACTIONS(plugins_action_list)

/* undo_act.c () */
REGISTER_ACTIONS(undo_action_list)

