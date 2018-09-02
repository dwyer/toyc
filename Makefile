CFLAGS+=-Wall

CFLAGS+=-DLOG_LEVEL=2

CFLAGS+=-I../repo/mono/lib/da
LDFLAGS+=-L../repo/mono/lib/da -lda

main: main.o crawl.o parser.o scanner.o

crawl.o: ast.h crawl.h
main.o: ast.h crawl.h parser.h
scanner.o: scanner.h token.h
parser.o: ast.h parser.h scanner.h token.h

.PHONY: clean test

clean:
	$(RM) main *.o

test: main
	./test_compiler.sh ./kcc
