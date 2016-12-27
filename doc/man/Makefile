# This Makefile is a plain old hand written one; all configuration settings
# are included from ../../Makefile.conf which is scconfig generated

IN=pcb-rnd.1.mml pcb-strip.1.mml pcb-prj2lht.1.mml fp2anim.1.mml
OUT_HTML = pcb-rnd.1.html pcb-strip.1.html pcb-prj2lht.1.html fp2anim.1.html
OUT_MAN1 = pcb-rnd.1 pcb-strip.1 pcb-prj2lht.1 fp2anim.1
OUT_LINT = pcb-rnd.1.lint pcb-strip.1.lint pcb-prj2lht.1.lint fp2anim.1.lint

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
	@echo "<HTML><BODY>" > $@
	$(MML) -i copyright.mml -f indexhtml $(IN) >> $@
	@echo "</BODY></HTML>" >> $@

clean:

distclean:

genclean:
	rm $(OUTPUT) 2>/dev/null ; true

.mml.lint:
	$(MML) -i copyright.mml -f lint $<

install_:
	$(MKDIR) "$(MAN1DIR)"
	$(CPC) "`pwd`/pcb-rnd.1" "$(MAN1DIR)/pcb-rnd.1"
	$(CPC) "`pwd`/pcb-strip.1" "$(MAN1DIR)/pcb-strip.1"
	$(CPC) "`pwd`/pcb-prj2lht.1" "$(MAN1DIR)/pcb-prj2lht.1"
	$(CPC) "`pwd`/gsch2pcb-rnd.1" "$(MAN1DIR)/gsch2pcb-rnd.1"
	$(CPC) "`pwd`/fp2anim.1" "$(MAN1DIR)/fp2anim.1"

uninstall:
	$(RM) "$(MAN1DIR)/pcb-rnd.1"
	$(RM) "$(MAN1DIR)/pcb-strip.1"
	$(RM) "$(MAN1DIR)/pcb-prj2lht.1"
	$(RM) "$(MAN1DIR)/gsch2pcb-rnd.1"
	$(RM) "$(MAN1DIR)/fp2anim.1"

install:
	$(MAKE) install_ CPC="$(CP)"

linstall:
	$(MAKE) install_ CPC="$(LN)"

include ../../Makefile.conf

