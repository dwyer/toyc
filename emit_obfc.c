#include "emit.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

static const char *r0 = "$r0";
static const char *r1 = "$r1";
static const char *st = "$st";
static const char *sp = "$sp";
static const int stack_size = 8 * 1024 * 1024 / sizeof(int);

static void print_ident(crawler_t *c, node_t *id)
{
    fprintf(c->fp, "%s", id->expr.ident.name);
}

static void push(crawler_t *c, const char *lit)
{
    fprintf(c->fp, "%s[%s] = %s; %s += 1;\n", st, sp, lit, sp);
}

static void pop(crawler_t *c, const char *lit)
{
    fprintf(c->fp, "%s = %s[%s -= 1];\n", lit, st, sp);
}

static void emit(crawler_t *c, const node_t *n)
{
    static int indent = 0;
    static const node_t *loop_node = NULL;

    assert(n);

    switch (n->t) {

    case NODE_UNDEFINED:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        print_ident(c, n->decl.func.type);
        fprintf(c->fp, " ");
        print_ident(c, n->decl.func.name);
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
        emit_tabs(c, indent);
        fprintf(c->fp, "typedef ");
        emit(c, n->decl.type.type);
        fprintf(c->fp, " ");
        emit(c, n->decl.type.name);
        break;

    case DECL_VAR:
        emit_tabs(c, indent);
        print_ident(c, n->decl.var.type);
        fprintf(c->fp, " ");
        print_ident(c, n->decl.var.name);
        fprintf(c->fp, ";\n");
        if (n->decl.var.value) {
            emit(c, n->decl.var.value);
            emit_tabs(c, indent);
            print_ident(c, n->decl.var.name);
            fprintf(c->fp, " = %s;\n", r0);
        }
        break;

    case EXPR_BASIC:
        emit_tabs(c, indent);
        fprintf(c->fp, "%s = %s;\n", r0, n->expr.basic.value);
        break;

    case EXPR_BINARY:
        emit(c, n->expr.binary.x);
        emit_tabs(c, indent);
        push(c, r0);
        emit(c, n->expr.binary.y);
        emit_tabs(c, indent);
        pop(c, r1);
        emit_tabs(c, indent);
        fprintf(c->fp, "%s = %s %s %s;\n", r0, r1, token_string(n->expr.unary.op), r0);
        break;

    case EXPR_CALL:
        do {
            int num_args = 0;
            for (node_t **args = n->expr.call.args; args && *args; ++args, ++num_args) {
                emit(c, *args);
                emit_tabs(c, indent);
                push(c, r0);
            }
            emit_tabs(c, indent);
            print_ident(c, n->expr.call.func);
            fprintf(c->fp, "(");
            for (int i = 0; i < num_args; ++i) {
                if (i)
                    fprintf(c->fp, ",");
                fprintf(c->fp, "%s[%s-%d]", st, sp, num_args-i);
            }
            fprintf(c->fp, ")");
            fprintf(c->fp, ";\n");
            emit_tabs(c, indent);
            fprintf(c->fp, "%s -= %d;\n", sp, num_args);
        } while (0);
        break;

    case EXPR_FIELD:
        print_ident(c, n->expr.field.type);
        fprintf(c->fp, " ");
        print_ident(c, n->expr.field.name);
        break;

    case EXPR_IDENT:
        emit_tabs(c, indent);
        fprintf(c->fp, "%s = %s;\n", r0, n->expr.ident.name);
        break;

    case EXPR_PAREN:
        emit(c, n->expr.paren.x);
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
        emit(c, n->expr.unary.expr);
        emit_tabs(c, indent);
        fprintf(c->fp, "%s = %s %s;\n", r0, token_string(n->expr.unary.op), r0);
        break;

    case STMT_ASSIGN:
        emit(c, n->stmt.assign.rhs);
        emit_tabs(c, indent);
        print_ident(c, n->stmt.assign.lhs);
        fprintf(c->fp, " = %s;\n", r0);
        break;

    case STMT_BLOCK:
        emit_tabs(c, indent);
        fprintf(c->fp, "{\n");
        ++indent;
        for (node_t **stmts = n->stmt.block.stmts; stmts && *stmts; ++stmts)
            emit(c, *stmts);
        --indent;
        emit_tabs(c, indent);
        fprintf(c->fp, "}\n");
        break;

    case STMT_BRANCH:
        emit_tabs(c, indent);
        switch (n->stmt.branch.tok) {
        case token_BREAK:
            fprintf(c->fp, "goto $loop_END_%p;\n", loop_node);
            break;
        case token_CONTINUE:
            fprintf(c->fp, "goto $loop_POST_%p;\n", loop_node);
            break;
        default:
            break;
        }
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
        emit_tabs(c, indent);
        fprintf(c->fp, "do {\n");
        ++indent;
        if (n->stmt.for_.init)
            emit(c, n->stmt.for_.init);
        fprintf(c->fp, "$loop_START_%p:\n", n);
        if (n->stmt.for_.cond) {
            emit(c, n->stmt.for_.cond);
            emit_tabs(c, indent);
            fprintf(c->fp, "if (!%s) goto $loop_END_%p;\n", r0, n);
        }
        for (const node_t *tmp = loop_node;;) {
            loop_node = n;
            emit(c, n->stmt.for_.body);
            loop_node = tmp;
            break;
        }
        fprintf(c->fp, "\n$loop_POST_%p:\n", n);
        if (n->stmt.for_.post)
            emit(c, n->stmt.for_.post);
        fprintf(c->fp, "goto $loop_START_%p;\n", n);
        fprintf(c->fp, "$loop_END_%p: /* NOOP */;\n", n);
        --indent;
        emit_tabs(c, indent);
        fprintf(c->fp, "} while (0);");
        break;

    case STMT_IF:
        emit(c, n->stmt.if_.cond);
        emit_tabs(c, indent);
        fprintf(c->fp, "if (%s) goto $if_true_%p;\n", r0, n);
        emit_tabs(c, indent);
        fprintf(c->fp, "goto $if_else_%p;\n", n);
        fprintf(c->fp, "$if_true_%p:\n", n);
        emit(c, n->stmt.if_.body);
        emit_tabs(c, indent);
        fprintf(c->fp, "goto $if_end_%p;\n", n);
        fprintf(c->fp, "$if_else_%p:\n", n);
        if (n->stmt.if_.else_)
            emit(c, n->stmt.if_.else_);
        fprintf(c->fp, "$if_end_%p: /* NOOP */;\n", n);
        break;

    case STMT_RETURN:
        emit(c, n->stmt.return_.expr);
        emit_tabs(c, indent);
        fprintf(c->fp, "return");
        if (n->stmt.return_.expr) {
            fprintf(c->fp, " %s", r0);
        }
        fprintf(c->fp, ";\n");
        break;

        /* XXX: no default clause, we want warnings for unhandled ndoe types */
    }
}

extern void emit_obfc(crawler_t *c, const file_t *f)
{
    fprintf(c->fp, "static int %s;\n", r0);
    fprintf(c->fp, "static int %s;\n", r1);
    fprintf(c->fp, "static int %s[%d];\n", st, stack_size);
    fprintf(c->fp, "static int %s;\n", sp);
    for (node_t **decls = f->decls; decls && *decls; ++decls) {
        switch ((*decls)->t) {
        case DECL_FUNC:
            emit(c, *decls);
            fprintf(c->fp, ";\n");
            break;
        default:
            PANIC("only func decls are supported at the top level");
            break;
        }
    }
}
