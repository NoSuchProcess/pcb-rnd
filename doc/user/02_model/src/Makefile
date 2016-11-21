PCBRND=pcb-rnd

all: ../objects_basic.png ../objects_complex.png ../obj_line.png

../objects_basic.png: objects_basic.lht
	$(PCBRND) -x png --dpi 450 objects_basic.lht
	mv objects_basic.lht.png ../objects_basic.png

../objects_complex.png: objects_complex.lht
	$(PCBRND) -x png --dpi 450 objects_complex.lht
	mv objects_complex.lht.png ../objects_complex.png

../obj_line.png: obj_line.lht
	$(PCBRND) -x png --dpi 450 obj_line.lht
	mv obj_line.lht.png ../obj_line.png


