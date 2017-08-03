all:

ROOT=../..
include $(ROOT)/Makefile.conf

test:
	@./Test_export.sh && echo "*** export: QC PASS ***"

clean:
	$(SCCBOX) rm -f out/* diff/*
