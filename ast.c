#include "ast.h"

extern scope_t *ast_new_scope(scope_t *outer)
{
    scope_t s = {.outer=outer};
    return copy(&s);
}
