static const char *templ_xy_hdr =
	"# $Id$\n"
	"# PcbXY Version 1.0\n"
	"# Date: %UTC%\n"
	"# Author: %author%\n"
	"# Title: %title% - PCB X-Y\n"
	"# RefDes, Description, Value, X, Y, rotation, top/bottom\n"
	"# X,Y in %suffix%.  rotation in degrees.\n"
	"# --------------------------------------------\n";

static const char *templ_xy_elem =
	"%elem.name%,\"%elem.descr%\",\"%elem.value%\",%elem.x%,%elem.y%,%elem.rot%,%elem.side%\n";
