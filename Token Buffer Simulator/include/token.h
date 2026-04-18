#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

#define TOKEN_DATA_MAX 256

typedef struct {
    int      id;
    char     data[TOKEN_DATA_MAX];
    int      priority;
    uint64_t insert_time;       /* logical clock at insertion */
    uint64_t last_access_time;  /* logical clock at last access */
    int      valid;             /* 1 = slot occupied, 0 = empty */
} Token;

#endif /* TOKEN_H */
