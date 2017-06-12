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


static const char *templ_gxyrs_hdr =
	"#gxyrs version 1.0\n"
	"#Date: %UTC%\n"
	"#Author: %author%\n"
	"#Title: %title% - pcb-rnd gxyrs\n"
	"#Placement data:\n"
	"#Designator X-Loc Y-Loc Rotation Side Type X-Size Y-Size Value Footprint\n"
	"# --------------------------------------------\n";

static const char *templ_gxyrs_elem =
	"%elem.name_% %elem.x% %elem.y% %elem.rot% %elem.side% %elem.pad_width% %elem.pad_height% %elem.value_% %elem.descr_%\n";

static const char *templ_macrofab_hdr =
	"#pcb-rnd Macrofab export version 1.0\n"
	"#Date: %UTC%\n"
	"#Author: %author%\n"
	"#Title: %title% - pcb-rnd Macrofab export\n"
	"#Placement data:\n"
	"#Designator X-Loc Y-Loc Rotation Side Type X-Size Y-Size Value Footprint Type\n"
	"# --------------------------------------------\n";

static const char *templ_macrofab_elem =
        "%elem.name_%\t%elem.x%\t%elem.y%\t%elem.side270-rot%\t%elem.num-side%\t%elem.pad_width%\t%elem.pad_height%\t%elem.value_%\t%elem.descr_%\t%elem.smdvsthru%\n";

static const char *templ_TM220TM240_hdr = 
	"#pcb-rnd TM220A/TM240A xyrs version 1.0\n"
	"#Date: %UTC%\n"
	"#Author: %author%\n"
	"#Title: %title% - pcb-rnd gxyrs\n"
	"#Placement data:\n"
	"#RefDes, top/bottom, X, Y, rotation\n"
	"# X,Y in %suffix%.  rotation in degrees.\n"
	"# --------------------------------------------\n";

static const char *templ_TM220TM240_elem =
	"%elem.name%,%elem.side%,%elem.x%,%elem.y%,%elem.rot%\n";

static const char *templ_KICADPOS_hdr =
	"###pcb-rnd KiCad .pos compatible xyrs version 1.0\n"
	"###Date: %UTC%, Author: %author%, Title: %title% - pcb-rnd gxyrs\n"
	"## Unit = %suffix%., Angle = degrees.\n"
	"#, Ref, Val, Package, PosX, PosY, Rot, Side\n";

static const char *templ_KICADPOS_elem =
	",\"%elem.name%\",\"%elem.value%\",\"%elem.descr%\",%elem.x%,%elem.y%,%elem.rot%,%elem.side%\n";

static const char *templ_NCAP_hdr =
	"ncapture 0\n"
	"4\n"
	"%boardw%\n"
        "%boardh%\n"
	"80hmmm\n"
	"-30hmmm\n";

static const char *templ_NCAP_elem =
	"%elem.element_num%\n"
	"0\n"
	"%elem.pincount%\n"
	"1\n"
	"%elem.name%\n"
	"%elem.90rot%\n"
	"%elem.x%\n"
	"%elem.y%\n"
	"0\n";

static const char *templ_NCAP_pad =
	"%pad.netname%\n";
