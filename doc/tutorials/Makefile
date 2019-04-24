all:
	$(MAKE) all_ CMD=all

all_:
	cd 15_no_sch && $(MAKE) -f Makefile.inst $(CMD)
	cd 16_tdx_netlist && $(MAKE) -f Makefile.inst $(CMD)
	cd 17_sch && $(MAKE) -f Makefile.inst $(CMD)

install:
	$(MAKE) all_ CMD=install

linstall:
	$(MAKE) all_ CMD=linstall

uninstall:
	$(MAKE) all_ CMD=uninstall

clean:

distclean:
