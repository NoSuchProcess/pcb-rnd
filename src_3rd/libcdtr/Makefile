#CFLAGS_DBG=-Wall -O0 -g3 
CFLAGS=$(CFLAGS_DBG) $(CFLAGS_LIBCDTR) -I. -I.. -I../..
LDFLAGS=-lm $(LDFLAGS_LIBCDTR)

OBJS=cdt_test.o cdt.o point.o edge.o triangle.o
LIBA=libcdtr.a

cdt_test: $(OBJS)

include Makefile.dep

$(LIBA): $(OBJS)
	-rm $(LIBA)
	ar ru $(LIBA) $(OBJS)

cdt_test.o: cdt_test.c
	$(CC) $(CFLAGS) -c cdt_test.c -o $@

cdt.o: cdt.c
	$(CC) $(CFLAGS) -c cdt.c -o $@

point.o: point.c
	$(CC) $(CFLAGS) -c point.c -o $@

edge.o: edge.c
	$(CC) $(CFLAGS) -c edge.c -o $@

triangle.o: triangle.c
	$(CC) $(CFLAGS) -c triangle.c -o $@

dep:
	echo $(OBJS) | ./dep.sh $(CFLAGS) > Makefile.dep

clean:
	-rm $(OBJS) cdt_test
