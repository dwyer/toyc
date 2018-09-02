#include <stdlib.h> // malloc
#include <stdio.h> // FILE, fopen, etc.

#include "crawl.h"
#include "parser.h"

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
    if (argc < 2)
        return 1;

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return 2;

    char *src = NULL;
    int src_len = freadall(fp, &src);
    fclose(fp);

    file_t *f = parse_file(filename, src, src_len);
    crawl_file(f);

    free(src);

    return 0;
}
