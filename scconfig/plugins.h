/******************************************************************************
 Auto-generated by trunk/src_plugins/map_plugins.sh - do NOT edit,
 run make map_plugins in trunk/src/ - to change any of the data below,
 edit trunk/src_plugins/PLUGIN/PLUGIN.pup
******************************************************************************/

plugin_header("\nLibrary plugins:\n")
plugin_def("lib_compat_help",  "#compatibility helper functions", sbuildin, 1)
plugin_def("lib_formula",      "mathematical formulas",           sbuildin, 1)
plugin_def("lib_hid_pcbui",    "common PCB-related GUI elements", sdisable, 0)
plugin_def("lib_netmap",       "map nets and objects",            sdisable, 0)
plugin_def("lib_polyhelp",     "polygon helpers",                 sbuildin, 1)
plugin_def("lib_vfs",          "fetch data for VFS export",       sdisable, 0)

plugin_header("\nFeature plugins:\n")
plugin_def("acompnet",         "net auto-completion",             sdisable, 1)
plugin_def("act_draw",         "action wrappers for drawing",     sbuildin, 1)
plugin_def("act_read",         "action wrappers for data access", sbuildin, 1)
plugin_def("ar_cpcb",          "autoroute with c-pcb",            sbuildin, 1)
plugin_def("ar_extern",        "external autorouter",             sbuildin, 1)
plugin_def("asm",              "assembly GUI",                    sbuildin, 1)
plugin_def("autocrop",         "crop board to fit objects",       sbuildin, 1)
plugin_def("autoplace",        "auto place components",           sbuildin, 1)
plugin_def("autoroute",        "the original autorouter",         sbuildin, 1)
plugin_def("ch_editpoint",     "crosshair: on-point highlight",   sbuildin, 1)
plugin_def("ch_onpoint",       "crosshair: on-point highlight",   sbuildin, 1)
plugin_def("diag",             "diagnostic acts. for devs",       sbuildin, 1)
plugin_def("dialogs",          "HID-independent GUI dialogs",     sdisable, 1)
plugin_def("distalign",        "distribute and align objs",       sbuildin, 1)
plugin_def("djopt",            "djopt board optimization",        sbuildin, 1)
plugin_def("draw_csect",       "draw cross-section (layers)",     sdisable, 1)
plugin_def("draw_fab",         "fab layer in some exports",       sbuildin, 1)
plugin_def("draw_fontsel",     "font selection GUI",              sdisable, 1)
plugin_def("drc_query",        "query() based DRC",               sbuildin, 1)
plugin_def("expfeat",          "experimental features",           sdisable, 1)
plugin_def("extedit",          "edit with external program",      sbuildin, 1)
plugin_def("exto_std",         "standard extended objects",       sbuildin, 1)
plugin_def("fontmode",         "font editor",                     sbuildin, 1)
plugin_def("jostle",           "push lines out of the way",       sbuildin, 1)
plugin_def("millpath",         "calculate toolpath for milling",  sdisable, 1)
plugin_def("mincut",           "minimal cut shorts",              sbuildin, 1)
plugin_def("oldactions",       "old/obsolete actions",            sdisable, 1)
plugin_def("order_pcbway",     "board ordering from PCBWay",      sdisable, 1)
plugin_def("order",            "online board ordering",           sdisable, 1)
plugin_def("polycombine",      "combine selected polygons",       sbuildin, 1)
plugin_def("polystitch",       "stitch polygon at cursor",        sbuildin, 1)
plugin_def("propedit",         "object property editor",          sbuildin, 1)
plugin_def("puller",           "puller",                          sbuildin, 1)
plugin_def("query",            "query language",                  sbuildin, 1)
plugin_def("renumber",         "renumber action",                 sbuildin, 1)
plugin_def("report",           "report actions",                  sbuildin, 1)
plugin_def("rubberband_orig",  "the original rubberband",         sbuildin, 1)
plugin_def("serpentine",       "Serpentine creation",             sdisable, 0)
plugin_def("shand_cmd",        "command shorthands",              sbuildin, 1)
plugin_def("shape",            "generate regular shapes",         sbuildin, 1)
plugin_def("show_netnames",    "display network names over tracks",sbuildin, 1)
plugin_def("sketch_route",     "assisted semi-auto-routing",      sdisable, 0)
plugin_def("smartdisperse",    "netlist based dispenser",         sbuildin, 1)
plugin_def("teardrops",        "draw teardrops on pins",          sbuildin, 1)
plugin_def("tool_std",         "standard tools",                  sbuildin, 1)
plugin_def("vendordrill",      "vendor drill mapping",            sbuildin, 1)

plugin_header("\nFootprint backends:\n")
plugin_def("fp_board",         "footprint library from boards",   sbuildin, 1)
plugin_def("fp_fs",            "filesystem footprints",           sbuildin, 1)
plugin_def("fp_wget",          "web footprints",                  sbuildin, 1)

plugin_header("\nImport plugins:\n")
plugin_def("import_accel_net", "import Accel netlist",            sbuildin, 1)
plugin_def("import_calay",     "import calay .net",               sbuildin, 1)
plugin_def("import_edif",      "import edif",                     sbuildin, 1)
plugin_def("import_fpcb_nl",   "import freepcb netlist",          sbuildin, 1)
plugin_def("import_gnetlist",  "sch import: run gnetlist",        sbuildin, 1)
plugin_def("import_hpgl",      "import HP-GL plot files",         sbuildin, 1)
plugin_def("import_ipcd356",   "import IPC-D-356 Netlist",        sbuildin, 1)
plugin_def("import_ltspice",   "import ltspice .net+.asc",        sbuildin, 1)
plugin_def("import_mentor_sch","import mentor graphics sch",      sbuildin, 1)
plugin_def("import_mucs",      "import mucs routing",             sbuildin, 1)
plugin_def("import_net_action","import net: action script",       sbuildin, 1)
plugin_def("import_net_cmd",   "sch/net import: run custom cmd",  sbuildin, 1)
plugin_def("import_netlist",   "import netlist",                  sbuildin, 1)
plugin_def("import_orcad_net", "import Orcad netlist",            sbuildin, 1)
plugin_def("import_pads_net",  "import PADS ascii netlist .asc",  sbuildin, 1)
plugin_def("import_protel_net","import Protel netlist 2.0",       sbuildin, 1)
plugin_def("import_pxm_gd",    "import pixmaps from png/gif/jpg", sbuildin, 1)
plugin_def("import_pxm_pnm",   "import pixmaps from pnm ",        sbuildin, 1)
plugin_def("import_sch2",      "import sch v2",                   sbuildin, 1)
plugin_def("import_tinycad",   "import tinycad .net",             sbuildin, 1)
plugin_def("import_ttf",       "import ttf glyphs",               sbuildin, 1)

plugin_header("\nExport plugins:\n")
plugin_def("cam",              "cam/job based export",            sbuildin, 1)
plugin_def("ddraft",           "2D drafting helper",              sbuildin, 1)
plugin_def("export_bom",       "bom pcb_exporter",                sbuildin, 1)
plugin_def("export_c_draw",    "export drawing in C code",        sdisable, 0)
plugin_def("export_debug",     "export debug draw",               sdisable, 0)
plugin_def("export_dxf",       "DXF pcb_exporter",                sbuildin, 1)
plugin_def("export_excellon",  "Excellon drill exporter",         sbuildin, 1)
plugin_def("export_fidocadj",  "FidoCadJ .fcd pcb_exporter",      sbuildin, 1)
plugin_def("export_gcode",     "gcode pcb_exporter",              sbuildin, 1)
plugin_def("export_gerber",    "Gerber pcb_exporter",             sbuildin, 1)
plugin_def("export_hpgl",      "export thin-drawing in hpgl",     sdisable, 0)
plugin_def("export_ipcd356",   "IPC-D-356 Netlist pcb_exporter",  sbuildin, 1)
plugin_def("export_lpr",       "lpr pcb_exporter (printer)",      sbuildin, 1)
plugin_def("export_oldconn",   "old connection data format",      sbuildin, 1)
plugin_def("export_openems",   "openems exporter",                sbuildin, 1)
plugin_def("export_openscad",  "openscad pcb_exporter",           sbuildin, 1)
plugin_def("export_png",       "png/gif/jpg pcb_exporter",        sbuildin, 1)
plugin_def("export_ps",        "postscript pcb_exporter",         sbuildin, 1)
plugin_def("export_stat",      "export board statistics",         sbuildin, 1)
plugin_def("export_stl",       "3d export: STL/amf/pro",          sbuildin, 1)
plugin_def("export_svg",       "SVG pcb_exporter",                sbuildin, 1)
plugin_def("export_vfs_fuse",  "FUSE VFS server",                 sdisable, 1)
plugin_def("export_vfs_mc",    "GNU mc VFS server",               sdisable, 1)
plugin_def("export_xy",        "xy (centroid) pcb_exporter",      sbuildin, 1)

plugin_header("\nIO plugins (file formats):\n")
plugin_def("io_altium",        "Altium board",                    sbuildin, 1)
plugin_def("io_autotrax",      "autotrax (freeware PCB CAD)",     sbuildin, 1)
plugin_def("io_bxl",           "BXL footprint",                   sbuildin, 1)
plugin_def("io_dsn",           "specctra .dsn",                   sbuildin, 1)
plugin_def("io_eagle",         "Eagle's xml and binary formats",  sbuildin, 1)
plugin_def("io_hyp",           "hyperlynx .hyp loader",           sbuildin, 1)
plugin_def("io_kicad_legacy",  "Kicad's legacy format",           sbuildin, 1)
plugin_def("io_kicad",         "Kicad's s-expr format",           sbuildin, 1)
plugin_def("io_lihata",        "lihata board format",             sbuildin, 1)
plugin_def("io_mentor_cell",   "Mentor Graphics cell footprints", sdisable, 1)
plugin_def("io_pads",          "PADS board",                      sbuildin, 1)
plugin_def("io_pcb",           "the original pcb format",         sbuildin, 1)
plugin_def("io_tedax",         "tEDAx (Trivial EDA eXchange)",    sbuildin, 1)


plugin_dep("ar_cpcb", "lib_compat_help")
plugin_dep("ar_cpcb", "lib_gensexpr")
plugin_dep("ar_cpcb", "lib_netmap")
plugin_dep("ar_extern", "io_tedax")
plugin_dep("asm", "query")
plugin_dep("cam", "query")
plugin_dep("dialogs", "draw_csect")
plugin_dep("dialogs", "draw_fontsel")
plugin_dep("dialogs", "lib_hid_common")
plugin_dep("dialogs", "lib_hid_pcbui")
plugin_dep("draw_fab", "report")
plugin_dep("drc_query", "query")
plugin_dep("export_fidocadj", "lib_compat_help")
plugin_dep("export_gcode", "millpath")
plugin_dep("export_gerber", "export_excellon")
plugin_dep("export_ipcd356", "lib_compat_help")
plugin_dep("export_lpr", "export_ps")
plugin_dep("export_openems", "lib_polyhelp")
plugin_dep("export_openscad", "lib_polyhelp")
plugin_dep("export_ps", "lib_compat_help")
plugin_dep("export_vfs_fuse", "lib_vfs")
plugin_dep("export_vfs_mc", "lib_vfs")
plugin_dep("export_xy", "export_bom")
plugin_dep("extedit", "io_lihata")
plugin_dep("exto_std", "lib_compat_help")
plugin_dep("fp_wget", "fp_fs")
plugin_dep("fp_wget", "lib_wget")
plugin_dep("import_accel_net", "lib_gensexpr")
plugin_dep("import_ipcd356", "lib_compat_help")
plugin_dep("import_mentor_sch", "lib_gensexpr")
plugin_dep("import_orcad_net", "lib_gensexpr")
plugin_dep("io_altium", "lib_compat_help")
plugin_dep("io_autotrax", "lib_polyhelp")
plugin_dep("io_dsn", "lib_compat_help")
plugin_dep("io_dsn", "lib_gensexpr")
plugin_dep("io_dsn", "lib_netmap")
plugin_dep("io_eagle", "lib_compat_help")
plugin_dep("io_eagle", "shape")
plugin_dep("io_hyp", "lib_netmap")
plugin_dep("io_kicad", "lib_compat_help")
plugin_dep("io_kicad", "lib_gensexpr")
plugin_dep("io_kicad", "shape")
plugin_dep("io_kicad_legacy", "io_kicad")
plugin_dep("io_lihata", "lib_compat_help")
plugin_dep("io_pads", "lib_compat_help")
plugin_dep("io_pads", "lib_netmap")
plugin_dep("io_pads", "teardrops")
plugin_dep("io_pcb", "lib_compat_help")
plugin_dep("io_tedax", "lib_compat_help")
plugin_dep("io_tedax", "lib_netmap")
plugin_dep("lib_hid_pcbui", "lib_hid_common")
plugin_dep("lib_netmap", "query")
plugin_dep("lib_vfs", "propedit")
plugin_dep("millpath", "ddraft")
plugin_dep("millpath", "lib_polyhelp")
plugin_dep("order_pcbway", "lib_wget")
plugin_dep("order_pcbway", "order")
plugin_dep("query", "lib_formula")
plugin_dep("report", "query")
plugin_dep("show_netnames", "query")
