#########################################################
#  file name   : Makefile                               #
#  author      : Alex Dreier                            #
#  description : Makefile to automate compiling         #
#                and linking                            #
#########################################################
#
#

all: assembler

assembler: assembler.c asm_parser.o
	clang assembler.c asm_parser.o -o assembler

asm_parser.o: asm_parser.c asm_parser.h
	clang -c asm_parser.c

clean:
	rm -rf *.o

clobber: clean
	rm -rf assembler
