#include "scanner.h"
#include "token.h"
#include "log.h" // PANIC

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
    if (s->ch == token_EOF)
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
        int ch = s->ch;
        next(s);
        switch (ch) {
        case '\0':
            return token_EOF;
        case '!':
            return token_NOT;
        case '(':
            return token_LPAREN;
        case ')':
            return token_RPAREN;
        case '*':
            return token_MUL;
        case '+':
            return token_ADD;
        case '-':
            return token_SUB;
        case '.':
            return token_PERIOD;
        case '/':
            return token_QUO;
        case ';':
            return token_SEMICOLON;
        case '{':
            return token_LBRACE;
        case '}':
            return token_RBRACE;
        case '~':
            return token_BITWISE_NOT;
        default:
            PANIC("shit is illegal yo: `%c'\n", ch);
        }
    }
    return tok;
}
