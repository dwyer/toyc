#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    // Special tokens
    token_ILLEGAL,
    token_EOF,
    token_COMMENT,

    token_literal_beg,
    // Identifiers and basic type literals
    // (these tokens stand for classes of literals)
    token_IDENT,  // main
    token_INT,    // 12345
    token_FLOAT,  // 123.45
    token_IMAG,   // 123.45i
    token_CHAR,   // 'a'
    token_STRING, // "abc"
    token_literal_end,

    token_operator_beg,
    // Operators and delimiters
    token_ADD, // +
    token_SUB, // -
    token_MUL, // *
    token_QUO, // /
    token_REM, // %

    token_AND,     // &
    token_OR,      // |
    token_XOR,     // ^
    token_SHL,     // <<
    token_SHR,     // >>
    token_AND_NOT, // &^
    token_BITWISE_NOT,

    token_ADD_ASSIGN, // +=
    token_SUB_ASSIGN, // -=
    token_MUL_ASSIGN, // *=
    token_QUO_ASSIGN, // /=
    token_REM_ASSIGN, // %=

    token_AND_ASSIGN,     // &=
    token_OR_ASSIGN,      // |=
    token_XOR_ASSIGN,     // ^=
    token_SHL_ASSIGN,     // <<=
    token_SHR_ASSIGN,     // >>=
    token_AND_NOT_ASSIGN, // &^=

    token_LAND,  // &&
    token_LOR,   // ||
    token_ARROW, // <-
    token_INC,   // ++
    token_DEC,   // --

    token_EQL,    // ==
    token_LSS,    // <
    token_GTR,    // >
    token_ASSIGN, // =
    token_NOT,    // !

    token_NEQ,      // !=
    token_LEQ,      // <=
    token_GEQ,      // >=
    token_DEFINE,   // :=
    token_ELLIPSIS, // ...

    token_LPAREN, // (
    token_LBRACK, // [
    token_LBRACE, // {
    token_COMMA,  // ,
    token_PERIOD, // .

    token_RPAREN,    // )
    token_RBRACK,    // ]
    token_RBRACE,    // }
    token_SEMICOLON, // ;
    token_COLON,     // :
    token_operator_end,

    token_keyword_beg,
    // Keywords
    token_BREAK,
    token_CASE,
    token_CHAN,
    token_CONST,
    token_CONTINUE,

    token_DEFAULT,
    token_DEFER,
    token_ELSE,
    token_FALLTHROUGH,
    token_FOR,

    token_FUNC,
    token_GO,
    token_GOTO,
    token_IF,
    token_IMPORT,

    token_INTERFACE,
    token_MAP,
    token_PACKAGE,
    token_RANGE,
    token_RETURN,

    token_SELECT,
    token_STRUCT,
    token_SWITCH,
    token_TYPE,
    token_VAR,
    token_keyword_end,
} token_t;

enum {
    token_lowest_prec = 0,
    token_unary_prec  = 6,
    token_highestPrec = 7,
};

extern const char *token_string(token_t tok);
extern int token_precedence(token_t op);
extern token_t token_lookup(const char *ident);
extern int token_is_literal(token_t tok);
extern int token_is_operator(token_t tok);
extern int token_is_keyword(token_t tok);

#endif
