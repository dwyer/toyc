#include "token.h"

#include <stddef.h> // NULL
#include <string.h> // strcmp

#define streq(a, b) (!strcmp((a), (b)))

static struct token_string_pair {
    token_t tok;
    const char *str;
} tokens[] = {
    {token_ILLEGAL, "ILLEGAL"},

    {token_EOF,     "EOF"},
    {token_COMMENT, "COMMENT"},

    {token_IDENT,  "IDENT"},
    {token_INT,    "INT"},
    {token_FLOAT,  "FLOAT"},
    {token_IMAG,   "IMAG"},
    {token_CHAR,   "CHAR"},
    {token_STRING, "STRING"},

    {token_ADD, "+"},
    {token_SUB, "-"},
    {token_MUL, "*"},
    {token_QUO, "/"},
    {token_REM, "%"},

    {token_AND,     "&"},
    {token_OR,      "|"},
    {token_XOR,     "^"},
    {token_SHL,     "<<"},
    {token_SHR,     ">>"},
    {token_AND_NOT, "&^"},
    {token_BITWISE_NOT, "~"},

    {token_ADD_ASSIGN, "+="},
    {token_SUB_ASSIGN, "-="},
    {token_MUL_ASSIGN, "*="},
    {token_QUO_ASSIGN, "/="},
    {token_REM_ASSIGN, "%="},

    {token_AND_ASSIGN,     "&="},
    {token_OR_ASSIGN,      "|="},
    {token_XOR_ASSIGN,     "^="},
    {token_SHL_ASSIGN,     "<<="},
    {token_SHR_ASSIGN,     ">>="},
    {token_AND_NOT_ASSIGN, "&^="},

    {token_LAND,  "&&"},
    {token_LOR,   "||"},
    {token_ARROW, "<-"},
    {token_INC,   "++"},
    {token_DEC,   "--"},

    {token_EQL,    "=="},
    {token_LSS,    "<"},
    {token_GTR,    ">"},
    {token_ASSIGN, "="},
    {token_NOT,    "!"},

    {token_NEQ,      "!="},
    {token_LEQ,      "<="},
    {token_GEQ,      ">="},
    {token_DEFINE,   ":="},
    {token_ELLIPSIS, "..."},

    {token_LPAREN, "("},
    {token_LBRACK, "["},
    {token_LBRACE, "{"},
    {token_COMMA,  ","},
    {token_PERIOD, "."},

    {token_RPAREN,    ")"},
    {token_RBRACK,    "]"},
    {token_RBRACE,    "}"},
    {token_SEMICOLON, ";"},
    {token_COLON,     ":"},

    {token_BREAK,    "break"},
    {token_CASE,     "case"},
    {token_CHAN,     "chan"},
    {token_CONST,    "const"},
    {token_CONTINUE, "continue"},

    {token_DEFAULT,     "default"},
    {token_DEFER,       "defer"},
    {token_ELSE,        "else"},
    {token_FALLTHROUGH, "fallthrough"},
    {token_FOR,         "for"},

    {token_FUNC,   "func"},
    {token_GO,     "go"},
    {token_GOTO,   "goto"},
    {token_IF,     "if"},
    {token_IMPORT, "import"},

    {token_INTERFACE, "interface"},
    {token_MAP,       "map"},
    {token_PACKAGE,   "package"},
    {token_RANGE,     "range"},
    {token_RETURN,    "return"},

    {token_SELECT, "select"},
    {token_STRUCT, "struct"},
    {token_SWITCH, "switch"},
    {token_TYPE,   "type"},
    {token_VAR,    "var"},
    {0, NULL},
};

extern const char *token_string(token_t tok)
{
    struct token_string_pair *pairs = tokens;
    while (pairs->str) {
        if (tok == pairs->tok)
            return pairs->str;
        ++pairs;
    }
    return NULL;
}

extern int token_precedence(token_t op)
{
    switch (op) {
    case token_LOR:
        return 1;
    case token_LAND:
        return 2;
    case token_EQL:
    case token_NEQ:
        return 3;
    case token_LSS:
    case token_LEQ:
    case token_GTR:
    case token_GEQ:
        return 4;
    case token_ADD:
    case token_SUB:
    case token_OR:
    case token_XOR:
        return 5;
    case token_MUL:
    case token_QUO:
    case token_REM:
    case token_SHL:
    case token_SHR:
    case token_AND:
    case token_AND_NOT:
        return 6;
    default:
        return token_lowest_prec;
    }
}

struct {
    const char *str;
    token_t token;
} keywords[token_keyword_end-token_keyword_beg];

extern token_t token_lookup(const char *ident)
{
    if (streq(ident, "break"))
        return token_BREAK;
    if (streq(ident, "continue"))
        return token_CONTINUE;
    if (streq(ident, "else"))
        return token_ELSE;
    if (streq(ident, "for"))
        return token_FOR;
    if (streq(ident, "func"))
        return token_FUNC;
    if (streq(ident, "if"))
        return token_IF;
    if (streq(ident, "return"))
        return token_RETURN;
    if (streq(ident, "struct"))
        return token_STRUCT;
    if (streq(ident, "type"))
        return token_TYPE;
    if (streq(ident, "var"))
        return token_VAR;
    return token_IDENT;
}

extern int token_is_literal(token_t tok)
{
    return token_literal_beg < tok && tok < token_literal_end;
}

extern int token_is_operator(token_t tok)
{
    return token_operator_beg < tok && tok < token_operator_end;
}

extern int token_is_keyword(token_t tok)
{
    return token_keyword_beg < tok && tok < token_keyword_end;
}
