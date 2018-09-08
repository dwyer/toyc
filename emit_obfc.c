#include "emit.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

static const char *r0 = "$eax";
static const char *r1 = "$ebx";
static const char *esp = "$esp";
static const char *ebp = "$ebp";
static const char *st = "$mem";
static const int stack_size = 8 * 1024 * 1024 / sizeof(int);

static int lookup(scope_t *s, const char *ident)
{
    int idx = 1;
    while (s) {
        for (int i = 1; i <= da_len(&s->list); ++i, ++idx) {
            if (!strcmp(da_get_s(&s->list, -i), ident))
                return idx;
        }
        s = s->outer;
    }
    return 0;
}

static char *ident_string(node_t *n)
{
    return n->expr.ident.name;
}

static void print_ident(crawler_t *c, node_t *n)
{
    fprintf(c->fp, "%s", n->expr.ident.name);
}

static void push(crawler_t *c, scope_t *s, char *var, const char *val)
{
    fprintf(c->fp, "\t%s[%s++] = %s;\n", st, esp, val);
    da_append_s(&s->list, var = var ? var : "");
}

static void pop(crawler_t *c, scope_t *s, const char *val)
{
    fprintf(c->fp, "\t%s = %s[--%s];\n", val, st, esp);
    free(da_pop_s(&s->list));
}

static void emit(crawler_t *c, const node_t *n)
{
    static const node_t *loop_node = NULL;
    static scope_t *top_scope = NULL;

    assert(n);

    switch (n->t) {

    case NODE_UNDEFINED:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        top_scope = ast_new_scope(top_scope);
        for (node_t **params = n->decl.func.params; params && *params; ++params) {
            node_t *param = *params;
            da_append_s(&top_scope->list, ident_string(param->expr.field.name));
        }
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
        if (n->decl.func.body) {
            fprintf(c->fp, " {\n");
            push(c, top_scope, NULL, ebp);
            fprintf(c->fp, "\t%s = %s;\n", ebp, esp);
            emit(c, n->decl.func.body);
            fprintf(c->fp, "\t%s = 0;\n", r0); /* return 0 if no return stmt */
            fprintf(c->fp, "$ret:\n");
            fprintf(c->fp, "\t%s = %s;\n", esp, ebp);
            pop(c, top_scope, ebp);
            fprintf(c->fp, "\treturn %s;\n", r0);
            fprintf(c->fp, "}");
        }
        top_scope = top_scope->outer;
        break;

    case DECL_TYPE:
        fprintf(c->fp, "typedef ");
        emit(c, n->decl.type.type);
        fprintf(c->fp, " ");
        emit(c, n->decl.type.name);
        break;

    case DECL_VAR:
        if (n->decl.var.value)
            emit(c, n->decl.var.value);
        push(c, top_scope, ident_string(n->decl.var.name), r0);
        break;

    case EXPR_BASIC:
        fprintf(c->fp, "\t%s = %s;\n", r0, n->expr.basic.value);
        break;

    case EXPR_BINARY:
        emit(c, n->expr.binary.x);
        push(c, top_scope, NULL, r0);
        emit(c, n->expr.binary.y);
        pop(c, top_scope, r1);
        fprintf(c->fp, "\t%s = %s %s %s;\n", r0, r1, token_string(n->expr.unary.op), r0);
        break;

    case EXPR_CALL:
        do {
            int num_args = 0;
            for (node_t **args = n->expr.call.args; args && *args; ++args, ++num_args) {
                emit(c, *args);
                push(c, top_scope, NULL, r0);
            }
            fprintf(c->fp, "\t%s(", ident_string(n->expr.call.func));
            for (int i = 0; i < num_args; ++i) {
                if (i)
                    fprintf(c->fp, ",");
                fprintf(c->fp, "%s[%s-%d]", st, esp, num_args-i);
            }
            fprintf(c->fp, ");\n");
            fprintf(c->fp, "\t%s -= %d;\n", esp, num_args);
            for (int i = 0; i < num_args; ++i)
                free(da_pop_s(&top_scope->list));
        } while (0);
        break;

    case EXPR_FIELD:
        print_ident(c, n->expr.field.type);
        fprintf(c->fp, " ");
        print_ident(c, n->expr.field.name);
        break;

    case EXPR_IDENT:
        fprintf(c->fp, "\t%s = %s[%s-%d];\n", r0, st, esp,
                lookup(top_scope, n->expr.ident.name));
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
        fprintf(c->fp, "\t%s = %s %s;\n", r0, token_string(n->expr.unary.op), r0);
        break;

    case STMT_ASSIGN:
        emit(c, n->stmt.assign.rhs);
        fprintf(c->fp, "\t%s[%s-%d] = %s;\n", st, esp,
                lookup(top_scope, ident_string(n->stmt.assign.lhs)),
                r0);
        break;

    case STMT_BLOCK:
        top_scope = ast_new_scope(top_scope);
        for (node_t **stmts = n->stmt.block.stmts; stmts && *stmts; ++stmts)
            emit(c, *stmts);
        fprintf(c->fp, "\t%s -= %d;\n", esp, da_len(&top_scope->list));
        da_deinit(&top_scope->list);
        top_scope = top_scope->outer;
        break;

    case STMT_BRANCH:
        switch (n->stmt.branch.tok) {
        case token_BREAK:
            fprintf(c->fp, "\tgoto $loop_END_%p;\n", loop_node);
            break;
        case token_CONTINUE:
            fprintf(c->fp, "\tgoto $loop_POST_%p;\n", loop_node);
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
        top_scope = ast_new_scope(top_scope);
        if (n->stmt.for_.init)
            emit(c, n->stmt.for_.init);
        fprintf(c->fp, "$loop_START_%p:\n", n);
        if (n->stmt.for_.cond) {
            emit(c, n->stmt.for_.cond);
            fprintf(c->fp, "\tif (!%s) goto $loop_END_%p;\n", r0, n);
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
        fprintf(c->fp, "$loop_END_%p:\n", n);
        fprintf(c->fp, "\t%s -= %d;\n", esp, da_len(&top_scope->list));
        da_deinit(&top_scope->list);
        top_scope = top_scope->outer;
        break;

    case STMT_IF:
        emit(c, n->stmt.if_.cond);
        fprintf(c->fp, "\tif (%s) goto $if_true_%p;\n", r0, n);
        fprintf(c->fp, "\tgoto $if_else_%p;\n", n);
        fprintf(c->fp, "$if_true_%p:\n", n);
        emit(c, n->stmt.if_.body);
        fprintf(c->fp, "\tgoto $if_end_%p;\n", n);
        fprintf(c->fp, "$if_else_%p:\n", n);
        if (n->stmt.if_.else_)
            emit(c, n->stmt.if_.else_);
        fprintf(c->fp, "$if_end_%p: /* NOOP */;\n", n);
        break;

    case STMT_RETURN:
        if (n->stmt.return_.expr)
            emit(c, n->stmt.return_.expr);
        fprintf(c->fp, "\tgoto $ret;\n");
        break;

        /* XXX: no default clause, we want warnings for unhandled ndoe types */
    }
}

extern void emit_obfc(crawler_t *c, const file_t *f)
{
    fprintf(c->fp, "static int %s, %s, %s, %s;\n", r0, r1, esp, ebp);
    fprintf(c->fp, "static int %s[%d];\n", st, stack_size);
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
