#ifndef EXPR_H
#define EXPR_H

#include "token.h"

#include <da/da.h>
#include <da/da_str.h>

#include <stdlib.h> // malloc
#include <string.h> // memcpy

#define memdup(p, size) ({void *vp = malloc((size)); memcpy(vp, (p), (size)); vp;})
#define copy(p) memdup((p), sizeof(*(p)))

typedef struct _node node_t;

typedef struct _file file_t;

typedef enum {
    NODE_UNDEFINED = 0,

    EXPR_BASIC,
    EXPR_BINARY,
    EXPR_CALL,
    EXPR_FIELD,
    EXPR_IDENT,
    EXPR_PAREN,
    EXPR_STRUCT,
    EXPR_UNARY,

    STMT_ASSIGN,
    STMT_BLOCK,
    STMT_BRANCH,
    STMT_DECL,
    STMT_EMPTY,
    STMT_EXPR,
    STMT_FOR,
    STMT_IF,
    STMT_RETURN,

    DECL_FUNC,
    DECL_TYPE,
    DECL_VAR,
} node_type_t;

struct _node {

    node_type_t t;
    int pos;

    union {

        union { // expr

            struct {
                int kind; // token
                char *value; // buf
            } basic;

            struct {
                int op;
                node_t *x;
                node_t *y;
            } binary;

            struct {
                node_t *func;
                node_t **args;
            } call;

            struct {
                node_t *name;
                node_t *type;
            } field;

            struct {
                char *name;
            } ident;

            struct {
                node_t *x;
            } paren;

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
                node_t *lhs;
                token_t tok;
                node_t *rhs;
            } assign;

            struct {
                node_t **stmts;
            } block;

            struct {
                token_t tok;
            } branch;

            struct {
                node_t *decl;
            } decl;

            struct {
                node_t *x;
            } expr;

            struct {
                node_t *init;
                node_t *cond;
                node_t *post;
                node_t *body;
            } for_;

            struct {
                node_t *cond;
                node_t *body;
                node_t *else_;
            } if_;

            struct {
                node_t *expr;
            } return_;

        } stmt;

        union { // decl

            struct {
                node_t *recv;
                node_t *name;
                node_t **params;
                node_t *type;
                node_t *body;
            } func;

            struct {
                node_t *spec;
            } gen;

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

typedef struct _scope {
    struct _scope *outer;
    da_t list;
} scope_t;

extern scope_t *ast_new_scope(scope_t *outer);

#endif
