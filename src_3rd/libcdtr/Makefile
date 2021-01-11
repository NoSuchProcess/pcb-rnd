#CFLAGS_DBG=-Wall -O0 -g3 
#CFLAGS_COORDT=-DCDT_COORD_T=double
CFLAGS=$(CFLAGS_DBG) $(CFLAGS_COORDT) $(CFLAGS_LIBCDTR) -I. -I.. -I../..
LDFLAGS=-lm $(LDFLAGS_LIBCDTR)

OBJS= cdt.o point.o edge.o triangle.o
LIBA=libcdtr.a

cdt_test: cdt_test.o $(OBJS)

include Makefile.dep

$(LIBA): $(OBJS)
	@touch $(LIBA)
	@rm $(LIBA)
	@ar ru $(LIBA) $(OBJS)
	-ar s $(LIBA)
	-ranlib $(LIBA)

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

test:
	cd regression && ./Test.sh

dep:
	echo $(OBJS) | ./dep.sh $(CFLAGS) > Makefile.dep

clean:
	-touch $(LIBA) $(OBJS) cdt_test.o cdt_test
	-rm $(LIBA) $(OBJS) cdt_test.o cdt_test
