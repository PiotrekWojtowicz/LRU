# LRU algorithm
This is an implementation of a popular
<b>LRU</b> <i>(least recently used)</i> algorithm which is a page-replacement
algorithm used in the virtual memory management system. 

This is an implementation of a stack-algorithm version.

### Assumptions made

- <b>Page size is equal to 64 bytes</b>
- We can only keep 5 virtual addresses at the time
- The memory is bte addressable
- No TLB/Cache taken into account

### Files

Check the main.c where you can update filenames as well as their paths.

### Valgrind

The algorithm is 100% memory safe, does not trigger any error nor warnings. Tested with valgrind 3.19.0.

- No memory leaks detected
- No warnings occurred
- No invalid memory reads/writes

