#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

struct scanner {
    const char *src;
    int src_len;
    int offset;
    int ch;
    int isdelim;
    int iswhite;
    int line;
    int column;
};

extern void scanner_init(struct scanner *s, char *src, int len);
extern token_t scanner_scan(struct scanner *s, char *lit);

#endif
