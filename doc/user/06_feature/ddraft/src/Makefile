PCBRND=pcb-rnd
CLEANFILES= ../*.png

all: ../coord_sys.png

../coord_sys.png: coord_sys.lht
	$(PCBRND) -x png --dpi 450 coord_sys.lht
	mv coord_sys.png ../coord_sys.png

clean: $(CLEANRULES) 
	-rm $(CLEANFILES)
