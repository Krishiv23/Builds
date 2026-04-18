#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "token.h"

/* ------------------------------------------------------------------ */
/* Lifecycle                                                            */
/* ------------------------------------------------------------------ */

Buffer *buffer_create(int capacity, EvictPolicy policy)
{
    if (capacity <= 0) return NULL;

    Buffer *buf = malloc(sizeof(Buffer));
    if (!buf) return NULL;

    buf->slots = calloc(capacity, sizeof(Token));
    if (!buf->slots) { free(buf); return NULL; }

    buf->capacity        = capacity;
    buf->count           = 0;
    buf->policy          = policy;
    buf->clock           = 0;
    buf->total_inserts   = 0;
    buf->total_evictions = 0;
    buf->cache_hits      = 0;
    buf->cache_misses    = 0;
    return buf;
}

void buffer_destroy(Buffer *buf)
{
    if (!buf) return;
    free(buf->slots);
    free(buf);
}

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

/* Find index of token with given id; returns -1 if not found. */
int buffer_find_index(Buffer *buf, int id)
{
    for (int i = 0; i < buf->capacity; i++) {
        if (buf->slots[i].valid && buf->slots[i].id == id)
            return i;
    }
    return -1;
}

/* Find pointer to token with given id; returns NULL if not found. */
Token *buffer_find(Buffer *buf, int id)
{
    int idx = buffer_find_index(buf, id);
    return idx >= 0 ? &buf->slots[idx] : NULL;
}

/* Find index of first empty slot; returns -1 if buffer is full. */
static int find_empty_slot(Buffer *buf)
{
    for (int i = 0; i < buf->capacity; i++) {
        if (!buf->slots[i].valid)
            return i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Eviction (delegated to evict.c via evict_select_index)              */
/* ------------------------------------------------------------------ */

/* Declared in evict.c */
int evict_select_index(Buffer *buf);

int buffer_evict_one(Buffer *buf)
{
    int idx = evict_select_index(buf);
    if (idx < 0) return -1;

    int evicted_id = buf->slots[idx].id;
    buf->slots[idx].valid = 0;
    buf->count--;
    buf->total_evictions++;
    return evicted_id;
}

/* ------------------------------------------------------------------ */
/* Public operations                                                    */
/* ------------------------------------------------------------------ */

int buffer_insert(Buffer *buf, int id, int priority, const char *data)
{
    buf->clock++;

    /* Check for existing token with same id -> update in place */
    int idx = buffer_find_index(buf, id);
    if (idx >= 0) {
        Token *t = &buf->slots[idx];
        t->priority        = priority;
        t->insert_time     = buf->clock;
        t->last_access_time = buf->clock;
        strncpy(t->data, data, TOKEN_DATA_MAX - 1);
        t->data[TOKEN_DATA_MAX - 1] = '\0';
        buf->total_inserts++;
        printf("UPDATED  id=%d\n", id);
        return 0;
    }

    /* Evict if full */
    if (buf->count >= buf->capacity) {
        int evicted = buffer_evict_one(buf);
        if (evicted >= 0)
            printf("EVICTED  id=%d\n", evicted);
        else {
            fprintf(stderr, "ERROR: buffer full and eviction failed\n");
            return -1;
        }
    }

    /* Place in first empty slot */
    idx = find_empty_slot(buf);
    if (idx < 0) {
        fprintf(stderr, "ERROR: no empty slot after eviction\n");
        return -1;
    }

    Token *t = &buf->slots[idx];
    t->id              = id;
    t->priority        = priority;
    t->insert_time     = buf->clock;
    t->last_access_time = buf->clock;
    t->valid           = 1;
    strncpy(t->data, data, TOKEN_DATA_MAX - 1);
    t->data[TOKEN_DATA_MAX - 1] = '\0';

    buf->count++;
    buf->total_inserts++;
    printf("INSERTED id=%d\n", id);
    return 0;
}

int buffer_access(Buffer *buf, int id)
{
    buf->clock++;
    Token *t = buffer_find(buf, id);
    if (t) {
        t->last_access_time = buf->clock;
        buf->cache_hits++;
        printf("HIT  id=%d  data=\"%s\"\n", id, t->data);
        return 1;
    }
    buf->cache_misses++;
    printf("MISS id=%d\n", id);
    return 0;
}

int buffer_delete(Buffer *buf, int id)
{
    buf->clock++;
    int idx = buffer_find_index(buf, id);
    if (idx >= 0) {
        buf->slots[idx].valid = 0;
        buf->count--;
        printf("DELETED  id=%d\n", id);
        return 1;
    }
    printf("NOT FOUND id=%d\n", id);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Display                                                              */
/* ------------------------------------------------------------------ */

void buffer_print(const Buffer *buf)
{
    printf("--- Buffer (%d/%d) ---\n", buf->count, buf->capacity);
    printf("%-6s %-6s %-8s %-12s %-12s  %s\n",
           "ID", "PRIO", "VALID", "INSERT_CLK", "ACCESS_CLK", "DATA");
    printf("%-6s %-6s %-8s %-12s %-12s  %s\n",
           "------", "------", "--------", "------------", "------------", "----");

    int printed = 0;
    for (int i = 0; i < buf->capacity; i++) {
        const Token *t = &buf->slots[i];
        if (!t->valid) continue;
        printf("%-6d %-6d %-8s %-12llu %-12llu  %s\n",
               t->id,
               t->priority,
               "yes",
               (unsigned long long)t->insert_time,
               (unsigned long long)t->last_access_time,
               t->data);
        printed++;
    }
    if (printed == 0)
        printf("  (empty)\n");
    printf("---------------------\n");
}

void buffer_stats(const Buffer *buf)
{
    uint64_t total_accesses = buf->cache_hits + buf->cache_misses;
    double hit_rate = total_accesses > 0
        ? (double)buf->cache_hits / (double)total_accesses * 100.0
        : 0.0;

    const char *policy_name =
        buf->policy == POLICY_FIFO     ? "FIFO" :
        buf->policy == POLICY_LRU      ? "LRU"  :
                                         "PRIORITY";

    printf("--- Stats ---\n");
    printf("  Policy         : %s\n", policy_name);
    printf("  Capacity       : %d\n", buf->capacity);
    printf("  Current load   : %d\n", buf->count);
    printf("  Total inserts  : %llu\n", (unsigned long long)buf->total_inserts);
    printf("  Total evictions: %llu\n", (unsigned long long)buf->total_evictions);
    printf("  Cache hits     : %llu\n", (unsigned long long)buf->cache_hits);
    printf("  Cache misses   : %llu\n", (unsigned long long)buf->cache_misses);
    printf("  Hit rate       : %.1f%%\n", hit_rate);
    printf("-------------\n");
}
