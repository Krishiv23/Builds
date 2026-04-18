# Memory Allocator

A simple custom memory allocator written in C. It implements basic versions of `_malloc()`, `_calloc()`, `_realloc()`, and `_free()` using block metadata and the `sbrk()` system call. The allocator keeps track of each block’s size, free status, pointer, and next block, and it can split larger free blocks when needed. :contentReference[oaicite:0]{index=0}

## Features

- Custom `_malloc()` implementation
- Custom `_calloc()` implementation
- Custom `_realloc()` implementation
- Custom `_free()` implementation
- Free block tracking with linked metadata
- Block splitting for better space usage
- Simple demo in `main()` showing allocation, zero-initialized memory, reallocation, and free operations. :contentReference[oaicite:1]{index=1}

## How it works

Each allocated chunk has a metadata structure placed before the user memory. The allocator scans the block list for a free block that is large enough; if none is found, it requests more memory from the system. When a free block is larger than needed, it is split into two blocks. :contentReference[oaicite:2]{index=2}

## Build

Use `gcc` on a POSIX system:

```bash
gcc memory-allocator.c -o memory-allocator
