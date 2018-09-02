#pragma once

#include "ast.h"

void crawl_decl(node_t *decl);
void crawl_expr(node_t *expr);
void crawl_stmt(node_t *stmt);
