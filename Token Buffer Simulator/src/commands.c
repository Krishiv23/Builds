#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "commands.h"
#include "buffer.h"

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

/* Trim leading whitespace; return pointer into s. */
static const char *ltrim(const char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

/*
 * Extract a quoted string from src into dest (at most dest_size-1 chars).
 * Expects src to point at the opening '"'.
 * Returns 1 on success, 0 on failure.
 */
static int extract_quoted(const char *src, char *dest, int dest_size)
{
    src = ltrim(src);
    if (*src != '"') return 0;
    src++;  /* skip opening quote */

    int i = 0;
    while (*src && *src != '"') {
        if (i < dest_size - 1)
            dest[i++] = *src;
        src++;
    }
    dest[i] = '\0';
    return (*src == '"');  /* must end with closing quote */
}

/* ------------------------------------------------------------------ */
/* Command dispatch                                                      */
/* ------------------------------------------------------------------ */

int command_dispatch(Buffer *buf, const char *line)
{
    /* Skip blank lines and comments */
    line = ltrim(line);
    if (*line == '\0' || *line == '#')
        return 0;

    char cmd[32] = {0};
    if (sscanf(line, "%31s", cmd) != 1)
        return 0;

    /* ---- QUIT ---- */
    if (strcmp(cmd, "QUIT") == 0) {
        printf("Goodbye.\n");
        return 1;
    }

    /* ---- PRINT ---- */
    if (strcmp(cmd, "PRINT") == 0) {
        buffer_print(buf);
        return 0;
    }

    /* ---- STATS ---- */
    if (strcmp(cmd, "STATS") == 0) {
        buffer_stats(buf);
        return 0;
    }

    /* ---- ACCESS <id> ---- */
    if (strcmp(cmd, "ACCESS") == 0) {
        int id;
        if (sscanf(line + strlen(cmd), "%d", &id) != 1) {
            fprintf(stderr, "ERROR: ACCESS requires an integer id\n");
            return 0;
        }
        buffer_access(buf, id);
        return 0;
    }

    /* ---- DELETE <id> ---- */
    if (strcmp(cmd, "DELETE") == 0) {
        int id;
        if (sscanf(line + strlen(cmd), "%d", &id) != 1) {
            fprintf(stderr, "ERROR: DELETE requires an integer id\n");
            return 0;
        }
        buffer_delete(buf, id);
        return 0;
    }

    /* ---- INSERT <id> <priority> "<data>" ---- */
    if (strcmp(cmd, "INSERT") == 0) {
        int id, priority;
        const char *rest = line + strlen(cmd);

        if (sscanf(rest, "%d %d", &id, &priority) != 2) {
            fprintf(stderr, "ERROR: INSERT requires: INSERT <id> <priority> \"data\"\n");
            return 0;
        }

        /* Advance past the two integers to find the quoted string */
        rest = ltrim(rest);
        /* skip id */
        while (*rest && !isspace((unsigned char)*rest)) rest++;
        rest = ltrim(rest);
        /* skip priority */
        while (*rest && !isspace((unsigned char)*rest)) rest++;
        rest = ltrim(rest);

        char data[TOKEN_DATA_MAX];
        if (!extract_quoted(rest, data, TOKEN_DATA_MAX)) {
            fprintf(stderr, "ERROR: INSERT data must be a quoted string, e.g. \"hello\"\n");
            return 0;
        }

        buffer_insert(buf, id, priority, data);
        return 0;
    }

    fprintf(stderr, "ERROR: unknown command \"%s\"\n", cmd);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Policy parsing                                                        */
/* ------------------------------------------------------------------ */

EvictPolicy parse_policy(const char *s)
{
    if (strcmp(s, "FIFO")     == 0) return POLICY_FIFO;
    if (strcmp(s, "LRU")      == 0) return POLICY_LRU;
    if (strcmp(s, "PRIORITY") == 0) return POLICY_PRIORITY;

    fprintf(stderr, "ERROR: unknown policy \"%s\". Using FIFO.\n", s);
    return POLICY_FIFO;
}
