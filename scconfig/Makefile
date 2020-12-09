# --- configuration part --

# - generic configuration -
SRC=../src_3rd/librnd-local/scconfig/src
BIN=$(SRC)

#  what cflags to use to compile scconfig
USER_CFLAGS = -g -DGENCALL -I$(SRC) -I../src_3rd
#USER_CFLAGS = -Wall -Wextra -g -DGENCALL -Isrc

#  what ldflags to use to link scconfig
USER_LDFLAGS =

#  in case hooks.c needs to link to something local
USER_OBJS = $(BIN)/util/arg_auto_set.o

#  what to build - a ./configure
all: configure cquote

# This line imports scconfig core and default tests
include $(SRC)/default/Makefile.plugin

#
# - PLUGINS -
#
#  Comment this line if you are not interested in c99 features
include $(SRC)/c99/Makefile.plugin

#  Comment this line if you do not need script libs to be detected
include $(SRC)/scripts/Makefile.plugin

#  Comment this line if you do not need parser libs to be detected
# include $(SRC)/parser/Makefile.plugin

#  Comment this line if you do not need to detect parser generators
include $(SRC)/parsgen/Makefile.plugin

#  Comment this line if you do not need math related libs
include $(SRC)/math/Makefile.plugin

#  Comment this line if you do not need socket/networking
include $(SRC)/socket/Makefile.plugin

#  Comment this line if you do not need user/password API detection
include $(SRC)/userpass/Makefile.plugin

#  Comment this line if you do not need gui (X11, toolkits)
include $(SRC)/gui/Makefile.plugin

#  Comment this line if you do not need software utility libs (glib)
include $(SRC)/sul/Makefile.plugin

#  Uncomment this line if you need menus
include $(SRC)/menulib/Makefile.plugin

#  Comment this line if you do not need generator (templating)
include $(SRC)/generator/Makefile.plugin.legacy

#  Comment this line if you do not need tmpasm (templating)
include $(SRC)/tmpasm/Makefile.plugin

# --- you shouldn't edit the lines below ---
OBJS = $(USER_OBJS) hooks.o $(DEFAULT_NOMAIN_OBJS) $(SCRIPT_OBJS) $(PARSER_OBJS) $(GENERATOR_OBJS) $(TMPASM_OBJS) $(C99_OBJS) $(PARSGEN_OBJS) $(MATH_OBJS) $(SOCKET_OBJS) $(USERPASS_OBJS) $(GUI_OBJS) $(SUL_OBJS)
CFLAGS = $(USER_CFLAGS) $(DEFAULT_CFLAGS) $(SCRIPT_CFLAGS) $(PARSER_CFLAGS) $(GENERATOR_CFLAGS) $(TMPASM_CFLAGS) $(C99_CFLAGS) $(PARSGEN_CFLAGS) $(MATH_CFLAGS) $(SOCKET_CFLAGS) $(USERPASS_CFLAGS) $(GUI_CFLAGS)  $(SUL_CFLAGS) $(MENULIB_CFLAGS) -I$(SRC)/default -I.
LDFLAGS = $(USER_LDFLAGS) $(DEFAULT_LDFLAGS) $(SCRIPT_LDFLAGS) $(PARSER_LDFLAGS) $(GENERATOR_LDFLAGS) $(TMPASM_LDFLAGS) $(C99_LDFLAGS) $(PARSGEN_LDFLAGS) $(MATH_LDFLAGS) $(SOCKET_LDFLAGS) $(USERPASS_LDFLAGS) $(GUI_LDFLAGS) $(SUL_LDFLAGS) $(MENULIB_LDFLAGS)

all: configure revtest sccbox

revtest: revtest.o
	$(CC) -o revtest revtest.o

revtest.o: revtest.c  Rev.h
	$(CC) -c $(CFLAGS) -o revtest.o revtest.c

configure: $(OBJS)  $(DEFAULT_MAIN_OBJS)
	$(CC) -o configure $(OBJS)  $(DEFAULT_MAIN_OBJS)

menuconfig: menucfg
	./menucfg

menucfg: $(OBJS) $(MENULIB_OBJS) $(BIN)/default/main_lib.o $(BIN)/default/main_custom_args.o menucfg.o $(BIN)/util/arg_auto_menu.o $(BIN)/util/arg_auto_set.o
	$(CC) -o menucfg $(OBJS) $(MENULIB_OBJS) menucfg.o $(BIN)/default/main_lib.o $(BIN)/default/main_custom_args.o $(BIN)/util/arg_auto_menu.o

menucfg.o: menucfg.c
	$(CC) -c $(CFLAGS) -o menucfg.o menucfg.c

LIBRNDSCC=../src_3rd/librnd-local/src/librnd/scconfig
RNDHOOKS=$(LIBRNDSCC)/plugin_3state.h $(LIBRNDSCC)/hooks_common.h $(LIBRNDSCC)/hooks_gui.c $(LIBRNDSCC)/rnd_hook_detect.h
hooks.o: hooks.c $(RNDHOOKS)  plugins.h Rev.h ../src_3rd/librnd-local/src_3rd/puplug/scconfig_hooks.h ../src_3rd/librnd-local/src_3rd/libfungw/scconfig_hooks.h ../src_3rd/librnd-local/src_3rd/libporty_net/hooks_net.c

$(BIN)/util/arg_auto_set.o: $(SRC)/util/arg_auto_set.c $(SRC)/util/arg_auto_set.h
	$(CC) -c $(CFLAGS) -o $(BIN)/util/arg_auto_set.o $(SRC)/util/arg_auto_set.c

$(BIN)/util/sccbox.o: $(SRC)/util/sccbox.c
	$(CC) -c $(CFLAGS) -o $(BIN)/util/sccbox.o $(SRC)/util/sccbox.c

sccbox: $(BIN)/util/sccbox.o
	$(CC) $(LDFLAGS) -o sccbox $(BIN)/util/sccbox.o

cquote: $(SRC)/util/cquote.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o cquote $(SRC)/util/cquote.c

clean:
	-rm $(OBJS) $(DEFAULT_MAIN_OBJS) configure revtest revtest.o cquote core $(BIN)/util/sccbox.o

distclean:
	-rm ../config.h ../Makefile.conf ../src/Makefile ../util/gsch2pcb-rnd/Makefile config.cache config.log Rev.stamp sccbox gmon.out ../src_3rd/puplug/Makefile ../src_3rd/puplug/config.h ../src_plugins/lib_hid_gl/opengl.h
	-$(MAKE) clean
