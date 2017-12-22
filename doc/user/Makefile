TRUNK=../..
BUILD=$(TRUNK)/util/build_ps.sh

export HTML2PS_OPTS=-f default.css
export HTML2PS_SED="s^<[hH]1>.*</[hH]1>^^g"

all: user.pdf

user.ps: ps.lst $(BUILD) default.css Makefile
	$(BUILD) `cat ps.lst` > user.ps

user.pdf: user.ps
	ps2pdf user.ps

ps.lst: */*.html */*.png
	echo "title.html" > ps.lst
	echo "index.html" >> ps.lst
	ls */*.html | sort >> ps.lst
