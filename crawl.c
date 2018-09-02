#include "crawl.h"

#include <assert.h>
#include <stdio.h>

extern void crawl_node(const node_t *n)
{
    assert(n);
    switch (n->t) {

    case DECL_FUNC:
        crawl_node(n->decl.func.type);
        printf(" ");
        crawl_node(n->decl.func.name);
        printf("() ");
        crawl_node(n->decl.func.body);
        break;
    case DECL_TYPE:
        printf("typedef ");
        crawl_node(n->decl.type.type);
        printf(" ");
        crawl_node(n->decl.type.name);
        printf(";\n");
        break;
    case DECL_VAR:
        crawl_node(n->decl.var.type);
        printf(" ");
        crawl_node(n->decl.var.name);
        if (n->decl.var.value) {
            printf(" = ");
            crawl_node(n->decl.var.value);
        }
        printf(";\n");
        break;

    case EXPR_BASIC:
        printf("%s", n->expr.basic.value);
        break;
    case EXPR_IDENT:
        printf("%s", n->expr.ident.name);
        break;
    case EXPR_STRUCT:
        printf("struct {\n");
        node_t **fields = n->expr.struct_.fields;
        while (*fields)
            crawl_node(*fields++);
        printf("}");
        break;
    case EXPR_UNARY:
        printf("%c", n->expr.unary.op);
        crawl_node(n->expr.unary.expr);
        break;

    case STMT_BLOCK:
        printf("{\n");
        node_t **stmts = n->stmt.block.stmts;
        while (stmts && *stmts)
            crawl_node(*stmts++);
        printf("}\n");
        break;
    case STMT_DECL:
        crawl_node(n->stmt.decl.decl);
        break;
    case STMT_EMPTY:
        break;
    case STMT_RETURN:
        printf("return");
        if (n->stmt.return_.expr) {
            printf(" ");
            crawl_node(n->stmt.return_.expr);
        }
        printf(";\n");
        break;

    default:
        printf("??? ");
        break;
    }
}
