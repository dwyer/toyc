void *append(void *dst, int *capp, int *lenp, const void *src, int size)
{
    if (dst == NULL) {
        *capp = 100;
        *lenp = 0;
        dst = malloc(*capp * size);
    } else if (*capp <= *lenp) {
        *capp = *capp * 2;
        dst = realloc(dst, *capp * size);
    }
    memcpy((char *)dst + *lenp * size, &src, size);
    ++*lenp;
    return dst;
}
