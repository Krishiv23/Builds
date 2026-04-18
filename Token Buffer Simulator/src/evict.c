#include "buffer.h"

/*
 * evict_select_index - choose which slot to evict based on policy.
 *
 * FIFO     : evict the token with the smallest insert_time.
 * LRU      : evict the token with the smallest last_access_time.
 * PRIORITY : evict the token with the lowest priority;
 *            ties broken by smallest insert_time (oldest).
 *
 * Returns the slot index to evict, or -1 if the buffer is empty.
 */
int evict_select_index(Buffer *buf)
{
    int victim = -1;

    for (int i = 0; i < buf->capacity; i++) {
        if (!buf->slots[i].valid)
            continue;

        if (victim < 0) {
            victim = i;
            continue;
        }

        const Token *cur  = &buf->slots[i];
        const Token *best = &buf->slots[victim];

        switch (buf->policy) {

        case POLICY_FIFO:
            if (cur->insert_time < best->insert_time)
                victim = i;
            break;

        case POLICY_LRU:
            if (cur->last_access_time < best->last_access_time)
                victim = i;
            break;

        case POLICY_PRIORITY:
            /* Lower priority value = evict first */
            if (cur->priority < best->priority) {
                victim = i;
            } else if (cur->priority == best->priority) {
                /* tie-break: older insert_time wins eviction */
                if (cur->insert_time < best->insert_time)
                    victim = i;
            }
            break;
        }
    }

    return victim;
}
