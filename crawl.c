#include "crawl.h"

#include <assert.h>
#include <stdio.h>

void crawl_expr(expr_t *expr)
{
    assert(expr);
    switch (expr->t) {
    case EXPR_BASIC:
        printf("%s", expr->expr.basic.value);
        break;
    case EXPR_IDENT:
        printf("%s", expr->expr.ident.name);
        break;
    case EXPR_STRUCT:
        printf("struct {\n");
        decl_t **fields = expr->expr.struct_.fields;
        while (*fields)
            crawl_decl(*fields++);
        printf("}");
        break;
    case EXPR_UNARY:
        printf("%c", expr->expr.unary.op);
        crawl_expr(expr->expr.unary.expr);
        break;
    default:
        printf("??? ");
        break;
    }
}

void crawl_stmt(stmt_t *stmt)
{
    assert(stmt);
    switch (stmt->t) {
    case STMT_BLOCK:
        printf("{\n");
        stmt_t **stmts = stmt->stmt.block.stmts;
        while (stmts && *stmts) 
            crawl_stmt(*stmts++);
        printf("}\n");
        break;
    case STMT_DECL:
        crawl_decl(stmt->stmt.decl.decl);
        break;
    case STMT_EMPTY:
        break;
    case STMT_RETURN:
        printf("return");
        if (stmt->stmt.return_.expr) {
            printf(" ");
            crawl_expr(stmt->stmt.return_.expr);
        }
        printf(";\n");
        break;
    default:
        printf("??? ");
        break;
    }
}

void crawl_decl(decl_t *decl)
{
    assert(decl);
    switch (decl->t) {
    case DECL_FUNC:
        printf("func ");
        crawl_expr(decl->decl.func.name);
        printf("() ");
        crawl_expr(decl->decl.func.type);
        crawl_stmt(decl->decl.func.body);
        break;
    case DECL_TYPE:
        printf("type ");
        crawl_expr(decl->decl.type.name);
        printf(" ");
        crawl_expr(decl->decl.type.type);
        printf(";\n");
        break;
    case DECL_VAR:
        printf("var ");
        crawl_expr(decl->decl.var.name);
        printf(" ");
        crawl_expr(decl->decl.var.type);
        if (decl->decl.var.value) {
            printf(" = ");
            crawl_expr(decl->decl.var.value);
        }
        printf(";\n");
        break;
    default:
        printf("??? ");
        break;
    }
}
