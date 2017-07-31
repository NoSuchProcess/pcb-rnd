E = ./Export.sh -t -a

all:

ROOT=../..
include $(ROOT)/Makefile.conf

test:
	@make test_export && echo "*** export: QC PASS ***"

test_export:
	@$E -f bboard ; \
	$E -f nelma ; \
	$E -f bom ; \
	$E -f dsn ;\
	$E -f IPC-D-356 ; \
	$E -f eps ; \
	$E -f ps ; \
	$E -f XY  2>/dev/null; \
	$E -f openscad ; \
	$E -f png ; \
	$E -f gerber ; \
	$E -f svg

clean:
	$(SCCBOX) rm -f out/* diff/*
