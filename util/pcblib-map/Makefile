OUT = map.html conn.html parametric.html smd2.html smd3.html smdN.html \
      trh2.html trh3.html trhN.html cache

all: $(OUT)

map.html map.png: map.pcb
	./imgmap_page.sh map

%.html %.png: %.pcb
	./imgmap_fp.sh $*

cache:
	./cache.sh > cache

clean:
	rm $(OUT)
