#ifndef EXPR_H
#define EXPR_H

typedef struct _node node_t;

typedef struct _file file_t;

typedef enum {
    NODE_UNDEFINED = 0,

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
} node_type_t;

struct _node {

    node_type_t t;

    union {

        union { // expr

            struct {
                int kind; // token
                char *value; // buf
            } basic;

            struct {
                int op;
                node_t *expr1;
                node_t *expr2;
            } binary;

            struct {
                char *name;
            } ident;

            struct {
                node_t **fields;
            } struct_;

            struct {
                int op;
                node_t *expr;
            } unary;

        } expr;

        union { // stmt

            struct {
                node_t **stmts;
            } block;

            struct {
                node_t *decl;
            } decl;

            struct {
                node_t *expr;
            } return_;

        } stmt;

        union { // decl

            struct {
                node_t *recv;
                node_t *name;
                node_t *type;
                node_t *body;
            } func;

            struct {
                node_t *name;
                node_t *type;
            } type;

            struct {
                node_t *name;
                node_t *type;
                node_t *value;
            } var;

        } decl;
    };
};

struct _file {
    node_t **decls;
};

#endif
