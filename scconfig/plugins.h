plugin_header("\nLibrary plugins:\n")
plugin_def("lib_gensexpr",    "#s-expression library",     sdisable, 0)
plugin_def("lib_legacy_func", "legacy functions",          sbuildin, 1)
plugin_def("lib_gtk_common",  "all-hid_gtk common code",   sdisable, 0)

plugin_header("\nFeature plugins:\n")
plugin_def("gpmi",            "GPMI scripting",            sbuildin, 1)
plugin_def("diag",            "diagnostic acts. for devs", sdisable, 1)
plugin_def("autocrop",        "crop board to fit objects", sbuildin, 1)
plugin_def("autoroute",       "the original autorouter",   sbuildin, 1)
plugin_def("boardflip",       "flip board objects",        sdisable, 0)
plugin_def("distalign",       "distribute and align objs", sbuildin, 1)
plugin_def("distaligntext",   "distribute and align text", sbuildin, 1)
plugin_def("draw_fab",        "fab layer in some exports", sbuildin, 1)
plugin_def("draw_csect",      "draw cross-section (layers)",sdisable, 1)
plugin_def("jostle",          "push lines out of the way", sbuildin, 1)
plugin_def("polycombine",     "combine selected polygons", sbuildin, 1)
plugin_def("polystitch",      "stitch polygon at cursor",  sdisable, 0)
plugin_def("teardrops",       "draw teardrops on pins",    sbuildin, 1)
plugin_def("smartdisperse",   "netlist based dispenser",   sbuildin, 1)
plugin_def("autoplace",       "auto place components",     sbuildin, 1)
plugin_def("vendordrill",     "vendor drill mapping",      sbuildin, 1)
plugin_def("puller",          "puller",                    sbuildin, 1)
plugin_def("djopt",           "djopt",                     sbuildin, 1)
plugin_def("mincut",          "minimal cut shorts",        sbuildin, 1)
plugin_def("renumber",        "renumber action",           sbuildin, 1)
plugin_def("oldactions",      "old/obsolete actions",      sdisable, 1)
plugin_def("fontmode",        "font editor",               sbuildin, 1)
plugin_def("stroke",          "libstroke gestures",        sdisable, 1)
plugin_def("report",          "report actions",            sbuildin, 1)
plugin_def("dbus",            "DBUS interface",            sdisable, 1)
plugin_def("shand_cmd",       "command shorthands",        sbuildin, 1)
plugin_def("propedit",        "object property editor",    sbuildin, 1)
plugin_def("loghid",          "diagnostics: log HID calls",sdisable, 1)
plugin_def("query",           "query language",            sbuildin, 1)
plugin_def("acompnet",        "net auto-completion",       sdisable, 1)
plugin_def("rubberband_orig", "the original rubberband",   sbuildin, 1)

plugin_header("\nFootprint backends:\n")
plugin_def("fp_fs",           "filesystem footprints",     sbuildin, 1)
plugin_def("fp_wget",         "web footprints",            sbuildin, 1)

plugin_header("\nImport plugins:\n")
plugin_def("import_sch",      "import sch",                sbuildin, 1)
plugin_def("import_edif",     "import edif",               sbuildin, 1)
plugin_def("import_netlist",  "import netlist",            sbuildin, 1)
plugin_def("import_dsn",      "specctra .dsn importer",    sbuildin, 1)
plugin_def("import_hyp",      "hyperlynx .hyp importer",   sdisable, 0)
plugin_def("import_mucs",     "import mucs routing",       sbuildin, 1)
plugin_def("import_ltspice",  "import ltspice .net+.asc",  sbuildin, 1)
plugin_def("import_tinycad",  "import tinycad .net",       sbuildin, 1)

plugin_header("\nExport plugins:\n")
plugin_def("export_gcode",    "gcode pcb_exporter",            sbuildin, 1)
plugin_def("export_nelma",    "nelma pcb_exporter",            sbuildin, 1)
plugin_def("export_png",      "png/gif/jpg pcb_exporter",      sbuildin, 1)
plugin_def("export_bom",      "bom pcb_exporter",              sbuildin, 1)
plugin_def("export_xy",       "xy (centroid) pcb_exporter",    sbuildin, 1)
plugin_def("export_gerber",   "gerber pcb_exporter",           sbuildin, 1)
plugin_def("export_lpr",      "lpr pcb_exporter (printer)",    sbuildin, 1)
plugin_def("export_ps",       "postscript pcb_exporter",       sbuildin, 1)
plugin_def("export_dxf",      "DXF pcb_exporter",              sdisable, 0)
plugin_def("export_test",     "dummy test pcb_exporter",       sdisable, 1)
plugin_def("export_bboard",   "breadboard pcb_exporter",       sdisable, 0)
plugin_def("export_openscad", "openscad pcb_exporter",         sdisable, 0)
plugin_def("export_dsn",      "specctra .dsn pcb_exporter",    sbuildin, 1)
plugin_def("export_ipcd356",  "IPC-D-356 Netlist pcb_exporter",sdisable, 0)
plugin_def("export_svg",      "SVG pcb_exporter",              sbuildin, 1)
plugin_def("export_stat",     "export board statistics",       sbuildin, 1)
plugin_def("export_fidocadj", "FidoCadJ .fcd pcb_exporter ",   sdisable, 0)

plugin_header("\nIO plugins (file formats):\n")
plugin_def("io_lihata",       "lihata board format",       sbuildin, 1)
plugin_def("io_pcb",          "the original pcb format",   sbuildin, 1)
plugin_def("io_kicad_legacy", "Kicad's legacy format",     sbuildin, 1)
plugin_def("io_kicad",        "Kicad's s-expr format",     sbuildin, 1)

plugin_header("\nHID plugins:\n")
plugin_def("hid_batch",       "batch process (no-gui HID)",sbuildin, 1)
plugin_def("hid_gtk",         "the GTK gui",               sbuildin, 1)
plugin_def("hid_lesstif",     "the lesstif gui",           sbuildin, 1)
plugin_def("hid_remote",      "remote HID server",         sdisable, 1)

plugin_dep("export_lpr", "export_ps")
plugin_dep("export_xy", "export_bom")
plugin_dep("io_kicad", "lib_gensexpr")
plugin_dep("hid_gtk", "lib_gtk_common")
plugin_dep("hid_gtk", "draw_csect")
plugin_dep("hid_lesstif", "draw_csect")

/* for the uniq name lib: */
plugin_dep("io_kicad_legacy", "io_kicad")

/* for drill.[ch] */
plugin_dep("draw_fab", "report")
