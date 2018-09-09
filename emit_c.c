#include "emit.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

extern void emit_tabs(crawler_t *c, int n)
{
    for (int i = 0; i < n; ++i)
        fputc('\t', c->fp);
}

static void emit(crawler_t *c, const node_t *n)
{
    static int indent = 0;

    assert(n);

    switch (n->t) {

    case NODE_UNDEFINED:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        emit(c, n->decl.func.type);
        fprintf(c->fp, " ");
        emit(c, n->decl.func.name);
        fprintf(c->fp, "(");
        for (node_t **params = n->decl.func.params; params && *params; ) {
            emit(c, *params);
            if (*++params)
                fprintf(c->fp, ",");
        }
        fprintf(c->fp, ") ");
        if (n->decl.func.body)
            emit(c, n->decl.func.body);
        break;

    case DECL_TYPE:
        fprintf(c->fp, "typedef ");
        emit(c, n->decl.type.type);
        fprintf(c->fp, " ");
        emit(c, n->decl.type.name);
        break;

    case DECL_VAR:
        emit(c, n->decl.var.type);
        fprintf(c->fp, " ");
        emit(c, n->decl.var.name);
        if (n->decl.var.value) {
            fprintf(c->fp, " = ");
            emit(c, n->decl.var.value);
        }
        break;

    case EXPR_BASIC:
        fprintf(c->fp, "%s", n->expr.basic.value);
        break;

    case EXPR_BINARY:
        fprintf(c->fp, "(");
        emit(c, n->expr.binary.x);
        fprintf(c->fp, " %s ", token_string(n->expr.binary.op));
        emit(c, n->expr.binary.y);
        fprintf(c->fp, ")");
        break;

    case EXPR_CALL:
        emit(c, n->expr.call.func);
        fprintf(c->fp, "(");
        for (node_t **args = n->expr.call.args; args && *args; ) {
            emit(c, *args);
            if (*++args)
                fprintf(c->fp, ", ");
        }
        fprintf(c->fp, ")");
        break;

    case EXPR_FIELD:
        emit(c, n->expr.field.type);
        fprintf(c->fp, " ");
        emit(c, n->expr.field.name);
        break;

    case EXPR_IDENT:
        fprintf(c->fp, "%s", n->expr.ident.name);
        break;

    case EXPR_PAREN:
        fprintf(c->fp, "(");
        emit(c, n->expr.paren.x);
        fprintf(c->fp, ")");
        break;

    case EXPR_STRUCT:
        fprintf(c->fp, "struct {\n");
        node_t **fields = n->expr.struct_.fields;
        while (*fields) {
            emit(c, *fields++);
            fprintf(c->fp, ";\n");
        }
        fprintf(c->fp, "}");
        break;

    case EXPR_UNARY:
        fprintf(c->fp, "%s", token_string(n->expr.unary.op));
        emit(c, n->expr.unary.expr);
        break;

    case STMT_ASSIGN:
        emit(c, n->stmt.assign.lhs);
        fprintf(c->fp, "%s", token_string(n->stmt.assign.tok));
        emit(c, n->stmt.assign.rhs);
        break;

    case STMT_BLOCK:
        fprintf(c->fp, "{\n");
        ++indent;
        for (node_t **stmts = n->stmt.block.stmts; stmts && *stmts; ++stmts) {
            emit_tabs(c, indent);
            emit(c, *stmts);
            fprintf(c->fp, ";\n");
        }
        --indent;
        emit_tabs(c, indent);
        fprintf(c->fp, "}");
        break;

    case STMT_BRANCH:
        fprintf(c->fp, "%s", token_string(n->stmt.branch.tok));
        break;

    case STMT_DECL:
        emit(c, n->stmt.decl.decl);
        break;

    case STMT_EMPTY:
        break;

    case STMT_EXPR:
        emit(c, n->stmt.decl.decl);
        break;

    case STMT_FOR:
        fprintf(c->fp, "for (");
        if (n->stmt.for_.init)
            emit(c, n->stmt.for_.init);
        fprintf(c->fp, ";");
        if (n->stmt.for_.cond)
            emit(c, n->stmt.for_.cond);
        fprintf(c->fp, ";");
        if (n->stmt.for_.post)
            emit(c, n->stmt.for_.post);
        fprintf(c->fp, ") ");
        emit(c, n->stmt.for_.body);
        break;

    case STMT_IF:
        fprintf(c->fp, "if (");
        emit(c, n->stmt.if_.cond);
        fprintf(c->fp, ") ");
        emit(c, n->stmt.if_.body);
        if (n->stmt.if_.else_) {
            fprintf(c->fp, " else ");
            emit(c, n->stmt.if_.else_);
        }
        break;

    case STMT_RETURN:
        fprintf(c->fp, "return");
        if (n->stmt.return_.expr) {
            fprintf(c->fp, " ");
            emit(c, n->stmt.return_.expr);
        }
        break;

        /* XXX: no default clause, we want warnings for unhandled ndoe types */
    }
}

extern void emit_c(crawler_t *c, const file_t *f)
{
    for (node_t **decls = f->decls; decls && *decls; ++decls) {
        emit(c, *decls);
        fprintf(c->fp, ";\n");
    }
}
