### config part ###

# edit this to point to pcb-rnd svn checkout's trunk/ (or the release
# tarball's root). Assumes the pcb-rnd source is configured.
TRUNK=../../../..

# You may want to use some configuration system for these; below is
# a simple "edited by the user" concept to keep the example small and clear
CFLAGS = -Wall -g -fPIC -I$(TRUNK) -I$(TRUNK)/src_3rd
LDFLAGS =
SOFLAGS = -rdynamic -shared


### rules ###

all: ext_bar.so

include $(TRUNK)/Makefile.conf

ext_bar.so: ext_bar.o
	$(CC) -o ext_bar.so $(LDFLAGS) $(SOFLAGS) ext_bar.o

ext_bar.o: ext_bar.c

clean:
	rm -f ext_bar.o ext_bar.so
