CFLAGS = -Wall -g
LDLIBS = -lm

all: tester

tester: tester.c ../svgpath.c ../svgpath.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o tester tester.c $(LDLIBS)

test:
	@./tests.sh
