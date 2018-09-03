#include "crawl.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

extern void crawl_file(crawler_t *c, const file_t *f)
{
    for (node_t **decls = f->decls; decls && *decls; ++decls) {
        crawl_node(c, *decls);
        fprintf(c->fp, ";\n");
    }
}

extern void crawl_node(crawler_t *c, const node_t *n)
{
    assert(n);

    switch (n->t) {

    case NODE_UNDEFINED:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        crawl_node(c, n->decl.func.type);
        fprintf(c->fp, " ");
        crawl_node(c, n->decl.func.name);
        fprintf(c->fp, "(");
        for (node_t **params = n->decl.func.params; params && *params; ) {
            crawl_node(c, *params);
            if (*++params)
                fprintf(c->fp, ",");
        }
        fprintf(c->fp, ") ");
        if (n->decl.func.body)
            crawl_node(c, n->decl.func.body);
        break;

    case DECL_TYPE:
        fprintf(c->fp, "typedef ");
        crawl_node(c, n->decl.type.type);
        fprintf(c->fp, " ");
        crawl_node(c, n->decl.type.name);
        break;

    case DECL_VAR:
        crawl_node(c, n->decl.var.type);
        fprintf(c->fp, " ");
        crawl_node(c, n->decl.var.name);
        if (n->decl.var.value) {
            fprintf(c->fp, " = ");
            crawl_node(c, n->decl.var.value);
        }
        break;

    case EXPR_BASIC:
        fprintf(c->fp, "%s", n->expr.basic.value);
        break;

    case EXPR_BINARY:
        crawl_node(c, n->expr.binary.x);
        fprintf(c->fp, " %s ", token_string(n->expr.binary.op));
        crawl_node(c, n->expr.binary.y);
        break;

    case EXPR_CALL:
        crawl_node(c, n->expr.call.func);
        fprintf(c->fp, "(");
        for (node_t **args = n->expr.call.args; args && *args; ) {
            crawl_node(c, *args);
            if (*++args)
                fprintf(c->fp, ", ");
        }
        fprintf(c->fp, ")");
        break;

    case EXPR_FIELD:
        crawl_node(c, n->expr.field.type);
        fprintf(c->fp, " ");
        crawl_node(c, n->expr.field.name);
        break;

    case EXPR_IDENT:
        fprintf(c->fp, "%s", n->expr.ident.name);
        break;

    case EXPR_PAREN:
        fprintf(c->fp, "(");
        crawl_node(c, n->expr.paren.x);
        fprintf(c->fp, ")");
        break;

    case EXPR_STRUCT:
        fprintf(c->fp, "struct {\n");
        node_t **fields = n->expr.struct_.fields;
        while (*fields) {
            crawl_node(c, *fields++);
            fprintf(c->fp, ";\n");
        }
        fprintf(c->fp, "}");
        break;

    case EXPR_UNARY:
        fprintf(c->fp, "%s", token_string(n->expr.unary.op));
        crawl_node(c, n->expr.unary.expr);
        break;

    case STMT_ASSIGN:
        crawl_node(c, n->stmt.assign.lhs);
        fprintf(c->fp, "%s", token_string(n->stmt.assign.tok));
        crawl_node(c, n->stmt.assign.rhs);
        break;

    case STMT_BLOCK:
        fprintf(c->fp, "{\n");
        node_t **stmts = n->stmt.block.stmts;
        while (stmts && *stmts) {
            crawl_node(c, *stmts++);
            fprintf(c->fp, ";\n");
        }
        fprintf(c->fp, "}\n");
        break;

    case STMT_BRANCH:
        fprintf(c->fp, "%s", token_string(n->stmt.branch.tok));
        break;

    case STMT_DECL:
        crawl_node(c, n->stmt.decl.decl);
        break;

    case STMT_EMPTY:
        break;

    case STMT_EXPR:
        crawl_node(c, n->stmt.decl.decl);
        break;

    case STMT_FOR:
        fprintf(c->fp, "for (");
        if (n->stmt.for_.init)
            crawl_node(c, n->stmt.for_.init);
        fprintf(c->fp, ";");
        if (n->stmt.for_.cond)
            crawl_node(c, n->stmt.for_.cond);
        fprintf(c->fp, ";");
        if (n->stmt.for_.post)
            crawl_node(c, n->stmt.for_.post);
        fprintf(c->fp, ") ");
        crawl_node(c, n->stmt.for_.body);
        break;

    case STMT_IF:
        fprintf(c->fp, "if (");
        crawl_node(c, n->stmt.if_.cond);
        fprintf(c->fp, ") ");
        crawl_node(c, n->stmt.if_.body);
        if (n->stmt.if_.else_) {
            fprintf(c->fp, " else ");
            crawl_node(c, n->stmt.if_.else_);
        }
        break;

    case STMT_RETURN:
        fprintf(c->fp, "return");
        if (n->stmt.return_.expr) {
            fprintf(c->fp, " ");
            crawl_node(c, n->stmt.return_.expr);
        }
        break;

        /* XXX: no default clause, we want warnings for unhandled ndoe types */
    }
}
