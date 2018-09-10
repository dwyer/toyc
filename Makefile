CFLAGS+=-Wall
CFLAGS+=-DLOG_LEVEL=2
CFLAGS+=-I/usr/local/include
CFLAGS+=-D_GNU_SOURCE

LDFLAGS+=-L/usr/local/lib

main: ast.o emit_c.o emit_x64.o main.o parser.o scanner.o token.o -lda

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
