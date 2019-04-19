# extra information about dialog boxes; anything that can not be extracte
# automatically from the code. Arrays indexed by id or source.
BEGIN {
	ACTION["brave"] = "brave()"
	ACTION["progress"] = "n/a"; COMMENT["progress"] = "no action API yet (but it is planned)"
	ACTION["asm"] = "asm()"
	ACTION["cam"] = "cam()"; COMMENT["cam"] = "does not yet work in lesstif"
	ACTION["constraint"] = "constraint()"
	ACTION["about"] = "about()"; COMMENT["about"] = "lesstif: needs to be resized to at least 600 pixels high"
	ACTION["prompt_for"] = "n/a"; COMMENT["prompt_for"] = "not for use from actions (for internal HID use)"
	ACTION["fallback_color_pick"] = "n/a"; COMMENT["fallback_color_pick"] = "not for use from actions (for internal HID use)"
	ACTION["flags"] = "FlagEdit()"
	ACTION["fontsel"] = "FontSel()"
	ACTION["layer_binding"] = "LayerBinding()"
	ACTION["pstk_lib"] = "PstkLib()"; COMMENT["pstk_lib"] = "does not yet work in lesstif"
	ACTION["log"] = "log()"
	ACTION["netlist"] = "NetlistDialog()"; COMMENT["NetlistDialog"] = "does not yet work in lesstif"
	ACTION["padstack_shape"] = "n/a"; COMMENT["padstack_shape"] = "open from within the padstack dialog"
	ACTION["padstack"] = "PadstackEdit()"
	ACTION["pinout"] = "pinout()"
	ACTION["plugins"] = "ManagePlugins()"; COMMENT["plugins"] = "read-only list at the moment"

}
