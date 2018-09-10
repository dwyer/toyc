CFLAGS+=-Wall
CFLAGS+=-I/usr/local/include
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-DLOG_LEVEL=2

LDFLAGS+=-L/usr/local/lib
LDLIBS+=-lda

main: ast.o emit_c.o emit_x64.o main.o parser.o scanner.o token.o

ast.o: ast.h token.h
emit_c.o: ast.h emit.h token.h
emit_x64.o: ast.h emit.h token.h
main.o: ast.h emit.h parser.h token.h
parser.o: ast.h parser.h scanner.h token.h
scanner.o: scanner.h token.h
token.o: token.h

.PHONY: clean test

clean:
	$(RM) main *.o

test: main
	./test_compiler.sh ./kcc
