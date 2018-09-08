#include "ast.h"

extern scope_t *ast_new_scope(scope_t *outer)
{
    scope_t s = {.outer=outer};
    da_init_s(&s.list);
    return copy(&s);
}
