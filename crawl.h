#pragma once

#include "ast.h"

void crawl_decl(decl_t *decl);
void crawl_expr(expr_t *expr);
void crawl_stmt(stmt_t *stmt);
