#include "emit.h"
#include "token.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

#ifdef __APPLE__
static const char *pre = "_";
#else
static const char *pre = "";
#endif

static const char *eax = "%eax";
static const char *ecx = "%ecx";


static int scope_len(const scope_t *s)
{
    int n = 1;
    while (s) {
        n += da_len(&s->list);
        s = s->outer;
    }
    return n;
}

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

static char *simplify(const node_t *n, scope_t *s) {
    char *ret = NULL;
    switch (n->t) {
    case EXPR_BASIC:
        asprintf(&ret, "$%s", n->expr.basic.value);
        break;
    case EXPR_IDENT:
        asprintf(&ret, "%d(%%esp)", lookup(s, n->expr.ident.name));
        break;
    default:
        break;
    }
    return ret;
}

static char *ident_string(node_t *n)
{
    return n->expr.ident.name;
}

static void push(crawler_t *c, scope_t *s, const char *val, char *var)
{
    fprintf(c->fp, "\tpushl %s\n", val);
    da_append_s(&s->list, var = var ? var : "");
}

static void pop(crawler_t *c, scope_t *s, const char *val)
{
    fprintf(c->fp, "\tpopl %s\n", val);
    free(da_pop_s(&s->list));
}

static void emit(crawler_t *c, const node_t *n)
{
    static const node_t *func_node = NULL;
    static const node_t *loop_node = NULL;
    static scope_t *top_scope = NULL;
    static int num_rets = 0;

    assert(n);

    switch (n->t) {

    case NODE_UNDEFINED:
    case DECL_TYPE:
    case EXPR_FIELD:
        PANIC("illegal node, probably uninitialized");
        break;

    case DECL_FUNC:
        if (n->decl.func.body) {
            fprintf(c->fp, ".globl %s%s\n", pre, ident_string(n->decl.func.name));
            top_scope = ast_new_scope(NULL);
            for (node_t **params = n->decl.func.params; params && *params; ++params) {
                node_t *param = *params;
                da_append_s(&top_scope->list, ident_string(param->expr.field.name));
            }
            da_append_s(&top_scope->list, ""); // return address
            fprintf(c->fp, "%s%s:\n", pre, ident_string(n->decl.func.name));
            push(c, top_scope, "%ebp", NULL);
            fprintf(c->fp, "\tmovl %%esp, %%ebp\n");
            num_rets = 0;
            for (const node_t *tmp = func_node;;) {
                func_node = n;
                emit(c, n->decl.func.body);
                func_node = tmp;
                break;
            }
            if (!num_rets)
                fprintf(c->fp, "\tmovl $0, %%eax\n");
            fprintf(c->fp, "ret_%p:\n", n);
            fprintf(c->fp, "\tmovl %%ebp, %%esp\n");
            pop(c, top_scope, "%ebp");
            fprintf(c->fp, "\tret\n");
            da_deinit(&top_scope->list);
            top_scope = NULL;
        }
        break;

    case DECL_VAR:
        if (n->decl.var.value) {
            char *lit = simplify(n->decl.var.value, top_scope);
            if (lit) {
                push(c, top_scope, lit, ident_string(n->decl.var.name));
                free(lit);
            } else {
                emit(c, n->decl.var.value);
                push(c, top_scope, eax, ident_string(n->decl.var.name));
            }
        } else {
            da_append_s(&top_scope->list, ident_string(n->decl.var.name));
            fprintf(c->fp, "\tsubl $4, %%esp\n");
        }
        break;

    case EXPR_BASIC:
        fprintf(c->fp, "\tmovl $%s, %%eax\n", n->expr.basic.value);
        break;

    case EXPR_BINARY:
        do {
            char *rhs = simplify(n->expr.binary.y, top_scope);
            if (rhs) {
                emit(c, n->expr.binary.x);
            } else {
                asprintf(&rhs, "%s", ecx);
                emit(c, n->expr.binary.y);
                push(c, top_scope, eax, NULL);
                emit(c, n->expr.binary.x);
                pop(c, top_scope, ecx);
            }
            switch (n->expr.binary.op) {
            case token_EQL:
            case token_GEQ:
            case token_GTR:
            case token_LEQ:
            case token_LSS:
            case token_NEQ:
                fprintf(c->fp, "\tcmpl %s, %s\n", rhs, eax);
                fprintf(c->fp, "\tmovl $0, %%eax\n");
                break;
            default:
                break;
            }
            switch (n->expr.binary.op) {
            case token_ADD:
                fprintf(c->fp, "\taddl %s, %s\n", rhs, eax);
                break;
            case token_SUB:
                fprintf(c->fp, "\tsubl %s, %s\n", rhs, eax);
                break;
            case token_MUL:
                fprintf(c->fp, "\timul %s, %s\n", rhs, eax);
                break;
            case token_QUO:
            case token_REM:
                fprintf(c->fp, "\tmovl %s, %s\n", rhs, ecx);
                fprintf(c->fp, "\tmovl $0, %%edx\n");
                fprintf(c->fp, "\tidivl %s\n", ecx);
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
                fprintf(c->fp, "\tsetle %%al\n");
                break;
            case token_LSS:
                fprintf(c->fp, "\tsetl %%al\n");
                break;
            case token_NEQ:
                fprintf(c->fp, "\tsetne %%al\n");
                break;
            case token_LAND:
                fprintf(c->fp, "\tmovl %s, %s\n", rhs, ecx);
                fprintf(c->fp, "\tcmpl $0, %s\n", ecx);
                fprintf(c->fp, "\tsetne %%cl\n");
                fprintf(c->fp, "\tcmpl $0, %s\n", eax);
                fprintf(c->fp, "\tmovl $0, %%eax\n");
                fprintf(c->fp, "\tsetne %%al\n");
                fprintf(c->fp, "\tandb %%cl, %%al\n");
                break;
            case token_LOR:
                fprintf(c->fp, "\torl %s, %s\n", rhs, eax);
                fprintf(c->fp, "\tmovl $0, %%eax\n");
                fprintf(c->fp, "\tsetne %%al\n");
                break;
            default:
                fprintf(c->fp, "\t# error: unknown binary op: `%s`;\n",
                        token_string(n->expr.binary.op));
                break;
            }
            free(rhs);
        } while (0);
        break;

    case EXPR_CALL:
        do {
            top_scope = ast_new_scope(top_scope);
#ifdef __APPLE__
            int num_args = 0;
            for (node_t **args = n->expr.call.args; args && *args; ++args)
                num_args += 1;
            int len = scope_len(top_scope) + num_args + 1;
            int pad = len % 4;
            if (pad) {
                fprintf(c->fp, "\tsubl $%d, %%esp # pad\n", 4 * pad);
                for (int i = 0; i < pad; ++i)
                    da_append_s(&top_scope->list, "");
            }
#endif
            for (node_t **args = n->expr.call.args; args && *args; ++args) {
                char *lit = simplify(*args, top_scope);
                if (lit) {
                    push(c, top_scope, lit, NULL);
                    free(lit);
                } else {
                    emit(c, *args);
                    push(c, top_scope, eax, NULL);
                }
            }
            fprintf(c->fp, "\tcall %s%s\n", pre,
                    ident_string(n->expr.call.func));
            if (da_len(&top_scope->list))
                fprintf(c->fp, "\taddl $%d, %%esp\n",
                        4 * da_len(&top_scope->list));
            da_deinit(&top_scope->list);
            top_scope = top_scope->outer;
        } while (0);
        break;

    case EXPR_IDENT:
        fprintf(c->fp, "\tmovl %d(%%esp), %%eax\n",
                lookup(top_scope, n->expr.ident.name));
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
            fprintf(c->fp, "\t# unknown op `%s` #\n",
                    token_string(n->expr.unary.op));
            break;
        }
        break;

    case STMT_ASSIGN:
        if (n->stmt.assign.rhs->t == EXPR_BASIC) {
            char *rhs = NULL;
            rhs = simplify(n->stmt.assign.rhs, top_scope);
            fprintf(c->fp, "\tmovl %s, %d(%%esp)\n", rhs,
                    lookup(top_scope, ident_string(n->stmt.assign.lhs)));
            free(rhs);
        } else {
            emit(c, n->stmt.assign.rhs);
            fprintf(c->fp, "\tmovl %%eax, %d(%%esp)\n",
                    lookup(top_scope, ident_string(n->stmt.assign.lhs)));
        }
        break;

    case STMT_BLOCK:
        top_scope = ast_new_scope(top_scope);
        for (node_t **stmts = n->stmt.block.stmts; stmts && *stmts; ++stmts)
            emit(c, *stmts);
        if (da_len(&top_scope->list))
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
        if (n->stmt.if_.else_)
            fprintf(c->fp, "\tje if_else_%p\n", n);
        else
            fprintf(c->fp, "\tje if_end_%p\n", n);
        emit(c, n->stmt.if_.body);
        if (n->stmt.if_.else_) {
            fprintf(c->fp, "\tjmp if_end_%p\n", n);
            fprintf(c->fp, "if_else_%p:\n", n);
            emit(c, n->stmt.if_.else_);
        }
        fprintf(c->fp, "if_end_%p:\n", n);
        break;

    case STMT_RETURN:
        if (n->stmt.return_.expr)
            emit(c, n->stmt.return_.expr);
        fprintf(c->fp, "\tjmp ret_%p\n", func_node);
        num_rets++;
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
