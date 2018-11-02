### config part ###

# edit this to point to pcb-rnd svn checkout's trunk/ (or the release
# tarball's root). Assumes the pcb-rnd source is configured.
TRUNK=../../../..

# You may want to use some configuration system for these; below is
# a simple "edited by the user" concept to keep the example small and clear
CFLAGS = -Wall -g -fPIC -I$(TRUNK) -I$(TRUNK)/src_3rd
LDFLAGS =
SOFLAGS = -rdynamic -shared
PLUGDIR = $(LIBDIR)/plugins


### rules ###

all: ext_foo.so

include $(TRUNK)/Makefile.conf

ext_foo.so: ext_foo.o
	$(CC) -o ext_foo.so $(LDFLAGS) $(SOFLAGS) ext_foo.o

ext_foo.o: ext_foo.c

clean:
	rm -f ext_foo.o ext_foo.so

install:
	mkdir -p $(PLUGDIR)
	cp ext_foo.so $(PLUGDIR)/ext_foo.so
	cp ext_foo.pup $(PLUGDIR)/ext_foo.pup

uninstall:
	rm -f $(PLUGDIR)/ext_foo.so $(PLUGDIR)/ext_foo.pup

