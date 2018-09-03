#pragma once

#include "ast.h"

typedef struct {
    void *fp;
    int indent;
} crawler_t;

extern void crawl_file(crawler_t *c, const file_t *f);
extern void crawl_node(crawler_t *c, const node_t *n);
