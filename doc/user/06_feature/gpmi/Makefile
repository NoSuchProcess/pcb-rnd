DPI_BIG=-Gsize=8,8 -Gdpi=166
DPI=-Gsize=8,8 -Gdpi=100

all:  gpmi_flow.png gpmi_flow_load.png gpmi_flow_reg.png gpmi_flow_menu.png gpmi_flow_exp.png
	cd rosetta && make

gpmi_flow.png: gpmi_flow.dot Makefile
	dot $(DPI_BIG) -Tpng gpmi_flow.dot >$@

gpmi_flow_load.png: gpmi_flow_load.dot Makefile
	dot $(DPI) -Tpng gpmi_flow_load.dot >$@

gpmi_flow_reg.png: gpmi_flow_reg.dot Makefile
	dot $(DPI) -Tpng gpmi_flow_reg.dot >$@

gpmi_flow_menu.png: gpmi_flow_menu.dot Makefile
	dot $(DPI) -Tpng gpmi_flow_menu.dot >$@

gpmi_flow_exp.png: gpmi_flow_exp.dot Makefile
	dot $(DPI) -Tpng gpmi_flow_exp.dot >$@
