# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

all:

install_:
	$(MKDIR) $(DATADIR)
	$(CPC) `pwd`/ListLibraryContents.sh $(DATADIR)/ListLibraryContents.sh

install:
	make install_ CPC="$(CP)"

linstall:
	make install_ CPC="$(LN)"

uninstall:
	$(RM) $(DATADIR)/ListLibraryContents.sh

clean:

include ../Makefile.conf

