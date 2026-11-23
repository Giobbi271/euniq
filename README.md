# euniq

A C implementation of `uniq` that filters out **non-adjacent** duplicate lines across an entire file or stream while preserving the original order of first appearance.

Unlike standard POSIX `uniq`—which only compares consecutive lines and typically requires piping through `sort` first—`euniq` tracks previously seen lines in a dynamically resized hash table, allowing single-pass deduplication without altering line order.

## Build & Install

```bash
make
sudo make install   # installs to /usr/local/bin/euniq
```

To build with AddressSanitizer and UndefinedBehaviorSanitizer enabled, or run the test suite:

```bash
make debug
make test
```

## Usage

```text
euniq [INPUT [OUTPUT]]
```

* With no arguments, `euniq` reads from `stdin` and writes to `stdout`.
* With one argument, it reads from `INPUT` and writes to `stdout`.
* With two arguments, it reads from `INPUT` and writes to `OUTPUT`.

### Examples

```bash
# Deduplicate a file to stdout
euniq access.log

# Deduplicate from a pipeline
cat raw_data.txt | euniq > clean_data.txt

# Read from input file and write to output file
euniq input.txt output.txt
```

## Design & Implementation

* **Hash Table Lookup ($O(1)$ average time):** Detecting duplicates anywhere in the input requires remembering every unique line encountered. Scanning a linked list or array on each line would degrade to $O(N^2)$ time. A hash table keeps insertion and lookup at $O(1)$ average time, making overall execution $O(N)$ in the number of input lines.
* **Open Addressing & Linear Probing:** Because `euniq` only inserts keys and never deletes them, open addressing avoids pointer overhead and simplifies memory layout. With a conservative maximum load factor of $\alpha = 0.5$, linear probing stays fast and cache-friendly without needing double hashing or quadratic probing.
* **Dynamic Resizing for Streams:** Because `euniq` supports non-seekable streams like `stdin`, it cannot perform a preliminary pass with `rewind()` to count lines upfront. Instead, it starts with a small prime table size (67 buckets) and doubles to the next prime whenever the load factor exceeds `0.5`.
* **String Hashing:** Keys are hashed using Horner's polynomial rolling hash method with base `127`.
* **Arbitrary Line Lengths & Signal Handling:** Input lines are read dynamically via POSIX `getline()`, avoiding fixed buffer truncation, and `SIGINT` (`Ctrl+C`) is intercepted cleanly so open file descriptors are flushed and closed before exiting.

## Current Limitations

* **No CLI Flags:** Currently acts as a focused proof of concept without GNU/POSIX flags such as `-c` (count), `-d` (only duplicates), or `-i` (case-insensitive comparison).
* **Single-Threaded Execution** 
