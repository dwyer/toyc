#include "crawl.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

extern void crawl_file(const file_t *f)
{
    for (node_t **decls = f->decls; decls && *decls; ++decls) {
        crawl_node(*decls);
        printf(";\n");
    }
}

extern void crawl_node(const node_t *n)
{
    assert(n);
    switch (n->t) {

    case NODE_UNDEFINED:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        crawl_node(n->decl.func.type);
        printf(" ");
        crawl_node(n->decl.func.name);
        printf("(");
        for (node_t **params = n->decl.func.params; params && *params; ) {
            crawl_node(*params);
            if (*++params)
                printf(",");
        }
        printf(") ");
        if (n->decl.func.body)
            crawl_node(n->decl.func.body);
        break;

    case DECL_TYPE:
        printf("typedef ");
        crawl_node(n->decl.type.type);
        printf(" ");
        crawl_node(n->decl.type.name);
        break;

    case DECL_VAR:
        crawl_node(n->decl.var.type);
        printf(" ");
        crawl_node(n->decl.var.name);
        if (n->decl.var.value) {
            printf(" = ");
            crawl_node(n->decl.var.value);
        }
        break;

    case EXPR_BASIC:
        printf("%s", n->expr.basic.value);
        break;

    case EXPR_BINARY:
        crawl_node(n->expr.binary.x);
        printf(" %s ", token_string(n->expr.binary.op));
        crawl_node(n->expr.binary.y);
        break;

    case EXPR_CALL:
        crawl_node(n->expr.call.func);
        printf("(");
        for (node_t **args = n->expr.call.args; args && *args; ) {
            crawl_node(*args);
            if (*++args)
                printf(", ");
        }
        printf(")");
        break;

    case EXPR_FIELD:
        crawl_node(n->expr.field.type);
        printf(" ");
        crawl_node(n->expr.field.name);
        break;

    case EXPR_IDENT:
        printf("%s", n->expr.ident.name);
        break;

    case EXPR_PAREN:
        printf("(");
        crawl_node(n->expr.paren.x);
        printf(")");
        break;

    case EXPR_STRUCT:
        printf("struct {\n");
        node_t **fields = n->expr.struct_.fields;
        while (*fields) {
            crawl_node(*fields++);
            printf(";\n");
        }
        printf("}");
        break;

    case EXPR_UNARY:
        printf("%s", token_string(n->expr.unary.op));
        crawl_node(n->expr.unary.expr);
        break;

    case STMT_ASSIGN:
        crawl_node(n->stmt.assign.lhs);
        printf("%s", token_string(n->stmt.assign.tok));
        crawl_node(n->stmt.assign.rhs);
        break;

    case STMT_BLOCK:
        printf("{\n");
        node_t **stmts = n->stmt.block.stmts;
        while (stmts && *stmts) {
            crawl_node(*stmts++);
            printf(";\n");
        }
        printf("}\n");
        break;

    case STMT_BRANCH:
        printf("%s", token_string(n->stmt.branch.tok));
        break;

    case STMT_DECL:
        crawl_node(n->stmt.decl.decl);
        break;

    case STMT_EMPTY:
        break;

    case STMT_EXPR:
        crawl_node(n->stmt.decl.decl);
        break;

    case STMT_FOR:
        printf("for (");
        if (n->stmt.for_.init)
            crawl_node(n->stmt.for_.init);
        printf(";");
        if (n->stmt.for_.cond)
            crawl_node(n->stmt.for_.cond);
        printf(";");
        if (n->stmt.for_.post)
            crawl_node(n->stmt.for_.post);
        printf(") ");
        crawl_node(n->stmt.for_.body);
        break;

    case STMT_IF:
        printf("if (");
        crawl_node(n->stmt.if_.cond);
        printf(") ");
        crawl_node(n->stmt.if_.body);
        if (n->stmt.if_.else_) {
            printf(" else ");
            crawl_node(n->stmt.if_.else_);
        }
        break;

    case STMT_RETURN:
        printf("return");
        if (n->stmt.return_.expr) {
            printf(" ");
            crawl_node(n->stmt.return_.expr);
        }
        break;

        /* XXX: no default clause, we want warnings for unhandled ndoe types */
    }
}
