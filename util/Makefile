# plain old hand crafted Makefile

all:
	cd gsch2pcb-rnd && make all

clean:
	cd gsch2pcb-rnd && make clean

install_:
	$(MKDIR) "$(BINDIR)" "$(LIBDIR)"
	$(CPC) "`pwd`/fp2anim" "$(BINDIR)/fp2anim"
	cd gsch2pcb-rnd && make install_ CPC="$(CP)"

install:
	make install_ CPC="$(CP)"

linstall:
	make install_ CPC="$(LN)"

uninstall:
	$(RM) "$(BINDIR)/fp2anim"
	cd gsch2pcb-rnd && make uninstall
