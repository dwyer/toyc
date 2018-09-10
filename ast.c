#include "ast.h"

extern scope_t *ast_new_scope(scope_t *outer)
{
    scope_t s = {.outer=outer};
    da_init_s(&s.list);
    return copy(&s);
}

extern int scope_lookup(scope_t *s, const char *ident)
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
