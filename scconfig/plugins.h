plugin_header("\nFeature plugins:\n")
plugin_def("gpmi",            "GPMI scripting",            sbuildin, 1)
plugin_def("diag",            "diagnostic acts. for devs", sdisable, 1)
plugin_def("autocrop",        "crop board to fit objects", sbuildin, 1)
plugin_def("autoroute",       "the original autorouter",   sbuildin, 1)
plugin_def("boardflip",       "flip board objects",        sdisable, 0)
plugin_def("distalign",       "distribute and align objs", sbuildin, 1)
plugin_def("distaligntext",   "distribute and align text", sbuildin, 1)
plugin_def("jostle",          "push lines out of the way", sbuildin, 1)
plugin_def("polycombine",     "combine selected polygons", sbuildin, 1)
plugin_def("polystitch",      "stitch polygon at cursor",  sdisable, 0)
plugin_def("teardrops",       "draw teardrops on pins",    sbuildin, 1)
plugin_def("smartdisperse",   "netlist based dispenser",   sbuildin, 1)
plugin_def("toporouter",      "topological autorouter",    sdisable, 0)
plugin_def("autoplace",       "auto place components",     sbuildin, 1)
plugin_def("vendordrill",     "vendor drill mapping",      sbuildin, 1)
plugin_def("puller",          "puller",                    sbuildin, 1)
plugin_def("djopt",           "djopt",                     sbuildin, 1)
plugin_def("mincut",          "minimal cut shorts",        sbuildin, 1)
plugin_def("renumber",        "renumber action",           sbuildin, 1)
plugin_def("oldactions",      "old/obsolete actions",      sdisable, 1)
plugin_def("fontmode",        "font editor",               sbuildin, 1)
plugin_def("legacy_func",     "legacy functions",          sbuildin, 1)
plugin_def("stroke",          "libstroke gestures",        sdisable, 1)
plugin_def("report",          "report actions",            sbuildin, 1)
plugin_def("dbus",            "DBUS interface",            sdisable, 1)
plugin_def("shand_cmd",       "command shorthands",        sbuildin, 1)
plugin_def("propedit",        "object property editor",    sbuildin, 1)
plugin_def("loghid",          "diagnostics: log HID calls",sdisable, 1)
plugin_def("query",           "query language",            sbuildin, 1)

plugin_header("\nFootprint backends:\n")
plugin_def("fp_fs",           "filesystem footprints",     sbuildin, 1)
plugin_def("fp_wget",         "web footprints",            sbuildin, 1)

plugin_header("\nImport plugins:\n")
plugin_def("import_sch",      "import sch",                sbuildin, 1)
plugin_def("import_edif",     "import edif",               sbuildin, 1)
plugin_def("import_netlist",  "import netlist",            sbuildin, 1)

plugin_header("\nExport plugins:\n")
plugin_def("export_gcode",    "gcode exporter",            sbuildin, 1)
plugin_def("export_nelma",    "nelma exporter",            sbuildin, 1)
plugin_def("export_png",      "png/gif/jpg exporter",      sbuildin, 1)
plugin_def("export_bom",      "bom exporter",              sbuildin, 1)
plugin_def("export_xy",       "xy (centroid) exporter",    sbuildin, 1)
plugin_def("export_gerber",   "gerber exporter",           sbuildin, 1)
plugin_def("export_lpr",      "lpr exporter (printer)",    sbuildin, 1)
plugin_def("export_ps",       "postscript exporter",       sbuildin, 1)
plugin_def("export_dxf",      "DXF exporter",              sdisable, 0)
plugin_def("export_test",     "dummy test exporter",       sdisable, 1)
plugin_def("export_bboard",   "breadboard exporter",       sdisable, 0)
plugin_def("export_openscad", "openscad exporter",         sdisable, 0)

plugin_header("\nIO plugins (file formats):\n")
plugin_def("io_lihata",       "lihata board format",       sbuildin, 1)
plugin_def("io_pcb",          "the original pcb format",   sbuildin, 1)
plugin_def("io_kicad_legacy", "Kicad's legacy format ",    sdisable, 1)

plugin_header("\nHID plugins:\n")
plugin_def("hid_batch",       "batch process (no-gui HID)",sbuildin, 1)
plugin_def("hid_gtk",         "the GTK gui",               sbuildin, 1)
plugin_def("hid_lesstif",     "the lesstif gui",           sbuildin, 1)


plugin_dep("export_lpr", "export_ps")
plugin_dep("export_xy", "export_bom")
