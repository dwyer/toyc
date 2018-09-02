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
    scanner_t scanner;
    token_t tok;
    char lit[BUFSIZ];
} parser_t;

static node_t *parse_decl(parser_t *p);
static node_t *parse_expr(parser_t *p);
static node_t *parse_stmt(parser_t *p);

DA_DEF_HELPERS(node, node_t *);

static void next(parser_t *p)
{
    p->tok = scanner_scan(&p->scanner, p->lit);
    LOGV("%d:%d, tok %s", p->scanner.line, p->scanner.column, token_string(p->tok));
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
    P_PANIC(p, "expected %s, got %s", token_string(tok), token_string(p->tok));
}

static void parse_arg_list(parser_t *p)
{
    expect(p, token_LPAREN);
    while (p->tok !=  token_RPAREN) {
        parse_expr(p);
        if (p->tok != token_COMMA)
            break;
        next(p);
    }
    expect(p, token_RPAREN);
}

static node_t *parse_ident(parser_t *p)
{
    node_t tmp = {.t = EXPR_IDENT};
    if (p->tok == token_IDENT) {
        tmp.expr.ident.name = strdup(p->lit);
        next(p);
    } else {
        expect(p, token_IDENT);
        tmp.expr.ident.name = strdup("_");
    }
    return copy(&tmp);
}

static node_t *parse_operand(parser_t *p)
{
    switch (p->tok) {
    case token_IDENT:
        return parse_ident(p);
    case token_INT:
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
    case token_LPAREN:
        do {
            next(p);
            node_t *x = parse_expr(p);
            expect(p, token_RPAREN);
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
    case token_NOT:
    case token_ADD:
    case token_SUB:
    case token_BITWISE_NOT:
        do {
            node_t tmp = {
                .t = EXPR_UNARY,
                .expr.unary.op = p->tok,
            };
            next(p);
            tmp.expr.unary.expr = parse_expr(p);
            return copy(&tmp);
        } while (0);
    default:
        return parse_primary_expr(p);
    }
}

static node_t *parse_binary_expr(parser_t *p)
{
    node_t *x = parse_unary_expr(p);
    switch (p->tok) {
    case token_ADD:
    case token_EQL:
    case token_GEQ:
    case token_GTR:
    case token_LAND:
    case token_LEQ:
    case token_LOR:
    case token_LSS:
    case token_MUL:
    case token_NEQ:
    case token_QUO:
    case token_SUB:
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
    default:
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
    expect(p, token_RETURN);
    node_t tmp = {
        .t = STMT_RETURN,
        .stmt.return_.expr = p->tok == token_SEMICOLON ? NULL : parse_expr(p),
    };
    expect(p, token_SEMICOLON);
    return copy(&tmp);
}

static node_t *parse_simple_stmt(parser_t *p)
{
    node_t *lhs = parse_ident(p);
    expect(p, token_ASSIGN);
    node_t *rhs = parse_expr(p);
    expect(p, token_SEMICOLON);
    node_t tmp = {
        .t = STMT_ASSIGN,
        .stmt.assign = {
            .lhs = lhs,
            .tok = token_ASSIGN,
            .rhs = rhs,
        }
    };
    return copy(&tmp);
}

static node_t *parse_block_stmt(parser_t *p)
{
    expect(p, token_LBRACE);
    da_t stmts;
    da_init_node(&stmts);
    while (p->tok != token_RBRACE) {
        da_append_node(&stmts, parse_stmt(p));
    }
    da_append_node(&stmts, NULL);
    expect(p, token_RBRACE);
    node_t tmp = {
        .t=STMT_BLOCK,
        .stmt.block.stmts = stmts.data,
    };
    return copy(&tmp);
}

static node_t *parse_if_stmt(parser_t *p)
{
    expect(p, token_IF);
    node_t *cond = parse_expr(p);
    node_t *body = parse_block_stmt(p);
    node_t *else_ = NULL;
    if (accept(p, token_ELSE)) {
        if (p->tok == token_IF) {
            else_ = parse_if_stmt(p);
        } else {
            else_ = parse_block_stmt(p);
        }
    }
    node_t tmp = {
        .t = STMT_IF,
        .stmt.if_ = {
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return copy(&tmp);
}

static node_t *parse_stmt(parser_t *p)
{
    switch (p->tok) {
    case token_VAR:
    case token_TYPE:
        do {
            node_t tmp = {
                .t = STMT_DECL,
                .stmt.decl.decl = parse_decl(p),
            };
            return copy(&tmp);
        } while (0);
    case token_IDENT:
        return parse_simple_stmt(p);
    case token_RETURN:
        return parse_return_stmt(p);
    case token_LBRACE:
        return parse_block_stmt(p);
    case token_IF:
        return parse_if_stmt(p);
    case token_SEMICOLON:
        do {
            node_t tmp = { .t = STMT_EMPTY, };
            next(p);
            return copy(&tmp);
        } while (0);
    default:
        P_PANIC(p, "expected statement: got %s", p->lit);
        return NULL;
    }
}

static node_t *parse_func_decl(parser_t *p)
{
    expect(p, token_FUNC);
    node_t *recv = NULL;
    node_t *ident = parse_ident(p);
    if (accept(p, token_PERIOD)) {
        recv = ident;
        ident = parse_ident(p);
    }
    expect(p, token_LPAREN);
    expect(p, token_RPAREN);
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
    expect(p, token_STRUCT);
    expect(p, token_LBRACE);
    da_t fields;
    da_init_node(&fields);
    while (p->tok != token_RBRACE) {
        da_append_node(&fields, parse_decl(p));
    }
    da_append_node(&fields, NULL);
    expect(p, token_RBRACE);
    node_t tmp = {
        .t = EXPR_STRUCT,
        .expr.struct_.fields = da_get(&fields, 0),
    };
    return copy(&tmp);
}

static node_t *parse_type_expr(parser_t *p)
{
    switch (p->tok) {
    case token_IDENT:
        return parse_ident(p);
    case token_STRUCT:
        return parse_struct_type(p);
    default:
        P_PANIC(p, "invalid token for parse_type_expr: %d", p->tok);
        return NULL;
    }
}

static node_t *parse_type_decl(parser_t *p)
{
    expect(p, token_TYPE);
    node_t tmp = {
        .t = DECL_TYPE,
        .decl.type = {
            .name = parse_ident(p),
            .type = parse_type_expr(p),
        },
    };
    expect(p, token_SEMICOLON);
    return copy(&tmp);
}

static node_t *parse_var_decl(parser_t *p)
{
    expect(p, token_VAR);
    node_t tmp = {
        .t = DECL_VAR,
        .decl.var = {
            .name = parse_ident(p),
            .type = parse_ident(p),
            .value = accept(p, token_ASSIGN) ? parse_expr(p) : NULL,
        },
    };
    expect(p, token_SEMICOLON);
    return copy(&tmp);
}

static node_t *parse_decl(parser_t *p)
{
    switch (p->tok) {
    case token_FUNC:
        return parse_func_decl(p);
    case token_TYPE:
        return parse_type_decl(p);
    case token_VAR:
        return parse_var_decl(p);
    case token_EOF:
        return NULL;
    default:
        P_PANIC(p, "unexpected token: %s", token_string(p->tok));
        break;
    }
}

static file_t *_parse_file(parser_t *p)
{
    da_t decls;
    da_init_node(&decls);
    next(p);
    while (p->tok != token_EOF) {
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
