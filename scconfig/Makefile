# --- configuration part --

# - generic configuration -
SRC=src
BIN=src

#  what cflags to use to compile scconfig
USER_CFLAGS = -DGENCALL -Isrc
#USER_CFLAGS = -Wall -Wextra -g -DGENCALL -Isrc

#  what ldflags to use to link scconfig
USER_LDFLAGS =

#  in case hooks.c needs to link to something local
USER_OBJS = src/util/arg_auto_set.o

#  what to build - a ./configure
all: configure cquote

# This line imports scconfig core and default tests
include src/default/Makefile.plugin

#
# - PLUGINS -
#
#  Comment this line if you are not interested in c99 features
include src/c99/Makefile.plugin

#  Comment this line if you do not need script libs to be detected
include src/scripts/Makefile.plugin

#  Comment this line if you do not need parser libs to be detected
# include src/parser/Makefile.plugin

#  Comment this line if you do not need to detect parser generators
include src/parsgen/Makefile.plugin

#  Comment this line if you do not need math related libs
include src/math/Makefile.plugin

#  Comment this line if you do not need socket/networking
# include src/socket/Makefile.plugin

#  Comment this line if you do not need user/password API detection
include src/userpass/Makefile.plugin

#  Comment this line if you do not need gui (X11, toolkits)
include src/gui/Makefile.plugin

#  Comment this line if you do not need software utility libs (glib)
include src/sul/Makefile.plugin

#  Uncomment this line if you need menus
include src/menulib/Makefile.plugin

#  Comment this line if you do not need generator (templating)
#include src/generator/Makefile.plugin

#  Comment this line if you do not need tmpasm (templating); conflicts with generator
include src/tmpasm/Makefile.plugin

# --- you shouldn't edit the lines below ---
OBJS = $(USER_OBJS) hooks.o $(DEFAULT_NOMAIN_OBJS) $(SCRIPT_OBJS) $(PARSER_OBJS) $(GENERATOR_OBJS) $(TMPASM_OBJS) $(C99_OBJS) $(PARSGEN_OBJS) $(MATH_OBJS) $(SOCKET_OBJS) $(USERPASS_OBJS) $(GUI_OBJS) $(SUL_OBJS)
CFLAGS = $(USER_CFLAGS) $(DEFAULT_CFLAGS) $(SCRIPT_CFLAGS) $(PARSER_CFLAGS) $(GENERATOR_CFLAGS) $(TMPASM_CFLAGS) $(C99_CFLAGS) $(PARSGEN_CFLAGS) $(MATH_CFLAGS) $(SOCKET_CFLAGS) $(USERPASS_CFLAGS) $(GUI_CFLAGS)  $(SUL_CFLAGS) $(MENULIB_CFLAGS) -Isrc/default
LDFLAGS = $(USER_LDFLAGS) $(DEFAULT_LDFLAGS) $(SCRIPT_LDFLAGS) $(PARSER_LDFLAGS) $(GENERATOR_LDFLAGS) $(TMPASM_LDFLAGS) $(C99_LDFLAGS) $(PARSGEN_LDFLAGS) $(MATH_LDFLAGS) $(SOCKET_LDFLAGS) $(USERPASS_LDFLAGS) $(GUI_LDFLAGS) $(SUL_LDFLAGS) $(MENULIB_LDFLAGS)

all: configure revtest

revtest: revtest.o
	$(CC) -o revtest revtest.o

revtest.o: revtest.c  Rev.h
	$(CC) -c $(CFLAGS) -o revtest.o revtest.c

configure: $(OBJS)  $(DEFAULT_MAIN_OBJS)
	$(CC) -o configure $(OBJS)  $(DEFAULT_MAIN_OBJS)

menuconfig: menucfg
	./menucfg

menucfg: $(OBJS) $(MENULIB_OBJS) src/default/main_lib.o src/default/main_custom_args.o menucfg.o src/util/arg_auto_menu.o
	$(CC) -o menucfg $(OBJS) $(MENULIB_OBJS) menucfg.o src/default/main_lib.o src/default/main_custom_args.o src/util/arg_auto_menu.o

menucfg.o: menucfg.c
	$(CC) -c $(CFLAGS) -o menucfg.o menucfg.c

hooks.o: plugin_3state.h plugins.h Rev.h

cquote: cquote.c

clean:
	rm $(OBJS) $(DEFAULT_MAIN_OBJS) configure

