PCBRND=pcb-rnd
CLEANFILES= ../*.png

all: ../coord_sys.png ../coord_seq.png ../trim.png

../coord_sys.png: coord_sys.lht
	$(PCBRND) -x png --dpi 450 coord_sys.lht
	mv coord_sys.png ../coord_sys.png

../coord_seq.png: coord_seq.lht
	$(PCBRND) -x png --dpi 450 coord_seq.lht
	mv coord_seq.png ../coord_seq.png

../trim.png: trim.lht
	$(PCBRND) -x png --dpi 450 trim.lht
	mv trim.png ../trim.png

clean: $(CLEANRULES) 
	-rm $(CLEANFILES)
