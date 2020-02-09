# This Makefile is a plain old hand written one; all configuration settings
# are included from $(ROOT)/Makefile.conf which is scconfig generated
ROOT=../..

IN=pcb-rnd.1.mml pcb-prj2lht.1.mml fp2preview.1.mml fp2subc.1.mml
OUT_HTML = pcb-rnd.1.html pcb-prj2lht.1.html fp2preview.1.html fp2subc.1.html
OUT_MAN1 = pcb-rnd.1 pcb-prj2lht.1 fp2preview.1  fp2subc.1
OUT_LINT = pcb-rnd.1.lint pcb-prj2lht.1.lint fp2preview.1.lint fp2subc.1.lint

OUTPUT = $(OUT_HTML) $(OUT_MAN1) index.html
MML = /usr/bin/mml

all: $(OUTPUT)

lint: $(OUT_LINT)

.SUFFIXES: .html .mml .lint

.mml.html: .mml_linkmap
	$(MML) -i copyright.mml -f html $<  > $@

.mml:
	$(MML) -i copyright.mml -f man $<  > $@

.mml_linkmap:
	$(MML) -i copyright.mml -f linkmap $(IN) > $@

index.html: $(IN)
	@echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">' > $@
	@echo '<HTML><HEAD><meta http-equiv="Content-Type" content="text/html;charset=us-ascii"><TITLE>man page index - pcb-rnd</TITLE></HEAD><BODY><UL>' >> $@
	$(MML) -i copyright.mml -f indexhtml $(IN) >> $@
	@echo "</UL></BODY></HTML>" >> $@

clean:

distclean:

genclean:
	rm $(OUTPUT) 2>/dev/null ; true

.mml.lint:
	$(MML) -i copyright.mml -f lint $<

install_all:
	$(SCCBOX) mkdir -p "$(MAN1DIR)"
	$(SCCBOX) $(HOW) "pcb-rnd.1" "$(MAN1DIR)/pcb-rnd.1"
	$(SCCBOX) $(HOW) "pcb-prj2lht.1" "$(MAN1DIR)/pcb-prj2lht.1"
	$(SCCBOX) $(HOW) "gsch2pcb-rnd.1" "$(MAN1DIR)/gsch2pcb-rnd.1"
	$(SCCBOX) $(HOW) "fp2preview.1" "$(MAN1DIR)/fp2preview.1"
	$(SCCBOX) $(HOW) "fp2subc.1" "$(MAN1DIR)/fp2subc.1"

install:
	$(MAKE) install_all HOW="install -f"

linstall:
	$(MAKE) install_all HOW="linstall -f"

uninstall:
	$(MAKE) install_all HOW="uninstall"

include $(ROOT)/Makefile.conf

