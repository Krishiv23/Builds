#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* isatty, fileno */

#include "buffer.h"
#include "commands.h"

#define LINE_MAX_LEN 512

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s <capacity> <policy>\n"
        "\n"
        "  capacity  : positive integer (max tokens in buffer)\n"
        "  policy    : FIFO | LRU | PRIORITY\n"
        "\n"
        "Commands are read from stdin. Redirect a file for batch mode:\n"
        "  %s 4 LRU < tests/test_lru.txt\n",
        prog, prog);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int capacity = atoi(argv[1]);
    if (capacity <= 0) {
        fprintf(stderr, "ERROR: capacity must be a positive integer\n");
        return EXIT_FAILURE;
    }

    EvictPolicy policy = parse_policy(argv[2]);

    Buffer *buf = buffer_create(capacity, policy);
    if (!buf) {
        fprintf(stderr, "ERROR: failed to allocate buffer\n");
        return EXIT_FAILURE;
    }

    printf("token_buffer  capacity=%d  policy=%s\n",
           capacity,
           policy == POLICY_FIFO ? "FIFO" :
           policy == POLICY_LRU  ? "LRU"  : "PRIORITY");
    printf("Type QUIT to exit. Commands: INSERT ACCESS DELETE PRINT STATS\n\n");

    char line[LINE_MAX_LEN];
    while (1) {
        /* Only show prompt when reading from a terminal */
        if (isatty(fileno(stdin)))
            printf("> ");

        if (!fgets(line, sizeof(line), stdin))
            break;   /* EOF */

        /* Strip trailing newline */
        line[strcspn(line, "\n")] = '\0';

        if (command_dispatch(buf, line))
            break;   /* QUIT received */
    }

    buffer_destroy(buf);
    return EXIT_SUCCESS;
}
