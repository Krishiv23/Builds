#ifndef BUFFER_H
#define BUFFER_H

#include "token.h"

typedef enum {
    POLICY_FIFO,
    POLICY_LRU,
    POLICY_PRIORITY
} EvictPolicy;

typedef struct {
    Token       *slots;         /* flat array of tokens */
    int          capacity;      /* max number of tokens */
    int          count;         /* current number of valid tokens */
    EvictPolicy  policy;

    /* logical clock */
    uint64_t     clock;

    /* stats */
    uint64_t     total_inserts;
    uint64_t     total_evictions;
    uint64_t     cache_hits;
    uint64_t     cache_misses;
} Buffer;

/* lifecycle */
Buffer *buffer_create(int capacity, EvictPolicy policy);
void    buffer_destroy(Buffer *buf);

/* operations */
int  buffer_insert(Buffer *buf, int id, int priority, const char *data);
int  buffer_access(Buffer *buf, int id);   /* returns 1=HIT, 0=MISS */
int  buffer_delete(Buffer *buf, int id);   /* returns 1=found, 0=not found */

/* helpers */
Token *buffer_find(Buffer *buf, int id);
int    buffer_find_index(Buffer *buf, int id);
int    buffer_evict_one(Buffer *buf);      /* returns evicted id, -1 on fail */

/* display */
void buffer_print(const Buffer *buf);
void buffer_stats(const Buffer *buf);

#endif /* BUFFER_H */
