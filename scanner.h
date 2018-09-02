#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

typedef struct {
    const char *src;
    int src_len;
    int offset;
    int ch;
    int line;
    int column;
} scanner_t;

extern void scanner_init(scanner_t *s, char *src, int len);
extern token_t scanner_scan(scanner_t *s, char *lit);

#endif
