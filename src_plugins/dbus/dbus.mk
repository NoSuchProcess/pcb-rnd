append /local/pcb/OBJS {dbus-pcbmain.o dbus.o}

append /local/pcb/RULES  [@
# dbus
dbus-introspect.h : dbus.xml Makefile
	echo '/* AUTOMATICALLY GENERATED FROM dbus.xml DO NOT EDIT */' > $@.tmp
	echo "static const char *pcb_dbus_introspect_xml ="  > $@.tmp
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/"/' < dbus.xml >> $@.tmp
	echo ";" >> $@.tmp
	mv $@.tmp $@
@]
