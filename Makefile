all: mancala.c
	clang -o mancala mancala.c

clean:
	$(RM) mancala
