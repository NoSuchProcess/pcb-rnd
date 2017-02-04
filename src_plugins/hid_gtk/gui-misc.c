/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "conf_core.h"

#include "math_helper.h"
#include "crosshair.h"
#include "pcb-printf.h"
#include "compat_nls.h"

/*FIXME: needed for ghidgui , but need to be removed */
#include "gui.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";
