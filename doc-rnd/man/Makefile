IN=pcb-rnd.1.mml
OUT_HTML = pcb-rnd.1.html
OUT_MAN1 = pcb-rnd.1
OUT_LINT = pcb-rnd.1.lint

OUTPUT = $(OUT_HTML) $(OUT_MAN1) index.html
MML = /usr/bin/mml

all: $(OUTPUT)

lint: pcb-rnd.1.lint

.SUFFIXES: .html .mml .lint

.mml.html: .mml_linkmap
	$(MML) -i copyright.mml -f html $<  > $@

.mml:
	$(MML) -i copyright.mml -f man $<  > $@

.mml_linkmap:
	$(MML) -i copyright.mml -f linkmap $(IN) > $@

index.html:
	@echo "<HTML><BODY>" > $@
	$(MML) -i copyright.mml -f indexhtml $(IN) >> $@
	@echo "</BODY></HTML>" >> $@

clean:
	rm $(OUTPUT) 2>/dev/null ; true

.mml.lint:
	$(MML) -i copyright.mml -f lint $<
