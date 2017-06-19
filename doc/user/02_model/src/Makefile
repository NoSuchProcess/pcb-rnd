PCBRND=pcb-rnd
CLEANFILES= ../*.png

all: ../objects_basic.png ../objects_complex.png ../obj_line.png \
     ../obj_arc.png ../via_therm_noconn.png ../via_therm_solid.png \
     ../via_therm_round_x90.png ../via_therm_round_x45.png \
     ../via_therm_sharp_x90.png ../via_therm_sharp_x45.png \
     via_ring_shape.png via_square_shape.png via_octagon_shape.png \
     via_asym_shape.png

../objects_basic.png: objects_basic.lht
	$(PCBRND) -x png --dpi 450 objects_basic.lht
	mv objects_basic.png ../objects_basic.png

../objects_complex.png: objects_complex.lht
	$(PCBRND) -x png --dpi 450 objects_complex.lht
	mv objects_complex.png ../objects_complex.png

../obj_line.png: obj_line.lht
	$(PCBRND) -x png --dpi 450 obj_line.lht
	mv obj_line.png ../obj_line.png

../obj_arc.png: obj_arc.lht
	$(PCBRND) -x png --dpi 450 obj_arc.lht
	mv obj_arc.png ../obj_arc.png

../via_therm_noconn.png: via_therm_noconn.lht
	$(PCBRND) -x png --dpi 450 $<
	mv via_therm_noconn.png ../

# make thermal style images
../via_therm_solid.png: via_therm_solid.lht
	$(PCBRND) -x png --dpi 450 $<
	mv via_therm_solid.png ../

../via_therm_round_x90.png: via_therm_round_x90.lht
	$(PCBRND) -x png --dpi 450 $<
	mv via_therm_round_x90.png ../

../via_therm_round_x45.png: via_therm_round_x45.lht
	$(PCBRND) -x png --dpi 450 $<
	mv via_therm_round_x45.png ../

../via_therm_sharp_x90.png: via_therm_sharp_x90.lht
	$(PCBRND) -x png --dpi 450 $<
	mv via_therm_sharp_x90.png ../

../via_therm_sharp_x45.png: via_therm_sharp_x45.lht
	$(PCBRND) -x png --dpi 450 $<
	mv via_therm_sharp_x45.png ../

#make shapes for shape table
via_ring_shape.png: via_ring_shape.lht
	$(PCBRND) -x png --dpi 450 $<
	mv $@ ../

via_square_shape.png: via_square_shape.lht
	$(PCBRND) -x png --dpi 450 $<
	mv $@ ../

via_octagon_shape.png: via_octagon_shape.lht
	$(PCBRND) -x png --dpi 450 $<
	mv $@ ../

via_asym_shape.png: via_asym_shape.lht
	$(PCBRND) -x png --dpi 450 $<
	mv $@ ../

clean: $(CLEANRULES) 
	-rm $(CLEANFILES)
