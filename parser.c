#include "ast.h"
#include "log.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

#include <stdlib.h> // alloc
#include <stdio.h> // BUFSIZ
#include <string.h> // memcpy

#include <da.h>

#define memdup(p, size) ({void *vp = malloc((size)); memcpy(vp, (p), (size)); vp;})
#define copy(p) memdup((p), sizeof(*(p)))

typedef struct {
    const char *filename;
    scanner_t scanner;
    token_t tok;
    char lit[BUFSIZ];
    int pos;
} parser_t;

static node_t *parse_decl(parser_t *p);
static node_t *parse_expr(parser_t *p);
static node_t *parse_stmt(parser_t *p);
static node_t *parse_type_expr(parser_t *p);

DA_DEF_HELPERS(node, node_t *);

static void next(parser_t *p)
{
    p->pos = p->scanner.offset;
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

static void error_expected(parser_t *p, int pos, const char *msg)
{
    int line = 1;
    int column = 1;
    for (int i = 0; i < pos-1; ++i) {
        if (p->scanner.src[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
    PANIC("%s:%d:%d: expected %s, got %s", p->filename, line, column, msg,
            token_string(p->tok));
}

static int expect(parser_t *p, token_t tok)
{
    int pos = p->pos;
    if (p->tok != tok) {
        error_expected(p, pos, token_string(tok));
    }
    next(p);
    return pos;
}

static node_t *parse_ident(parser_t *p)
{
    node_t tmp = {.t = EXPR_IDENT, .pos = p->pos};
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
                .pos = p->pos,
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
            int pos = expect(p, token_LPAREN);
            node_t *x = parse_expr(p);
            expect(p, token_RPAREN);
            node_t tmp = {
                .t = EXPR_PAREN,
                .pos = pos,
                .expr.paren.x = x,
            };
            return copy(&tmp);
        } while (0);
        break;
    default:
        error_expected(p, p->pos, "expression");
        break;
    }
    return NULL;
}

static node_t *parse_call(parser_t *p, node_t *func)
{
    int pos = expect(p, token_LPAREN);
    da_t args;
    da_init_node(&args);
    while (p->tok != token_RPAREN) {
        da_append_node(&args, parse_expr(p));
        if (!accept(p, token_COMMA))
            break;
    }
    if (da_len(&args))
        da_append_node(&args, NULL);
    expect(p, token_RPAREN);
    node_t tmp = {
        .t = EXPR_CALL,
        .pos = pos,
        .expr.call = {
            .func = func,
            .args = args.data,
        },
    };
    return copy(&tmp);
}

static node_t *parse_primary_expr(parser_t *p)
{
    node_t *x = parse_operand(p);
    if (p->tok == token_LPAREN)
        x = parse_call(p, x);
    return x;
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
                .pos = p->pos,
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
    case token_REM:
    case token_SUB:
        do {
            int op = p->tok;
            int pos = expect(p, op);
            node_t *y = parse_binary_expr(p);
            node_t tmp = {
                .t = EXPR_BINARY,
                .pos = pos,
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
    int pos = expect(p, token_RETURN);
    node_t tmp = {
        .t = STMT_RETURN,
        .pos = pos,
        .stmt.return_.expr = p->tok == token_SEMICOLON ? NULL : parse_expr(p),
    };
    expect(p, token_SEMICOLON);
    return copy(&tmp);
}

static node_t *parse_simple_stmt(parser_t *p)
{
    node_t *lhs = parse_expr(p);
    if (accept(p, token_ASSIGN)) {
        node_t *rhs = parse_expr(p);
        node_t tmp = {
            .t = STMT_ASSIGN,
            .pos = lhs->pos,
            .stmt.assign = {
                .lhs = lhs,
                .tok = token_ASSIGN,
                .rhs = rhs,
            },
        };
        return copy(&tmp);
    }
    node_t tmp = {
        .t = STMT_EXPR,
        .pos = lhs->pos,
        .stmt.expr = {
            .x = lhs,
        },
    };
    return copy(&tmp);
}

static node_t *parse_block_stmt(parser_t *p)
{
    int pos = expect(p, token_LBRACE);
    da_t stmts;
    da_init_node(&stmts);
    while (p->tok != token_RBRACE) {
        da_append_node(&stmts, parse_stmt(p));
    }
    da_append_node(&stmts, NULL);
    expect(p, token_RBRACE);
    node_t tmp = {
        .t=STMT_BLOCK,
        .pos = pos,
        .stmt.block.stmts = stmts.data,
    };
    return copy(&tmp);
}

static node_t *parse_if_stmt(parser_t *p)
{
    int pos = expect(p, token_IF);
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
        .pos = pos,
        .stmt.if_ = {
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return copy(&tmp);
}

static node_t *parse_branch_stmt(parser_t *p, token_t tok)
{
    int pos = expect(p, tok);
    node_t tmp = {
        .t = STMT_BRANCH,
        .pos = pos,
        .stmt.branch.tok = tok,
    };
    expect(p, token_SEMICOLON);
    return copy(&tmp);
}

static node_t *parse_for_stmt(parser_t *p)
{
    int pos = expect(p, token_FOR);
    node_t *init = NULL;
    if (p->tok != token_SEMICOLON)
        init = parse_stmt(p);
    else
        expect(p, token_SEMICOLON);
    node_t *cond = NULL;
    if (p->tok != token_SEMICOLON)
        cond = parse_expr(p);
    expect(p, token_SEMICOLON);
    node_t *post = NULL;
    if (p->tok != token_LBRACE)
        post = parse_simple_stmt(p);
    node_t *body = parse_block_stmt(p);
    node_t tmp = {
        .t = STMT_FOR,
        .pos = pos,
        .stmt.for_ = {
            .init = init,
            .cond = cond,
            .post = post,
            .body = body,
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
            node_t *decl = parse_decl(p);
            node_t tmp = {
                .t = STMT_DECL,
                .pos = decl->pos,
                .stmt.decl.decl = decl,
            };
            return copy(&tmp);
        } while (0);
    case token_IDENT:
        do {
            node_t *s = parse_simple_stmt(p);
            expect(p, token_SEMICOLON);
            return s;
        } while (0);
    case token_RETURN:
        return parse_return_stmt(p);
    case token_BREAK:
    case token_CONTINUE:
        return parse_branch_stmt(p, p->tok);
    case token_LBRACE:
        return parse_block_stmt(p);
    case token_IF:
        return parse_if_stmt(p);
    case token_FOR:
        return parse_for_stmt(p);
    case token_SEMICOLON:
        do {
            node_t tmp = { .t = STMT_EMPTY, .pos = p->pos, };
            next(p);
            return copy(&tmp);
        } while (0);
    default:
        error_expected(p, p->pos, "statement");
        return NULL;
    }
}

static node_t **parse_parameter_list(parser_t *p)
{
    da_t fields;
    da_init_node(&fields);
    for (;;) {
        node_t tmp = {
            .t = EXPR_FIELD,
            .pos = p->pos,
            .expr.field = {
                .name = parse_ident(p),
                .type = parse_type_expr(p),
            },
        };
        da_append_node(&fields, copy(&tmp));
        if (!accept(p, token_COMMA))
            break;
    }
    if (da_len(&fields))
        da_append_node(&fields, NULL);
    return fields.data;
}

static node_t **parse_params(parser_t *p)
{
    node_t **params = NULL;
    expect(p, token_LPAREN);
    if (p->tok != token_RPAREN)
        params = parse_parameter_list(p);
    expect(p, token_RPAREN);
    return params;
}

static node_t *parse_func_decl(parser_t *p)
{
    int pos = expect(p, token_FUNC);
    node_t *recv = NULL;
    node_t *ident = parse_ident(p);
    if (accept(p, token_PERIOD)) {
        recv = ident;
        ident = parse_ident(p);
    }
    node_t **params = parse_params(p);
    node_t *type = parse_ident(p);
    node_t *body = NULL;
    if (p->tok == token_LBRACE)
        body = parse_block_stmt(p);
    else
        expect(p, token_SEMICOLON);
    node_t tmp = {
        .t = DECL_FUNC,
        .pos = pos,
        .decl.func = {
            .recv = recv,
            .name = ident,
            .params = params,
            .type = type,
            .body = body,
        },
    };
    return copy(&tmp);
}

static node_t *parse_struct_type(parser_t *p)
{
    int pos = expect(p, token_STRUCT);
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
        .pos = pos,
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
        error_expected(p, p->pos, "type expression");
        return NULL;
    }
}

static node_t *parse_type_decl(parser_t *p)
{
    int pos = expect(p, token_TYPE);
    node_t tmp = {
        .t = DECL_TYPE,
        .pos = pos,
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
    int pos = expect(p, token_VAR);
    node_t tmp = {
        .t = DECL_VAR,
        .pos = pos,
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
    default:
        error_expected(p, p->pos, "declaration");
        return NULL;
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
