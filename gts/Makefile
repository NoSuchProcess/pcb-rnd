# plain old Makefile for now - should be scconfig generated for explicit rules

CFLAGS = -I.. -DG_LOG_DOMAIN=\"Gts\" $(shell pkg-config --cflags glib-2.0)

OBJS = \
	object.o \
	point.o \
	vertex.o \
	segment.o \
	edge.o \
	triangle.o \
	face.o \
	kdtree.o \
	bbtree.o \
	misc.o \
	predicates.o \
	heap.o \
	eheap.o \
	fifo.o \
	matrix.o \
	surface.o \
	stripe.o \
	vopt.o \
	refine.o \
	iso.o \
	isotetra.o \
	split.o \
	psurface.o \
	hsurface.o \
	cdt.o \
	boolean.o \
	named.o \
	oocs.o \
	container.o \
	graph.o \
	pgraph.o \
	partition.o \
	curvature.o \
	tribox3.o

libgts.a: $(OBJS)
	ar rvu libgts.a $(OBJS)
