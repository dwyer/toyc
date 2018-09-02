#ifndef EXPR_H
#define EXPR_H

typedef struct _node node_t;

typedef node_t decl_t __attribute__((deprecated));
typedef node_t expr_t __attribute__((deprecated));
typedef node_t stmt_t __attribute__((deprecated));

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

        union { // stmt

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

        union { // decl

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
};

struct _file {
    decl_t **decls;
};

#endif
