CFLAGS+=-Wall
CFLAGS+=-DLOG_LEVEL=2
LDFLAGS+=-lda

main: ast.o emit_c.o emit_obfc.o main.o parser.o scanner.o token.o

ast.o: ast.h token.h
emit_obfc.o: ast.h emit.h token.h
emit_c.o: ast.h emit.h token.h
main.o: ast.h emit.h parser.h token.h
scanner.o: scanner.h token.h
parser.o: ast.h parser.h scanner.h token.h
token.o: token.h

.PHONY: clean test

clean:
	$(RM) main *.o

test: main
	./test_compiler.sh ./kcc
