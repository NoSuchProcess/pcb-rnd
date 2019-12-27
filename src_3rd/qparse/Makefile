#CFLAGS = -Wall -Wextra -g
CFLAGS = -g

example: example.o qparse.o

qparse.o: qparse.c qparse.h
	$(CC) $(CFLAGS) -c qparse.c -o qparse.o

clean:
	rm qparse.o example example.o ; true
