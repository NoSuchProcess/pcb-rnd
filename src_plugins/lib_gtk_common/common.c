#include "config.h"

#include "crosshair.h"

#include "gui.h"
#include "common.h"

#include "dlg_topwin.h"
#include "in_keyboard.h"
#include "hid_gtk_conf.h"

GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

