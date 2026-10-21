# C++ STL — Learning Notes (Interview)

For each container: **what · internals · complexity · invalidation · thread safety · when to use**.

---

## 0. Global rules (apply to almost all STL)

### Thread safety (standard guarantee)

| Scenario | Safe? |
|----------|-------|
| Many threads **read** same container (only `const` ops) | **Yes** |
| Threads modify **different elements**, no structural change | **Yes** (most containers) |
| Any thread does insert/erase/reallocate while others access | **No** — need external mutex |
| Two threads write **same element** | **No** |

**None** of the standard containers are internally synchronized.  
`const` methods are assumed thread-safe w.r.t. each other; non-`const` need external sync if shared.

### Iterator categories (quick)

| Category | Examples | Can |
|----------|----------|-----|
| Contiguous | `vector`, `array`, `string` | pointer arithmetic |
| Random access | + `deque` | `it + n`, `[]` |
| Bidirectional | `list`, `set`, `map` | `++` / `--` |
| Forward | `forward_list`, `unordered_*` | `++` only |
| Input/Output | streams / single-pass | limited |

### Complexity notation
- **Amortized** = average over many ops (e.g. `vector::push_back`)
- **Avg** vs **Worst** for hash tables (depends on hash quality / load)

---

## 1. `std::vector` — `<vector>`

**What:** Dynamic contiguous array. **Default sequence container.**

**Internals:** Single heap buffer (`T* data`, `size`, `capacity`). Grows by reallocate + move/copy (typically ~2×).

| Operation | Time | Notes |
|-----------|------|-------|
| `v[i]`, `at`, `front`, `back` | O(1) | `at` throws if OOB |
| `push_back` / `emplace_back` | **Amortized O(1)** | Worst O(n) on reallocation |
| `pop_back` | O(1) | |
| `insert` / `erase` middle | O(n) | shifts elements |
| `insert` at end | amort O(1) | |
| find (linear) | O(n) | |
| `reserve(n)` | O(n) if grows | prevents realloc |
| `size` / `empty` / `capacity` | O(1) | |

**Iterator invalidation:**
- `push_back`: all invalidated **if** reallocated; else only `end()`
- `insert`/`erase`: at and after point (+ all if reallocate)
- `reserve`/`clear`/`shrink_to_fit`: may invalidate all

**Thread safety:** Not safe for concurrent writes. Concurrent read OK. Concurrent write to **different indices** OK **if no reallocation**.

**Extras:**
```cpp
v.reserve(n);           // set capacity
v.shrink_to_fit();      // non-binding request to free capacity
std::vector<bool> vb;   // SPECIAL — bit-packed proxy refs; avoid if possible
```

**Use when:** random access, cache-friendly, most algorithms.  
**Avoid when:** frequent insert/erase at front/middle.

---

## 2. `std::array` — `<array>`

**What:** Fixed-size array with STL interface (stack or static storage if local/global).

**Internals:** `T elems[N];` inside the object (no heap).

| Op | Time |
|----|------|
| access / iterate | O(1) / O(n) |
| size | O(1) compile-time `N` |

**Invalidation:** N/A for growth (no resize). Pointers to elements stable while object lives.

**Thread safety:** Same as vector elements — external sync if shared writes.

**Use when:** size known at compile time; want value semantics + STL algos.

---

## 3. `std::deque` — `<deque>`

**What:** Double-ended queue; fast push/pop **both** ends + random access.

**Internals:** Map of fixed-size blocks (chunked array). Not fully contiguous.

| Operation | Time |
|-----------|------|
| `operator[]`, `at` | O(1) |
| `push/pop_front`, `push/pop_back` | **Amortized O(1)** |
| insert/erase middle | O(n) |
| iterate all | O(n) but worse locality than `vector` |

**Invalidation:**
- Insert/erase at ends: may invalidate all iterators (pointers/refs to elements often stay valid for end ops — implementation-defined details; don’t rely across impls)
- Middle insert/erase: invalidate all

**Thread safety:** Not internally synced.

**Use when:** need both-end growth + indexing.  
**Prefer `vector`** if only back ops + iteration (better cache).

---

## 4. `std::list` — `<list>`

**What:** Doubly linked list.

**Internals:** Nodes with `prev`/`next` (+ stored size in all major impls).

| Operation | Time |
|-----------|------|
| insert/erase at known iterator | **O(1)** |
| `push_front` / `push_back` | O(1) |
| `size` (C++11+) | O(1) |
| find / traverse | O(n) |
| random access | **N/A** |

**Invalidation:** insert — none; erase — only erased element. Iterators to others stay valid.

**Thread safety:** Not synced. Different nodes can be read concurrently; structural changes need mutex.

**Use when:** many splice/insert/erase in middle **with held iterators**; stable iterators.  
**Usually slower than `vector`** due to allocator + poor locality — rare in practice.

**Nice API:** `splice`, `merge`, `remove_if`, `unique`, `sort` (member — O(n log n)).

---

## 5. `std::forward_list` — `<forward_list>`

**What:** Singly linked list.

**Internals:** Forward nodes only. No `size()` stored (C++ — `size` would be O(n) if you count).

| Op | Time |
|----|------|
| `push_front`, insert_after | O(1) |
| erase_after | O(1) |
| traverse | O(n) |

**No** `push_back`, no `--it`. Saves memory vs `list`.

**Thread safety:** Not synced.

**Use when:** memory-tight forward-only lists (rare in interviews).

---

## 6. `std::string` / `std::string_view` — `<string>` / `<string_view>`

### `string`
**Internals:** Contiguous chars. SSO (Small String Optimization): short strings live **inside** the object (no heap) — typical threshold ~15–23 bytes (impl-defined).

| Op | Time |
|----|------|
| `operator[]`, `size` | O(1) |
| `append` / `+=` / `push_back` | amort O(1) |
| `find` / `substr` | O(n) typical |
| `insert`/`erase` middle | O(n) |
| compare | O(min lengths) |

**Invalidation:** similar to `vector` (realloc on growth). SSO → growth may move from internal to heap.

**Thread safety:** Not synced.

### `string_view` (C++17)
Non-owning (`ptr + length`). **O(1)** create from string/literal.  
**Not a container** — no alloc. Dangling if underlying storage dies.

**Use:** `string` to own; `string_view` as read-only function params.

---

## 7. `std::set` / `std::multiset` — `<set>`

**What:** Sorted unique (`set`) or sorted multi (`multiset`) keys.

**Internals:** Balanced BST — typically **Red-Black Tree** (libstdc++, libc++, MSVC).

| Operation | Time |
|-----------|------|
| insert / erase / find | **O(log n)** |
| `lower_bound` / `upper_bound` | O(log n) |
| iterate sorted | O(n) |
| `count` | O(log n) set; O(log n + k) multiset |

**Invalidation:** insert — no invalidation of existing iterators; erase — only erased.

**Order:** `Compare` (default `std::less<Key>`). Custom comparator for reverse/special order.

**Thread safety:** Not synced.

**Use when:** need sorted unique keys, order statistics via iteration, closest bounds.

```cpp
std::set<int> s{3,1,2};
auto it = s.lower_bound(2);  // first >= 2
```

---

## 8. `std::map` / `std::multimap` — `<map>`

**What:** Sorted key → value. Unique keys (`map`) or multi.

**Internals:** Same **Red-Black Tree** of `pair<const Key, T>`.

| Operation | Time |
|-----------|------|
| `m[k]` / `at` / find / insert / erase | **O(log n)** |
| iterate (sorted by key) | O(n) |
| `lower_bound` | O(log n) |

**Note:** `operator[]` **default-constructs** value if missing (use `insert`/`try_emplace`/`contains` when you don’t want that).

**Invalidation:** same as set — erase invalidates only erased.

**Thread safety:** Not synced. Concurrent read OK; writes need mutex. Different elements’ mapped values can be modified concurrently if no rehash/structure change (tree structure unchanged).

**Use when:** ordered keys, range queries by key.  
**Prefer `unordered_map`** for pure lookup without order.

```cpp
std::map<std::string, int> m;
m.try_emplace("a", 1);           // C++17
if (auto it = m.find("a"); it != m.end()) { /* */ }
```

---

## 9. `std::unordered_set` / `unordered_multiset` — `<unordered_set>`

**What:** Hash table of keys.

**Internals:** **Hash table** — typically array of buckets + **chaining** (libstdc++/libc++). Load factor triggers rehash.

| Operation | Avg | Worst |
|-----------|-----|-------|
| find / insert / erase | **O(1)** | O(n) |
| iterate all | O(n) | bucket walk |
| `bucket_count`, `load_factor` | O(1) | |

**Requirements:** `std::hash<Key>` + `==`. Custom hash for pairs/structs.

**Invalidation:**
- insert: may **rehash** → invalidate all iterators
- erase: invalidates only erased (no rehash usually)

**Thread safety:** Not synced. Concurrent finds OK if no insert/rehash.

**Use when:** fast membership; no order needed.

---

## 10. `std::unordered_map` / `unordered_multimap` — `<unordered_map>`

**What:** Hash map key → value.

**Internals:** Same as unordered_set — **hashed buckets + nodes** storing `pair<const Key, T>`.

| Operation | Avg | Worst |
|-----------|-----|-------|
| `find`, `operator[]`, insert, erase | **O(1)** | O(n) |
| iterate | O(bucket_count + n) | unordered |

**`operator[]`:** inserts default if missing — O(1) avg.

**Invalidation:** rehash invalidates all iterators/refs/pointers; erase invalidates erased only.

**Thread safety:** **Not** thread-safe for writes. One of the most common interview questions — wrap with mutex or use concurrent hash map library.

**Extras:**
```cpp
m.reserve(n);              // buckets for n elements (hint)
m.max_load_factor(0.5);
// custom hash
struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const noexcept {
        return std::hash<long long>{}((long long)p.first << 32 ^ p.second);
    }
};
std::unordered_map<std::pair<int,int>, int, PairHash> mp;
```

**Use when:** fastest average lookup/insert.  
**Avoid when:** need sorted order, adversarial keys without good hash, tight worst-case latency (use `map`).

---

## 11. Container adapters

### `std::stack` — `<stack>`
**Internals:** Default **`deque`** (can use `vector`/`list`).

| Op | Time |
|----|------|
| push / pop / top / size | O(1) amort (depends on underlying) |

**No iterators.** LIFO. Not thread-safe.

### `std::queue` — `<queue>`
**Internals:** Default **`deque`**.

| Op | Time |
|----|------|
| push / pop / front / back | O(1) amort |

FIFO. Not thread-safe.

### `std::priority_queue` — `<queue>`
**What:** Max-heap by default.

**Internals:** Default **`vector`** + **binary heap** algorithms (`make_heap`/`push_heap`/`pop_heap`).

| Op | Time |
|----|------|
| `push` / `emplace` | **O(log n)** |
| `pop` | **O(log n)** |
| `top` | **O(1)** |

```cpp
// max-heap (default)
std::priority_queue<int> maxh;
// min-heap
std::priority_queue<int, std::vector<int>, std::greater<int>> minh;
```

**No decrease-key** API. Not thread-safe. No iterators.

**Use when:** Dijkstra top-K scheduling. For Dijkstra often need `set` or custom heap if decrease-key matters.

---

## 12. `std::bitset` — `<bitset>`

**What:** Fixed-N bit flags (`bitset<N>`).

**Internals:** Packed bits in words (array of `unsigned long` / similar).

| Op | Time |
|----|------|
| `operator[]`, set/reset/test | O(1) |
| count / bitwise ops | O(N / word_size) |

**Not** dynamically sized (`vector<bool>` is dynamic but hated). Not thread-safe for concurrent writes to same object.

---

## 13. `std::span` (C++20) — `<span>`

**What:** Non-owning view over contiguous `T[extent]`.

**Internals:** pointer + size. **O(1)** construct.

No ownership, no alloc. Thread safety = same as viewing underlying data.

---

## 14. Complexity cheat sheet (side-by-side)

| Container | Access | Search | Insert end | Insert middle | Memory / structure |
|-----------|--------|--------|------------|---------------|--------------------|
| `vector` | O(1) | O(n) | amort O(1) | O(n) | contiguous array |
| `deque` | O(1) | O(n) | amort O(1) both ends | O(n) | block map |
| `list` | O(n) | O(n) | O(1) | **O(1)** at known pos | doubly linked |
| `set`/`map` | — | O(log n) | O(log n) | O(log n) | red-black tree |
| `unordered_*` | — | **avg O(1)** | avg O(1) | avg O(1) | hash table |
| `priority_queue` | top O(1) | — | O(log n) push | — | binary heap on vector |
| `stack`/`queue` | ends O(1) | — | O(1) | — | adapter over deque |

---

## 15. Iterator invalidation summary

| Container | Insert | Erase |
|-----------|--------|-------|
| `vector`/`string` | reallocate → **all**; else after point | at/after point |
| `deque` | ends: tricky; middle → **all** | similar |
| `list`/`set`/`map` | none for others | **only erased** |
| `unordered_*` | rehash → **all** | only erased |

---

## 16. Choosing a container

```
Need index / cache scan?           → vector
Fixed size compile-time?           → array
Push/pop both ends + index?        → deque
Stable iters + splice?             → list (rare)
Sorted unique / order queries?     → set / map
Fastest avg lookup, no order?      → unordered_map / set
Top priority repeatedly?           → priority_queue
LIFO / FIFO only?                  → stack / queue
Bits?                              → bitset / uint64_t masks
Borrow contiguous?                 → span / string_view
```

---

## 17. Important `<algorithm>` + complexity

| Algorithm | Complexity | Needs |
|-----------|------------|-------|
| `sort` | O(n log n) | random access |
| `stable_sort` | O(n log² n) typ. | random access |
| `partial_sort` | O(n log k) | |
| `nth_element` | avg O(n) | |
| `binary_search` / `lower_bound` / `upper_bound` | O(log n) | **sorted** |
| `find` | O(n) | |
| `count` / `count_if` | O(n) | |
| `accumulate` | O(n) | `<numeric>` |
| `next_permutation` | O(n) | |
| `merge` | O(n+m) | sorted inputs |
| `make_heap` / `push_heap` | O(n) / O(log n) | |

```cpp
std::sort(v.begin(), v.end());
auto it = std::lower_bound(v.begin(), v.end(), x); // first >= x
std::nth_element(v.begin(), v.begin()+k, v.end()); // k-th in place
```

Parallel versions (C++17): `std::sort(std::execution::par, ...)`.

---

## 18. Allocators & growth (interview extras)

- All containers take an **Allocator** template param (default `std::allocator<T>` → `operator new`).
- `vector` capacity growth factor ≈ **2×** (impl-defined; some use 1.5×).
- `unordered_*` **rehash** when `size > load_factor * bucket_count`.
- Node-based containers (`list`, `map`, `unordered_map`): one alloc per node (pool allocators help in hot paths).

---

## 19. Thread safety — interview answer (memorize)

> “STL containers are not thread-safe for concurrent modification. Multiple threads may concurrently read a container. Concurrent reads and writes, or structural modifications, require external synchronization such as a mutex. Concurrent modification of different elements is allowed if no reallocation or rehash occurs. For shared maps in servers, use a mutex per shard or a concurrent hash map library.”

---

## 20. Common interview Qs

1. **vector vs list?** Contiguous vs nodes; vector almost always faster.  
2. **map vs unordered_map?** O(log n) worst-case guaranteed + ordered vs avg O(1) unordered.  
3. **How does unordered_map handle collision?** Chaining (typical) / open addressing (some impls — libstdc++ uses chaining).  
4. **Why vector grows?** Amortize realloc; geometric growth.  
5. **Is map a heap?** No — RB-tree. Heap is `priority_queue`.  
6. **vector\<bool\>?** Bit-packed specialization; not a real container of `bool&`.  
7. **Is string SSO?** Yes in major implementations.  
8. **priority_queue underlying?** `vector` + heap algorithms.  
9. **Can you iterate stack?** No — adapter without iterators.  
10. **Thread-safe map?** Not in std; wrap with mutex.  

---

## 21. Mini examples

```cpp
// LRU building blocks: list (order) + unordered_map (lookup)
std::list<std::pair<int,int>> lru;
std::unordered_map<int, std::list<std::pair<int,int>>::iterator> idx;

// Dijkstra-style
std::priority_queue<std::pair<long,int>,
                    std::vector<std::pair<long,int>>,
                    std::greater<>> pq;

// Frequency map
std::unordered_map<int, int> freq;
for (int x : a) ++freq[x];
```
