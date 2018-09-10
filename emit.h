#pragma once

#include "ast.h"

typedef struct {
    void *fp;
} crawler_t;

extern void emit_c(crawler_t *c, const file_t *f);
extern void emit_tabs(crawler_t *c, int n);
extern void emit_x64(crawler_t *c, const file_t *f);
