# System Design for C++ Developers

C++-only notes. Focus: systems, latency, memory, concurrency — not Java enterprise stacks.

---

## 1. What C++ SD interviews test

| Track | Examples | Emphasis |
|-------|----------|----------|
| **Low-latency / HFT-style** | Order book, market data handler | Cache, alloc, lock-free, latency |
| **Infrastructure** | Cache, message queue, key-value store | Throughput, durability, networking |
| **Embedded / realtime** | Telemetry pipeline, device agent | Determinism, memory bounds |
| **Backend services (C++)** | Auth gateway, media server | Concurrency, sockets, protocols |

You design with **processes/threads, memory, I/O, and APIs in C++ terms** — not Spring/JVM GC tuning.

---

## 2. Design template (use every time)

1. **Requirements** — functional + non-functional (latency p99, QPS, memory, durability)
2. **API / wire format** — structs, binary vs JSON, versioning
3. **Core data structures** — what lives in RAM, ownership model
4. **Concurrency model** — threads, shard-by-key, actor, event loop
5. **I/O path** — sync vs async (`epoll`/`io_uring`), batching
6. **Failure** — crash, partial write, backpressure
7. **Observability** — metrics, structured logs (cost of logging!)
8. **Trade-offs** — latency vs throughput vs complexity

---

## 3. C++ building blocks (name these)

### Memory & ownership
- **RAII** for sockets, FDs, locks, mmap
- `unique_ptr` / arenas / object pools — avoid alloc on hot path
- Custom allocators, preallocation, slab pools
- Cache line awareness (64B), false sharing → pad atomics / shard counters

### Concurrency
- Shard data → minimize cross-thread locks
- SPSC/MPSC lock-free queues for hot paths
- Thread-per-core + pin (`pthread_setaffinity_np`) when latency matters
- Prefer message passing over shared mutable state

### I/O & networking
- TCP sockets, non-blocking + `epoll`/`kqueue`
- Framing: length-prefix binary protocols
- Zero-copy ideas: `writev`, sendfile, shared memory ring buffers
- Serialization: FlatBuffers/Cap’n Proto/protobuf (know trade-offs)

### Persistence
- Append-only log + index (WAL pattern)
- `mmap` for read-heavy datasets
- fsync batching vs durability

---

## 4. Classic C++ design problems

### A. In-memory key-value store
```
Client → TCP → Connection threads / event loop
              → Sharded hash maps (N mutexes or concurrent hash)
              → Optional WAL on disk
```
**Talk about:** hash map + chaining/open addressing, shard by `hash(key) % N`, eviction (LRU list + map), serialization, snapshot + WAL replay.

### B. Producer–consumer message queue
```
Producers → lock-free / mutex queue → Consumers
         → persistence (optional) → acknowledgements
```
**Talk about:** bounded queue + backpressure, batching, at-least-once vs exactly-once, consumer groups.

### C. Order book (HFT-flavored)
```
Market data → decode → book update → matching → outbound
```
**Talk about:** price-time priority, `std::map`/`flat_map` vs intrusive lists, object pool for orders, single-writer principle, avoid alloc in match loop.

### D. Rate limiter
- Token bucket / sliding window
- Per-key atomic counters or sharded maps
- Distributed: Redis-like or local + sync (mention briefly)

### E. Thread pool
```
N workers + concurrent job queue + condition_variable
```
**Talk about:** bounded queue, graceful shutdown (`jthread`/poison pill), work stealing (advanced).

### F. Cache (LRU / LFU)
```cpp
// LRU sketch: list (recency) + unordered_map<key, list::iterator>
// thread-safe: mutex per shard or read-write lock
```

---

## 5. Latency budget language

Break down a request:
```
network → parse → business logic → serialize → network
```
Optimize:
1. Fewer syscalls / batch
2. Fewer allocations / reuse buffers
3. Better locality (SoA vs AoS when relevant)
4. Fewer locks / finer sharding
5. Precompute / cache

**Numbers to know roughly:**
- L1 ~1ns, L3 ~10–40ns, DRAM ~100ns
- Context switch ~µs, disk seek/SSD vary, datacenter RTT ~0.5ms+

---

## 6. Patterns (C++ wording)

| Pattern | C++ angle |
|---------|-----------|
| Reactor / event loop | Single thread + non-blocking FDs |
| Thread pool | Workers + queue |
| Pipeline | Stages connected by SPSC queues |
| Sharding | Partition state by key |
| Object pool | Prealloc nodes for orders/connections |
| RAII guard | Lock, FD, mmap wrapper |
| Pimpl | Stable ABI / hide impl |

Avoid Java-only jargon (Servlet, JVM heap, GC pauses) unless comparing.

---

## 7. Sample answer skeleton — “Design a concurrent LRU cache”

1. **API:** `get(key) → optional<value>`, `put(key,value)`, capacity `N`
2. **Structure:** `unordered_map` + `list` of keys; O(1) move-to-front
3. **Threading:**
   - Option A: one `shared_mutex` (simple, contended)
   - Option B: shard into M maps, lock per shard
4. **Memory:** store values by value or `shared_ptr`; define ownership
5. **Extras:** TTL, metrics (hit rate), persistence optional
6. **Code-level:** mention `std::list` iterator stability, why not invalidate iterators incorrectly

---

## 8. What *not* to lean on

- No Spring/Hibernate/Tomcat
- No “just use a managed language GC”
- Don’t ignore ownership and lifetime — C++ interviewers will probe

---

## 9. Practice set

1. Thread-safe LRU cache  
2. Multithreaded web crawler (C++ sockets or abstract I/O)  
3. In-memory DB with WAL  
4. Job scheduler / thread pool  
5. Chat server or pub-sub (TCP)  
6. Metrics aggregator (high write rate)  

For each: draw data flow, state ownership, lock map, failure modes.
