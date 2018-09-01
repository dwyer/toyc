#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    _TOKEN_UNDEFINED = 1000,

    TOKEN_IDENT,
    TOKEN_NUMBER,

    _TOKEN_KEYWORDS_BEGIN,

    TOKEN_FOR,
    TOKEN_FUNC,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_TYPE,
    TOKEN_VAL,
    TOKEN_VAR,

    _TOKEN_KEYWORDS_END,
} token_t;

#endif
