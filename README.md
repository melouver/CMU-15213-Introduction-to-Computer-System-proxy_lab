# solution to the CMU 15213: Introduction to Computer Systems (ICS) proxy_lab
## Cache eviction policy
LIFO


This is my implementation of the last project in CSAPP, the proxy lab. The implementation method is relatively rough. The connection management approach is one thread per connection.

The cache update strategy is FIFO instead of LRU or CLOCK (second chance). The reason for this is that I believe that if we want to implement LRU with every bookkeeping operation in a multi-threaded model, it would degrade to a regular FIFO. This is because LRU requires recording the access time for readers, which can be either logical time (counter) or physical time. However, regardless of the approach, readers still have write operations, which are essentially the same as writers.
