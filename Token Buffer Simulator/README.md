# Memory-Constrained Token Buffer Simulator

A CLI simulator for a fixed-size token buffer with pluggable eviction policies.  
Written in C (C11), no external dependencies. Built as a clean undergraduate systems project.

---

## Build

```sh
make
```

Compiler flags used: `gcc -std=c11 -Wall -Wextra -Iinclude -g`

Clean build artifacts:

```sh
make clean
```

---

## Usage

```
./token_buffer <capacity> <policy>
```

| Argument   | Values                  | Description                      |
|------------|-------------------------|----------------------------------|
| `capacity` | positive integer        | Maximum number of tokens in buffer |
| `policy`   | `FIFO` / `LRU` / `PRIORITY` | Eviction policy               |

Commands are read from **stdin**. Run interactively or redirect a file:

```sh
# Interactive
./token_buffer 4 LRU

# Batch (redirect test file)
./token_buffer 4 LRU < tests/test_lru.txt
```

Run all three bundled tests at once:

```sh
make test
```

---

## Commands

| Command                          | Description |
|----------------------------------|-------------|
| `INSERT <id> <priority> "data"`  | Add or update a token. Evicts if buffer is full. |
| `ACCESS <id>`                    | Look up a token. Prints `HIT` or `MISS`. Updates `last_access_time`. |
| `DELETE <id>`                    | Remove a token by id. |
| `PRINT`                          | Display all valid tokens in the buffer. |
| `STATS`                          | Print capacity, load, inserts, evictions, hit/miss counts, hit rate. |
| `QUIT`                           | Exit the program. |

Blank lines and lines starting with `#` are treated as comments and ignored.

### INSERT example

```
INSERT 42 7 "kernel_token"
```

- `id = 42`
- `priority = 7`
- `data = "kernel_token"`

If a token with `id=42` already exists it is **updated in place** — no duplicate slots are created.

---

## Eviction Policies

### FIFO — First In, First Out

The token with the **smallest `insert_time`** (i.e. the one that was inserted earliest) is evicted. Access patterns have no effect on eviction order.

```
INSERT 1 … -> INSERT 2 … -> INSERT 3 … -> INSERT 4 …
Buffer full. INSERT 5 …  -> evicts id=1
```

### LRU — Least Recently Used

The token with the **smallest `last_access_time`** is evicted. Every `ACCESS` command refreshes the clock for that token, keeping it safe from eviction.

```
ACCESS 1  # refresh id=1
ACCESS 2  # refresh id=2
# id=3 is now the least recently accessed -> evicted on next insert
```

### PRIORITY — Lowest Priority First

The token with the **lowest `priority` value** is evicted.  
Tie-break: among tokens with equal priority, the one with the **oldest `insert_time`** is evicted.

```
INSERT 1 priority=5
INSERT 2 priority=1   # lowest
INSERT 3 priority=1   # also lowest, inserted later
# Next eviction: id=2 (priority=1 and older than id=3)
```

---

## Token Fields

| Field             | Type       | Description                              |
|-------------------|------------|------------------------------------------|
| `id`              | `int`      | Unique token identifier                  |
| `data`            | `char[]`   | Payload string (max 255 chars)           |
| `priority`        | `int`      | Higher = more important                  |
| `insert_time`     | `uint64_t` | Logical clock value at insertion         |
| `last_access_time`| `uint64_t` | Logical clock value at last access       |
| `valid`           | `int`      | 1 = occupied slot, 0 = empty             |

The logical clock increments on **every operation** (INSERT, ACCESS, DELETE).

---

## Project Structure

```
token_buffer/
├── Makefile
├── README.md
├── include/
│   ├── token.h        # Token struct
│   ├── buffer.h       # Buffer struct + API
│   └── commands.h     # Command parsing API
├── src/
│   ├── main.c         # Entry point, REPL loop
│   ├── buffer.c       # Core buffer operations + display
│   ├── evict.c        # Eviction policy logic
│   ├── commands.c     # Command parsing and dispatch
│   └── stats.c        # Reserved for future stats extensions
└── tests/
    ├── test_fifo.txt
    ├── test_lru.txt
    └── test_priority.txt
```

---

## Sample Output

```
$ ./token_buffer 4 FIFO < tests/test_fifo.txt

token_buffer  capacity=4  policy=FIFO
Type QUIT to exit. Commands: INSERT ACCESS DELETE PRINT STATS

INSERTED id=1
INSERTED id=2
INSERTED id=3
INSERTED id=4
--- Buffer (4/4) ---
ID     PRIO   VALID    INSERT_CLK   ACCESS_CLK   DATA
...
EVICTED  id=1
INSERTED id=5
```

---

## Design Notes

- **Flat array storage** — no linked lists; simple index scanning for all operations.
- **Logical clock** — a single monotonic counter gives deterministic, reproducible timestamps.
- **No dynamic resizing** — capacity is fixed at startup, matching real embedded/cache scenarios.
- **Duplicate INSERT** — updates the existing token rather than creating a second slot.
- **Safe string copy** — all data copies are bounded to `TOKEN_DATA_MAX - 1` to prevent overflow.

---

## License

MIT
