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

static token_t switch2(struct scanner *s, token_t tok0, token_t tok1)
{
    if (s->ch == '=') {
        next(s);
        return tok1;
    }
    return tok0;
}

static token_t switch3(struct scanner *s, token_t tok0, token_t tok1, int ch2,
        token_t tok2)
{
    if (s->ch == '=') {
        next(s);
        return tok1;
    }
    if (s->ch == ch2) {
        next(s);
        return tok2;
    }
    return tok0;
}

static token_t switch4(struct scanner *s, token_t tok0, token_t tok1, int ch2,
        token_t tok2, token_t tok3)
{
    if (s->ch == '=') {
        next(s);
        return tok1;
    }
    if (s->ch == ch2) {
        next(s);
        if (s->ch == '=') {
            next(s);
            return tok3;
        }
        return tok2;
    }
    return tok0;
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
            tok = token_EOF;
            break;
        case '(':
            tok = token_LPAREN;
            break;
        case ')':
            tok = token_RPAREN;
            break;
        case '*':
            tok = token_MUL;
            break;
        case '+':
            tok = token_ADD;
            break;
        case '-':
            tok = token_SUB;
            break;
        case '.':
            tok = token_PERIOD;
            break;
        case '/':
            tok = token_QUO;
            break;
        case ';':
            tok = token_SEMICOLON;
            break;
        case '{':
            tok = token_LBRACE;
            break;
        case '}':
            tok = token_RBRACE;
            break;
        case '~':
            tok = token_BITWISE_NOT;
            break;
        case '<':
            tok = switch4(s, token_LSS, token_LEQ, '>', token_SHL, token_SHL_ASSIGN);
            break;
        case '>':
            tok = switch4(s, token_GTR, token_GEQ, '>', token_SHR, token_SHR_ASSIGN);
            break;
        case '=':
            tok = switch2(s, token_ASSIGN, token_EQL);
            break;
        case '!':
            tok = switch2(s, token_NOT, token_NEQ);
            break;
        case '&':
            tok = switch3(s, token_AND, token_AND_ASSIGN, '&', token_LAND);
            break;
        case '|':
            tok = switch3(s, token_OR, token_OR_ASSIGN, '|', token_LOR);
            break;
        default:
            PANIC("shit is illegal yo: `%c'\n", ch);
        }
    }
    return tok;
}
