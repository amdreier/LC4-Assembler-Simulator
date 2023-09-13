all: trace

trace: LC4.o loader.o trace.c
	clang -g LC4.o loader.o trace.c -o trace

LC4.o: LC4.c LC4.h
	clang -c LC4.c

loader.o: loader.c loader.h
	clang -c loader.c

clean:
	rm -rf *.o

clobber: clean
	rm -rf trace
