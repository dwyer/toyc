#include "scanner.h"
#include "token.h"

static int is_letter(int ch)
{
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static int is_digit(int ch)
{
    return '0' <= ch && ch <= '9';
}

static void next(struct scanner *s)
{
    if (!s->ch)
        return;
    s->column++;
    s->ch = s->src[s->offset++];
    if (s->ch == '\n') {
        s->column = 1;
        s->line++;
        /* TODO: insert semicolon */
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

static int scan_identifier(struct scanner *s, char *lit)
{
    int i = 0;
    while (is_letter(s->ch) || is_digit(s->ch)) {
        lit[i++] = s->ch;
        next(s);
    }
    lit[i] = '\0';
    return i;
}

static token_t scan_number(struct scanner *s, char *lit)
{
    while (is_digit(s->ch)) {
        *lit++ = s->ch;
        next(s);
    };
    *lit = '\0';
    return token_INT;
}

static void skip_whitespace(struct scanner *s)
{
    while (s->ch == ' ' || s->ch == '\t' || s->ch == '\n' || s->ch == '\r')
        next(s);
}

extern token_t scanner_scan(struct scanner *s, char *lit)
{
    char *st = lit;
    token_t tok;
    skip_whitespace(s);
    if (is_letter(s->ch)) {
        if (scan_identifier(s, lit) > 1) {
            tok = token_lookup(lit);
        } else {
            tok = token_IDENT;
        }
    } else if (is_digit(s->ch)) {
        tok = scan_number(s, lit);
    } else {
        tok = s->ch;
        if (s->ch != '\0')
            *st++ = s->ch;
        *st = '\0';
        next(s);
    }
    return tok;
}
