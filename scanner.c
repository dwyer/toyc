#include "scanner.h"
#include "token.h"

#include <ctype.h> // isalpha, isdigit
#include <string.h> // strcmp

#define streq(a, b) (!strcmp((a), (b)))

static void next(struct scanner *s)
{
    if (!s->ch)
        return;
    s->isdelim = 0;
    s->iswhite = 0;
    s->column++;
    s->ch = s->src[s->offset++];
    switch (s->ch) {
    case '\n':
        s->column = 1;
        s->line++;
        /* TODO: insert semicolon */
    case '\r':
    case '\t':
    case ' ':
        s->iswhite = 1;
    case '\0':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ';':
    case '=':
    case '{':
    case '}':
        s->isdelim = 1;
        break;
    }
}

extern void scanner_init(struct scanner *s, char *src, int len)
{
    s->line = 1;
    s->column = 1;
    s->ch = -1;
    s->src = src;
    s->src_len = len;
    next(s);
}

extern token_t scanner_scan(struct scanner *s, char *lit)
{
    char *st = lit;
    int tok;
    while (s->iswhite) {
        next(s);
    }
    if (s->isdelim) {
        tok = s->ch;
        if (s->ch != '\0')
            *st++ = s->ch;
        next(s);
    } else {
        if (isalpha(s->ch) || s->ch == '_')
            tok = TOKEN_IDENT;
        else if (isdigit(s->ch))
            tok = TOKEN_NUMBER;
        do {
            *st++ = s->ch;
            next(s);
        } while (!s->isdelim);
    }
    *st = '\0';
    if (tok == TOKEN_IDENT) {
        if (streq(lit, "for"))
            tok = TOKEN_FOR;
        else if (streq(lit, "func"))
            tok = TOKEN_FUNC;
        else if (streq(lit, "return"))
            tok = TOKEN_RETURN;
        else if (streq(lit, "struct"))
            tok = TOKEN_STRUCT;
        else if (streq(lit, "type"))
            tok = TOKEN_TYPE;
        else if (streq(lit, "var"))
            tok = TOKEN_VAR;
    }
    return tok;
}
