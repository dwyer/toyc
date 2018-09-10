#include <stdlib.h> // malloc
#include <stdio.h> // FILE, fopen, etc.
#include <string.h> // strcmp

#include "emit.h"
#include "parser.h"

static enum emitter {
    EMIT_C,
    EMIT_OBFC,
    EMIT_X64,
} emitter = EMIT_C;

int freadall(FILE *fp, char **sp)
{
    int n = 0;
    int cap = 0;
    int ch = 0;
    while ((ch = fgetc(fp)) != EOF) {
        if (!*sp) {
            cap = 8;
            *sp = malloc(sizeof(**sp) * cap);
        } else if (n == cap) {
            cap *= 2;
            *sp = realloc(*sp, sizeof(**sp) * cap);
        }
        (*sp)[n++] = ch;
    }
    if (*sp)
        (*sp)[n] = '\0';
    return n;
}

int main(int argc, char *argv[])
{
    const char *progname = *argv;
    (void)progname;

    while (*++argv) {
        if (!strcmp(*argv, "--emit-c")) {
            emitter = EMIT_C;
        } else if (!strcmp(*argv, "--emit-obfc")) {
            emitter = EMIT_OBFC;
        } else if (!strcmp(*argv, "--emit-x64")) {
            emitter = EMIT_X64;
        } else {
            if (argc < 1)
                return 1;

            const char *filename = *argv;
            FILE *fp = fopen(filename, "r");
            if (!fp)
                return 2;

            char *src = NULL;
            int src_len = freadall(fp, &src);
            fclose(fp);

            file_t *f = parse_file(filename, src, src_len);
            crawler_t crawler = {.fp = stdout};

            switch (emitter) {
            case EMIT_C:
                emit_c(&crawler, f);
                break;
            case EMIT_OBFC:
                emit_obfc(&crawler, f);
                break;
            case EMIT_X64:
                emit_x64(&crawler, f);
                break;
            }

            free(src);
        }
    }

    return 0;
}
