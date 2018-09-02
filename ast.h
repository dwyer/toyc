#ifndef EXPR_H
#define EXPR_H

#define NODE int t;

typedef struct _decl decl_t;
typedef struct _expr expr_t;
typedef struct _file file_t;
typedef struct _stmt stmt_t;

enum {
    _EXPR_UNDEFINED = 0,

    EXPR_BASIC,
    EXPR_BINARY,
    EXPR_IDENT,
    EXPR_STRUCT,
    EXPR_UNARY,

    STMT_BLOCK,
    STMT_DECL,
    STMT_EMPTY,
    STMT_RETURN,

    DECL_FUNC,
    DECL_TYPE,
    DECL_VAR,
};

struct _expr {
    NODE;

    union {

        struct {
            int kind; // token
            char *value; // buf
        } basic;

        struct {
            int op;
            expr_t *expr1;
            expr_t *expr2;
        } binary;

        struct {
            char *name;
        } ident;

        struct {
            decl_t **fields;
        } struct_;

        struct {
            int op;
            expr_t *expr;
        } unary;

    } expr;

};

struct _stmt {
    NODE;
    union {

        struct {
            stmt_t **stmts;
        } block;

        struct {
            decl_t *decl;
        } decl;

        struct {
            expr_t *expr;
        } return_;

    } stmt;
};

struct _decl {
    NODE;

    union {

        struct {
            expr_t *recv;
            expr_t *name;
            expr_t *type;
            stmt_t *body;
        } func;

        struct {
            expr_t *name;
            expr_t *type;
        } type;

        struct {
            expr_t *name;
            expr_t *type;
            expr_t *value;
        } var;

    } decl;
};

struct _file {
    decl_t **decls;
};

#endif
