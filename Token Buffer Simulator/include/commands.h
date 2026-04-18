#ifndef COMMANDS_H
#define COMMANDS_H

#include "buffer.h"

/* Parse and execute one line of input.
   Returns 0 to continue, 1 to quit. */
int command_dispatch(Buffer *buf, const char *line);

/* Parse the policy string from argv. */
EvictPolicy parse_policy(const char *s);

#endif /* COMMANDS_H */
