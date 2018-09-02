#include "ast.h"
#include "log.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

#include <stdlib.h> // alloc
#include <stdio.h> // BUFSIZ
#include <string.h> // memcpy

#include <da.h>

#define P_PANIC(p, fmt, ...) \
    PANIC("%s:%d:%d: " fmt, (p)->filename, (p)->scanner.line, (p)->scanner.column, ## __VA_ARGS__)

#define memdup(p, size) ({void *vp = malloc((size)); memcpy(vp, (p), (size)); vp;})
#define copy(p) memdup((p), sizeof(*(p)))

typedef struct {
    const char *filename;
    struct scanner scanner;
    int tok;
    char lit[BUFSIZ];
} parser_t;

static node_t *parse_decl(parser_t *p);
static node_t *parse_expr(parser_t *p);

DA_DEF_HELPERS(node, node_t *);

static void next(parser_t *p)
{
    p->tok = scanner_scan(&p->scanner, p->lit);
    LOGV("%d:%d, tok %d, \"%s\"", p->scanner.line, p->scanner.column, p->tok, p->lit);
}

static int accept(parser_t *p, token_t tok)
{
    if (p->tok == tok) {
        next(p);
        return 1;
    }
    return 0;
}

static void expect(parser_t *p, token_t tok)
{
    if (accept(p, tok))
        return;
    if (tok < 128)
        P_PANIC(p, "expected \"%c\", got \"%s\"", tok, p->lit);
    else
        P_PANIC(p, "expected token %d, got \"%s\"", tok, p->lit);
}

static void parse_arg_list(parser_t *p)
{
    expect(p, '(');
    while (p->tok != ')') {
        parse_expr(p);
        if (p->tok != ',')
            break;
        next(p);
    }
    expect(p, ')');
}

static node_t *parse_ident(parser_t *p)
{
    node_t tmp = {.t = EXPR_IDENT};
    if (p->tok == TOKEN_IDENT) {
        tmp.expr.ident.name = strdup(p->lit);
        next(p);
    } else {
        expect(p, TOKEN_IDENT);
        tmp.expr.ident.name = strdup("_");
    }
    return copy(&tmp);
}

static node_t *parse_operand(parser_t *p)
{
    switch (p->tok) {
    case TOKEN_IDENT:
        return parse_ident(p);
    case TOKEN_NUMBER:
        do {
            node_t tmp = {
                .t = EXPR_BASIC,
                .expr.basic = {
                    .kind = p->tok,
                    .value = strdup(p->lit),
                },
            };
            next(p);
            return copy(&tmp);
        } while (0);
    case '(':
        do {
            next(p);
            node_t *x = parse_expr(p);
            expect(p, ')');
            node_t tmp = {
                .t = EXPR_PAREN,
                .expr.paren.x = x,
            };
            return copy(&tmp);
        } while (0);
        break;
    default:
        P_PANIC(p, "expected expr, got %d", p->tok);
        break;
    }
    return NULL;
}

static node_t *parse_primary_expr(parser_t *p)
{
    return parse_operand(p);
}

static node_t *parse_unary_expr(parser_t *p)
{
    switch (p->tok) {
    case '!':
    case '+':
    case '-':
    case '~':
        do {
            node_t tmp = {
                .t = EXPR_UNARY,
                .expr.unary.op = p->tok,
            };
            next(p);
            tmp.expr.unary.expr = parse_expr(p);
            return copy(&tmp);
        } while (0);
    }
    return parse_primary_expr(p);
}

static node_t *parse_binary_expr(parser_t *p)
{
    node_t *x = parse_unary_expr(p);
    switch (p->tok) {
    case '*':
    case '+':
    case '-':
    case '/':
        do {
            int op = p->tok;
            expect(p, op);
            node_t *y = parse_binary_expr(p);
            node_t tmp = {
                .t = EXPR_BINARY,
                .expr.binary = {
                    .op = op,
                    .x = x,
                    .y = y,
                },
            };
            x = copy(&tmp);
        } while (0);
        break;
    }
    return x;
}

static node_t *parse_expr(parser_t *p)
{
    return parse_binary_expr(p);
}

static node_t *parse_return_stmt(parser_t *p)
{
    expect(p, TOKEN_RETURN);
    node_t tmp = {
        .t = STMT_RETURN,
        .stmt.return_.expr = p->tok == ';' ? NULL : parse_expr(p),
    };
    expect(p, ';');
    return copy(&tmp);
}

static node_t *parse_stmt(parser_t *p)
{
    switch (p->tok) {
    case TOKEN_RETURN:
        return parse_return_stmt(p);
    case TOKEN_VAR:
    case TOKEN_TYPE:
        do {
            node_t tmp = {
                .t = STMT_DECL,
                .stmt.decl.decl = parse_decl(p),
            };
            return copy(&tmp);
        } while (0);
    case ';':
        do {
            node_t tmp = { .t = STMT_EMPTY, };
            next(p);
            return copy(&tmp);
        } while (0);
    }
    P_PANIC(p, "expected statement: got %s", p->lit);
    return NULL;
}

static node_t *parse_block_stmt(parser_t *p)
{
    expect(p, '{');
    da_t stmts;
    da_init_node(&stmts);
    while (p->tok != '}') {
        da_append_node(&stmts, parse_stmt(p));
    }
    da_append_node(&stmts, NULL);
    expect(p, '}');
    node_t tmp = {
        .t=STMT_BLOCK,
        .stmt.block.stmts = stmts.data,
    };
    return copy(&tmp);
}

static node_t *parse_func_decl(parser_t *p)
{
    expect(p, TOKEN_FUNC);
    node_t *recv = NULL;
    node_t *ident = parse_ident(p);
    if (accept(p, '.')) {
        recv = ident;
        ident = parse_ident(p);
    }
    expect(p, '(');
    expect(p, ')');
    node_t tmp = {
        .t = DECL_FUNC,
        .decl.func = {
            .recv = recv,
            .name = ident,
            .type = parse_ident(p),
            .body = parse_block_stmt(p),
        },
    };
    return copy(&tmp);
}

static node_t *parse_struct_type(parser_t *p)
{
    expect(p, TOKEN_STRUCT);
    expect(p, '{');
    da_t fields;
    da_init_node(&fields);
    while (p->tok != '}') {
        da_append_node(&fields, parse_decl(p));
    }
    da_append_node(&fields, NULL);
    expect(p, '}');
    node_t tmp = {
        .t = EXPR_STRUCT,
        .expr.struct_.fields = da_get(&fields, 0),
    };
    return copy(&tmp);
}

static node_t *parse_type_expr(parser_t *p)
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

static node_t *parse_type_decl(parser_t *p)
{
    expect(p, TOKEN_TYPE);
    node_t tmp = {
        .t = DECL_TYPE,
        .decl.type = {
            .name = parse_ident(p),
            .type = parse_type_expr(p),
        },
    };
    expect(p, ';');
    return copy(&tmp);
}

static node_t *parse_var_decl(parser_t *p)
{
    expect(p, TOKEN_VAR);
    node_t tmp = {
        .t = DECL_VAR,
        .decl.var = {
            .name = parse_ident(p),
            .type = parse_ident(p),
            .value = accept(p, '=') ? parse_expr(p) : NULL,
        },
    };
    expect(p, ';');
    return copy(&tmp);
}

static node_t *parse_decl(parser_t *p)
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
    da_init_node(&decls);
    next(p);
    while (p->tok) {
        da_append_node(&decls, parse_decl(p));
    }
    da_append_node(&decls, NULL);
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
