#ifndef SCANNER_H
#define SCANNER_H

struct scanner {
    char *src;
    int src_len;
    int offset;
    int ch;
    int isdelim;
    int iswhite;
    int lineno;
    int colno;
};

extern void scanner_init(struct scanner *s, char *src, int len);
extern int scanner_scan(struct scanner *s, char *lit);

#endif
