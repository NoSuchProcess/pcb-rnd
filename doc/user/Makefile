ROOT=../..
BUILD=$(ROOT)/util/build_ps.sh
USERDIR=$(DOCDIR)/user

INSTD = 02_model 03_data 04_invoc 05_ui 06_feature 07_io \
        08_util 09_appendix

all: user.pdf

user.ps: ps.lst $(BUILD) default.css Makefile
	HTML2PS_OPTS="-f default.css" HTML2PS_SED="s^<[hH]1>.*</[hH]1>^^g" $(BUILD) `cat ps.lst` > user.ps

user.pdf: user.ps
	ps2pdf user.ps

ps.lst: */*.html */*.png
	echo "title.html" > ps.lst
	echo "index.html" >> ps.lst
	ls */*.html | sort >> ps.lst

include Makefile.inst

regen: instgen
	./instgen

install:
	$(MAKE) install_all HOW="install -f"

linstall:
	$(MAKE) install_all HOW="install -f -l --absolute "

uninstall:
	$(MAKE) install_all HOW="install -u"

include $(ROOT)/Makefile.conf
