#CFLAGS = -Wall -Wextra -g
CFLAGS = -g

all: example_dynamic example_static

example_dynamic: example_dynamic.o qparse.o

example_static: example_static.o qparse.o

qparse.o: qparse.c qparse.h
	$(CC) $(CFLAGS) -c qparse.c -o qparse.o

clean:
	rm qparse.o example example.o ; true
