#include "emit.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

static const char *r0 = "%eax";
static const char *r1 = "%ecx";

static int lookup(scope_t *s, const char *ident)
{
    int idx = 1;
    while (s) {
        for (int i = 1; i <= da_len(&s->list); ++i, ++idx) {
            if (!strcmp(da_get_s(&s->list, -i), ident))
                return 4 * (idx - 1);
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
    if (val)
        fprintf(c->fp, "\tpush %s\n", val);
    da_append_s(&s->list, var = var ? var : "");
}

static void pop(crawler_t *c, scope_t *s, const char *val)
{
    fprintf(c->fp, "\tpop %s\n", val);
    free(da_pop_s(&s->list));
}

static void emit(crawler_t *c, const node_t *n)
{
    static const node_t *func_node = NULL;
    static const node_t *loop_node = NULL;
    static scope_t *top_scope = NULL;

    assert(n);

    switch (n->t) {

    case NODE_UNDEFINED:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        if (n->decl.func.body) {
            fprintf(c->fp, ".globl _%s\n", ident_string(n->decl.func.name));
            top_scope = ast_new_scope(NULL);
            for (node_t **params = n->decl.func.params; params && *params; ++params) {
                node_t *param = *params;
                da_append_s(&top_scope->list, ident_string(param->expr.field.name));
            }
            da_append_s(&top_scope->list, ""); // return address
            fprintf(c->fp, "_%s:\n", ident_string(n->decl.func.name));
            push(c, top_scope, NULL, "%ebp");
            fprintf(c->fp, "\tmovl %%esp, %%ebp\n");
            for (const node_t *tmp = func_node;;) {
                func_node = n;
                emit(c, n->decl.func.body);
                func_node = tmp;
                break;
            }
            fprintf(c->fp, "\tmovl $0, %%eax\n");
            fprintf(c->fp, "ret_%p:\n", n);
            fprintf(c->fp, "\tmovl %%ebp, %%esp\n");
            pop(c, top_scope, "%ebp");
            fprintf(c->fp, "\tret\n");
            da_deinit(&top_scope->list);
            top_scope = NULL;
        }
        break;

    case DECL_TYPE:
        break;

    case DECL_VAR:
        if (n->decl.var.value)
            emit(c, n->decl.var.value);
        push(c, top_scope, ident_string(n->decl.var.name), r0);
        break;

    case EXPR_BASIC:
        fprintf(c->fp, "\tmovl $%s, %%eax\n", n->expr.basic.value);
        break;

    case EXPR_BINARY:
        emit(c, n->expr.binary.y);
        push(c, top_scope, NULL, r0);
        emit(c, n->expr.binary.x);
        pop(c, top_scope, r1);
        switch (n->expr.binary.op) {
        case token_EQL:
        case token_GEQ:
        case token_GTR:
        case token_LEQ:
        case token_LSS:
        case token_NEQ:
            fprintf(c->fp, "\tcmpl %s, %s\n", r1, r0);
            fprintf(c->fp, "\tmovl $0, %%eax\n");
            break;
        default:
            break;
        }
        switch (n->expr.binary.op) {
        case token_ADD:
            fprintf(c->fp, "\taddl %s, %s\n", r1, r0);
            break;
        case token_SUB:
            fprintf(c->fp, "\tsubl %s, %s\n", r1, r0);
            break;
        case token_MUL:
            fprintf(c->fp, "\timul %s, %s\n", r1, r0);
            break;
        case token_QUO:
        case token_REM:
            fprintf(c->fp, "\tmovl $0, %%edx\n");
            fprintf(c->fp, "\tidivl %s\n", r1);
            if (n->expr.binary.op == token_REM)
                fprintf(c->fp, "\tmovl %%edx, %%eax\n");
            break;
        case token_EQL:
            fprintf(c->fp, "\tsete %%al\n");
            break;
        case token_GEQ:
            fprintf(c->fp, "\tsetge %%al\n");
            break;
        case token_GTR:
            fprintf(c->fp, "\tsetg %%al\n");
            break;
        case token_LEQ:
            fprintf(c->fp, "\tsetl %%al\n");
            break;
        case token_LSS:
            fprintf(c->fp, "\tsetl %%al\n");
            break;
        case token_NEQ:
            fprintf(c->fp, "\tsetne %%al\n");
            break;
        case token_LAND:
            fprintf(c->fp, "\tcmpl $0, %%ecx\n");
            fprintf(c->fp, "\tsetne %%cl\n");
            fprintf(c->fp, "\tcmpl $0, %%eax\n");
            fprintf(c->fp, "\tmovl $0, %%eax\n");
            fprintf(c->fp, "\tsetne %%al\n");
            fprintf(c->fp, "\tandb %%cl, %%al\n");
            break;
        case token_LOR:
            fprintf(c->fp, "\torl %s, %s\n", r1, r0);
            fprintf(c->fp, "\tmovl $0, %%eax\n");
            fprintf(c->fp, "\tsetne %%al\n");
            break;
        default:
            fprintf(c->fp, "\t# error: unknown binary op: `%s`;\n",
                    token_string(n->expr.binary.op));
            break;
        }
        break;

    case EXPR_CALL:
        do {
            scope_t *new_scope = ast_new_scope(NULL);
            for (node_t **args = n->expr.call.args; args && *args; ++args) {
                emit(c, *args);
                push(c, new_scope, NULL, r0);
            }
            scope_t *old_scope = top_scope;
            top_scope = new_scope;
            fprintf(c->fp, "\tcall _%s\n", ident_string(n->expr.call.func));
            fprintf(c->fp, "\taddl $%d, %%esp\n", 4 * da_len(&top_scope->list));
            da_deinit(&top_scope->list);
            top_scope = old_scope;

        } while (0);
        break;

    case EXPR_FIELD:
        print_ident(c, n->expr.field.type);
        fprintf(c->fp, " ");
        print_ident(c, n->expr.field.name);
        break;

    case EXPR_IDENT:
        fprintf(c->fp, "\tmovl %d(%%esp), %%eax # %s\n",
                lookup(top_scope, n->expr.ident.name),
                n->expr.ident.name);
        break;

    case EXPR_PAREN:
        emit(c, n->expr.paren.x);
        break;

    case EXPR_STRUCT:
        break;

    case EXPR_UNARY:
        emit(c, n->expr.unary.expr);
        switch (n->expr.unary.op) {
        case token_SUB:
            fprintf(c->fp, "\tneg %%eax\n");
            break;
        case token_BITWISE_NOT:
            fprintf(c->fp, "\tnot %%eax\n");
            break;
        case token_NOT:
            fprintf(c->fp, "\tcmpl $0, %%eax\n");
            fprintf(c->fp, "\tmovl $0, %%eax\n");
            fprintf(c->fp, "\tsete %%al\n");
            break;
        default:
            fprintf(c->fp, "\t# unknown op `%s` #\n", token_string(n->expr.unary.op));
            break;
        }
        break;

    case STMT_ASSIGN:
        emit(c, n->stmt.assign.rhs);
        fprintf(c->fp, "\tmovl %%eax, %d(%%esp)\n",
                lookup(top_scope, ident_string(n->stmt.assign.lhs)));
        break;

    case STMT_BLOCK:
        top_scope = ast_new_scope(top_scope);
        for (node_t **stmts = n->stmt.block.stmts; stmts && *stmts; ++stmts)
            emit(c, *stmts);
        fprintf(c->fp, "\taddl $%d, %%esp\n", 4 * da_len(&top_scope->list));
        da_deinit(&top_scope->list);
        top_scope = top_scope->outer;
        break;

    case STMT_BRANCH:
        switch (n->stmt.branch.tok) {
        case token_BREAK:
            fprintf(c->fp, "\tjmp loop_END_%p\n", loop_node);
            break;
        case token_CONTINUE:
            fprintf(c->fp, "\tjmp loop_POST_%p\n", loop_node);
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
        fprintf(c->fp, "loop_START_%p:\n", n);
        if (n->stmt.for_.cond) {
            emit(c, n->stmt.for_.cond);
            fprintf(c->fp, "\tcmpl $0, %%eax\n");
            fprintf(c->fp, "\tje loop_END_%p\n", n);
        }
        for (const node_t *tmp = loop_node;;) {
            loop_node = n;
            emit(c, n->stmt.for_.body);
            loop_node = tmp;
            break;
        }
        fprintf(c->fp, "loop_POST_%p:\n", n);
        if (n->stmt.for_.post)
            emit(c, n->stmt.for_.post);
        fprintf(c->fp, "\tjmp loop_START_%p\n", n);
        fprintf(c->fp, "loop_END_%p:\n", n);
        fprintf(c->fp, "\taddl $%d, %%esp\n", 4 * da_len(&top_scope->list));
        da_deinit(&top_scope->list);
        top_scope = top_scope->outer;
        break;

    case STMT_IF:
        emit(c, n->stmt.if_.cond);
        fprintf(c->fp, "\tcmpl $0, %%eax\n");
        fprintf(c->fp, "\tje if_else_%p\n", n);
        fprintf(c->fp, "if_true_%p:\n", n);
        emit(c, n->stmt.if_.body);
        fprintf(c->fp, "\tjmp if_end_%p\n", n);
        fprintf(c->fp, "if_else_%p:\n", n);
        if (n->stmt.if_.else_)
            emit(c, n->stmt.if_.else_);
        fprintf(c->fp, "if_end_%p:\n", n);
        break;

    case STMT_RETURN:
        if (n->stmt.return_.expr)
            emit(c, n->stmt.return_.expr);
        fprintf(c->fp, "\tjmp ret_%p\n", func_node);
        break;

        /* XXX: no default clause, we want warnings for unhandled ndoe types */
    }
}

extern void emit_x64(crawler_t *c, const file_t *f)
{
    for (node_t **decls = f->decls; decls && *decls; ++decls) {
        switch ((*decls)->t) {
        case DECL_FUNC:
            emit(c, *decls);
            break;
        default:
            PANIC("only func decls are supported at the top level");
            break;
        }
    }
}
