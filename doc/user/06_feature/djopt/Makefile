PCB=../../src/pcb-rnd
HTML=index.html
CASES = \
 debumpify.$(TARGET) \
 miter.$(TARGET) \
 orthopull.$(TARGET) \
 unjaggy.$(TARGET) \
 vianudge.$(TARGET) \
 viatrim.$(TARGET)

all:
	make conv TARGET=out.png
	make conv TARGET=png
	make index

index:
	make index.html TARGET=txt

index.html: Pre.html Post.html $(CASES) Makefile
	cat Pre.html > $(HTML)
	make conv TARGET=tbl
	cat Post.html >>$(HTML)

conv: $(CASES)

.SUFFIXES: .pcb .out.pcb .png .out.png .tbl
.PRECIOUS: %.pcb %.out.pcb %.png %.out.png

.pcb.out.pcb:
	echo "OptAutoOnly(); djopt($*); SaveTo(LayoutAs, $*.out.pcb.tmp);"| $(PCB) --gui batch $*.pcb
	awk '/Symbol.. . 1800./,/^Attribute/ { next } { print $$0 }' < $*.out.pcb.tmp > $*.out.pcb
	rm $*.out.pcb.tmp

.pcb.tbl:
	echo '<tr><th>$*<td>'>>$(HTML)
	cat $*.txt >>$(HTML)
	echo '  <td><a href="$*.pcb"><img src="$*.png"></a>  <td><a href="$*.out.pcb"><img src="$*.out.png"></a>'>>$(HTML)

.pcb.png:
	$(PCB) -x png --dpi 300  $*.pcb

out.pcb.out.png:
	$(PCB) -x png --dpi 300  $*.out.pcb

clean:
	rm *.out.pcb *.png index.html 2>/dev/null; true




