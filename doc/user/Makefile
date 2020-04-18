ROOT=../..
BUILD=$(ROOT)/util/build_ps.sh
USERDIR=$(DOCDIR)/user

INSTD = 02_model 03_data 04_invoc 05_ui 06_feature 07_io \
        08_util 09_appendix

all: pdf

include $(ROOT)/Makefile.conf

pdf: user1.pdf user2.pdf user3.pdf

ps_logo.png: ../resources/logo32.png
	cp ../resources/logo32.png ps_logo.png

user1.ps: ps1.lst $(BUILD) default.css Makefile ps_logo.png
	HTML2PS_OPTS="-f default.css --toc h" HTML2PS_SED="s^%%%volume%%%^Volume 1: Core functionality^;s^%%%ver%%%^Version $(PCB_RND_VER)^g" $(BUILD) `cat ps1.lst` > user1.ps

user2.ps: ps2.lst $(BUILD) default.css Makefile
	HTML2PS_OPTS="-f default.css --toc h" HTML2PS_SED="s^%%%volume%%%^Volume 2: Plugins and utilities^;s^%%%ver%%%^Version $(PCB_RND_VER)^g" $(BUILD) `cat ps2.lst` > user2.ps

user3.ps: ps3.lst $(BUILD) default.css Makefile
	HTML2PS_OPTS="-f default.css --toc h" HTML2PS_SED="s^%%%volume%%%^Volume 3: Appendix and misc^;s^%%%ver%%%^Version $(PCB_RND_VER)^g" $(BUILD) `cat ps3.lst` > user3.ps

user1.pdf: user1.ps
	ps2pdf user1.ps

user2.pdf: user2.ps
	ps2pdf user2.ps

user3.pdf: user3.ps
	ps2pdf user3.ps

ps.lst.vols: */*.html */*.png ./listlnk Makefile
	./listlnk list -v | grep -v "^index.html	\|[.]png	\|[.]svg	\|[.]pcb\|	[.]lht" > ps.lst.vols

ps1.lst: */*.html */*.png ps.lst.vols
	echo "ps_title.html" > ps1.lst
	awk -F "	" '($$2 == 1) {print $$1}' < ps.lst.vols >> ps1.lst

ps2.lst: */*.html */*.png ps.lst.vols
	echo "ps_title.html" > ps2.lst
	awk -F "	" '($$2 == 2) {print $$1}' < ps.lst.vols >> ps2.lst

ps3.lst: */*.html */*.png ps.lst.vols
	echo "ps_title.html" > ps3.lst
	awk -F "	" '($$2 == 3) {print $$1}' < ps.lst.vols >> ps3.lst

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
