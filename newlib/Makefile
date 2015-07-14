# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

all:

install_:
	$(MKDIR) $(DATADIR)/newlib
	$(CPC) `pwd`/2_pin_thru-hole_packages $(DATADIR)/newlib/2_pin_thru-hole_packages
	$(CPC) `pwd`/connectors $(DATADIR)/newlib/connectors
	$(CPC) `pwd`/crystal $(DATADIR)/newlib/crystal
	$(CPC) `pwd`/electro-optics $(DATADIR)/newlib/electro-optics
	$(CPC) `pwd`/headers $(DATADIR)/newlib/headers
	$(CPC) `pwd`/keystone $(DATADIR)/newlib/keystone
	$(CPC) `pwd`/msp430 $(DATADIR)/newlib/msp430
	$(CPC) `pwd`/not_vetted_ingo $(DATADIR)/newlib/not_vetted_ingo
	$(CPC) `pwd`/tests $(DATADIR)/newlib/tests

install:
	make install_ CPC="$(CP)"

linstall:
	make install_ CPC="$(LN)"

uninstall:
	$(RM) $(DATADIR)/newlib

clean:

include ../Makefile.conf

