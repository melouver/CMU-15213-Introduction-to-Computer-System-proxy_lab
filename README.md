# proxy_lab
## Cache eviction policy
FIFO

## Sorry, I can't do it with LRU. So Any advice would be greatly appreciated

这是我对于CSAPP最后一个实验proxy lab的代码实现。实现方法较为粗糙。连接管理方式为one thread per connection，并没有实现线程池。
缓存的更新策略是FIFO而不是LRU或者CLOCK(second chance)，原因在于我认为如果要在多线程模型下实现LRU的每个bookkeeping操作，那么这会退化成普通的FIFO。因为LRU对于读者需要记录读取时间，这个时间可以是逻辑时间（counter)，也可以是physical time，但无论如何，对于读者仍然有写操作，和写者没有本质区别。很显然，我对(高)并发的理解太粗浅了，所以只实现了个demo。proxy是一个极其重要的middleware，我还需要学习一个。
