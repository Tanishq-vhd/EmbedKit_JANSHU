# EmbedKit_JANSHU

Embedded Developer Assignment (Fresher) — Embed Square Solutions Pvt. Ltd.



## Build Instructions

Requires `gcc` with C99 support (Linux, macOS, or Windows with MinGW/WSL).

```bash
gcc -Wall -std=c99 ringbuf.c -o ringbuf
./ringbuf
```

The program compiles with zero warnings and zero errors using
`gcc -Wall -std=c99`.

## Modules

- **ringbuf.c** — An 8-byte circular (ring) buffer for `uint8_t` data,
  modeling a typical UART receive buffer (ISR as producer, main loop as
  consumer). Supports init, write (fails if full), read (fails if empty),
  count query, full check, and empty check. Implements the bonus task:
  head/tail wrap-around uses `& (BUFFER_SIZE - 1)` instead of
  `% BUFFER_SIZE`, since `BUFFER_SIZE` (8) is a power of 2 — this avoids
  a division instruction, which is expensive inside a tight ISR on MCUs
  without a hardware divider. `main()` demonstrates the full required
  write/read/wrap-around sequence with formatted output.
