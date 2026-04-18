CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude -g
TARGET  = token_buffer
SRCS    = src/main.c src/buffer.c src/evict.c src/commands.c src/stats.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run all three test files
test: $(TARGET)
	@echo "=== FIFO test ==="; ./$(TARGET) 4 FIFO < tests/test_fifo.txt
	@echo ""
	@echo "=== LRU test ==="; ./$(TARGET) 4 LRU  < tests/test_lru.txt
	@echo ""
	@echo "=== PRIORITY test ==="; ./$(TARGET) 4 PRIORITY < tests/test_priority.txt

clean:
	rm -f $(OBJS) $(TARGET)
