#include "ast.h"
#include "log.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

#include <stdlib.h> // alloc
#include <stdio.h> // BUFSIZ
#include <string.h> // memcpy

#include <da.h>

DA_DEF_HELPERS(decl, node_t *);
DA_DEF_HELPERS(expr, node_t *);
DA_DEF_HELPERS(stmt, node_t *);

typedef struct {
    const char *filename;
    struct scanner scanner;
    int tok;
    char lit[BUFSIZ];
} parser_t;


#define P_PANIC(p, fmt, ...) \
    PANIC("%s:%d:%d: " fmt, (p)->filename, (p)->scanner.line, (p)->scanner.column, ## __VA_ARGS__)

#define memdup(p, size) ({void *vp = malloc((size)); memcpy(vp, (p), (size)); vp;})
#define copy(p) memdup((p), sizeof(*(p)))

void next(parser_t *p)
{
    p->tok = scanner_scan(&p->scanner, p->lit);
    LOGV("%d:%d, tok %d, \"%s\"", p->scanner.line, p->scanner.column, p->tok, p->lit);
}

void expect(parser_t *p, int tok)
{
    if (p->tok != tok) {

        if (tok < 128)
            P_PANIC(p, "expected \"%c\", got \"%s\"", tok, p->lit);
        else
            P_PANIC(p, "expected token %d, got \"%s\"", tok, p->lit);
    }
    next(p);
}

node_t *parse_expr(parser_t *p);

void parse_arg_list(parser_t *p)
{
    LOGV("parse_arg_list");
    expect(p, '(');
    while (p->tok != ')') {
        parse_expr(p);
        if (p->tok != ',')
            break;
        next(p);
    }
    expect(p, ')');
}

node_t *parse_ident(parser_t *p)
{
    node_t expr = {.t = EXPR_IDENT};
    if (p->tok == TOKEN_IDENT) {
        expr.expr.ident.name = strdup(p->lit);
        next(p);
    } else {
        expect(p, TOKEN_IDENT);
        expr.expr.ident.name = strdup("_");
    }
    return copy(&expr);
}

node_t *parse_expr(parser_t *p)
{
    node_t expr = {};
    switch (p->tok) {
    case '!':
    case '+':
    case '-':
    case '~':
        expr.t = EXPR_UNARY;
        expr.expr.unary.op = p->tok;
        next(p);
        expr.expr.unary.expr = parse_expr(p);
        break;
    case TOKEN_IDENT:
        do {
            node_t *ident = parse_ident(p);
            return ident;
            /* next(p); */
            /* if (p->tok == '(') { */
            /*     parse_arg_list(p); */
            /* } */
        } while (0);
        break;
    case TOKEN_NUMBER:
        expr.t = EXPR_BASIC;
        expr.expr.basic.kind = TOKEN_NUMBER;
        expr.expr.basic.value = strdup(p->lit);
        next(p);
        break;
    default:
        P_PANIC(p, "expected expr, got %d", p->tok);
        break;
    }
    return copy(&expr);
}

node_t *parse_return_stmt(parser_t *p)
{
    node_t stmt = {
        .t = STMT_RETURN,
    };
    expect(p, TOKEN_RETURN);
    if (p->tok != ';') {
        stmt.stmt.return_.expr = parse_expr(p);
    }
    expect(p, ';');
    return copy(&stmt);
}

node_t *parse_decl(parser_t *p);

node_t *parse_stmt(parser_t *p)
{
    node_t stmt = {};
    switch (p->tok) {
    case TOKEN_RETURN:
        return parse_return_stmt(p);
    case TOKEN_VAR:
    case TOKEN_TYPE:
        stmt.t = STMT_DECL;
        stmt.stmt.decl.decl = parse_decl(p);
        break;
    case ';':
        stmt.t = STMT_EMPTY;
        next(p);
        break;
    default:
        P_PANIC(p, "expected statement: got %d", p->tok);
        break;
    }
    return copy(&stmt);
}

node_t *parse_block_stmt(parser_t *p)
{
    da_t stmts;
    da_init_stmt(&stmts);
    expect(p, '{');
    while (p->tok != '}') {
        da_append_stmt(&stmts, parse_stmt(p));
    }
    da_append_stmt(&stmts, NULL);
    expect(p, '}');
    node_t stmt = {
        .t=STMT_BLOCK,
        .stmt.block.stmts = stmts.data,
    };
    return copy(&stmt);
}

node_t *parse_func_decl(parser_t *p)
{
    node_t decl = {.t = DECL_FUNC,};
    expect(p, TOKEN_FUNC);
    node_t *ident = parse_ident(p);
    if (p->tok == '.') {
        expect(p, '.');
        decl.decl.func.recv = ident;
        ident = parse_ident(p);
    }
    decl.decl.func.name = ident;
    expect(p, '(');
    expect(p, ')');
    decl.decl.func.type = parse_ident(p);
    decl.decl.func.body = parse_block_stmt(p);
    return copy(&decl);
}

node_t *parse_struct_type(parser_t *p)
{
    expect(p, TOKEN_STRUCT);
    expect(p, '{');
    da_t fields;
    da_init_decl(&fields);
    while (p->tok != '}') {
        da_append_decl(&fields, parse_decl(p));
    }
    da_append_decl(&fields, NULL);
    expect(p, '}');
    node_t expr = {
        .t = EXPR_STRUCT,
        .expr.struct_.fields = da_get(&fields, 0),
    };
    return copy(&expr);
}

node_t *parse_type_expr(parser_t *p)
{
    switch (p->tok) {
    case TOKEN_IDENT:
        return parse_ident(p);
    case TOKEN_STRUCT:
        return parse_struct_type(p);
    default:
        P_PANIC(p, "invalid token for parse_type_expr: %d", p->tok);
        return NULL;
    }
}

node_t *parse_type_decl(parser_t *p)
{
    node_t decl = {.t = DECL_TYPE};
    expect(p, TOKEN_TYPE);
    decl.decl.type.name = parse_ident(p);
    decl.decl.type.type = parse_type_expr(p);
    expect(p, ';');
    return copy(&decl);
}

node_t *parse_var_decl(parser_t *p)
{
    node_t decl = {.t = DECL_VAR};
    expect(p, TOKEN_VAR);
    decl.decl.var.name = parse_ident(p);
    decl.decl.var.type = parse_ident(p);
    decl.decl.var.value = NULL;
    if (p->tok == '=') {
        expect(p, '=');
        decl.decl.var.value = parse_expr(p);
    }
    expect(p, ';');
    return copy(&decl);
}

node_t *parse_decl(parser_t *p)
{
    switch (p->tok) {
    case TOKEN_FUNC:
        return parse_func_decl(p);
    case TOKEN_TYPE:
        return parse_type_decl(p);
    case TOKEN_VAR:
        return parse_var_decl(p);
    case '\0':
        return NULL;
    default:
        P_PANIC(p, "bad tok: %d: %s", p->tok, p->lit);
        break;
    }
}

static file_t *_parse_file(parser_t *p)
{
    da_t decls;
    da_init_decl(&decls);
    next(p);
    while (p->tok) {
        da_append_decl(&decls, parse_decl(p));
    }
    da_append_decl(&decls, NULL);
    file_t file = {.decls = decls.data};
    return copy(&file);
}

static void init(parser_t *p, const char *filename, char *src, int src_len)
{
    p->filename = filename;
    scanner_init(&p->scanner, src, src_len);
}

extern file_t *parse_file(const char *filename, char *src, int src_len)
{
    parser_t parser = {};
    init(&parser, filename, src, src_len);
    return _parse_file(&parser);
}
