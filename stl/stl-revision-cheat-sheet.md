# STL — Revision Cheat Sheet

Details: [stl-learning-notes.md](stl-learning-notes.md)

---

## Thread safety (one liner)

**Not internally synchronized.** Concurrent **reads** OK. Any write / rehash / realloc while others access → need **mutex**. Different elements modifiable concurrently if no structural change.

---

## Internals + complexity

| Container | Internal | Access | Search | Insert | Notes |
|-----------|----------|--------|--------|--------|-------|
| `vector` | contiguous buffer | O(1) | O(n) | back amort O(1); mid O(n) | default choice; ~2× grow |
| `array` | `T[N]` in object | O(1) | O(n) | N/A | fixed size |
| `deque` | block map | O(1) | O(n) | both ends amort O(1) | not fully contiguous |
| `list` | doubly linked | O(n) | O(n) | O(1) at iterator | stable iters; poor locality |
| `forward_list` | singly linked | O(n) | O(n) | O(1) after | no `size()` O(1) |
| `string` | contiguous + **SSO** | O(1) | O(n) | like vector | short strings on stack |
| `set`/`map` | **red-black tree** | — | **O(log n)** | O(log n) | sorted |
| `unordered_*` | **hash + buckets** | — | **avg O(1)** | avg O(1) | worst O(n); rehash |
| `stack` | adapter → `deque` | top O(1) | — | O(1) | no iterators |
| `queue` | adapter → `deque` | front O(1) | — | O(1) | FIFO |
| `priority_queue` | **binary heap** on `vector` | top O(1) | — | push/pop O(log n) | max-heap default |
| `bitset<N>` | packed words | O(1) bit | — | N/A | fixed N |
| `span` / `string_view` | ptr+size | O(1) | — | N/A | non-owning |

---

## Invalidation (shortest)

| | Realloc / rehash | Erase |
|-|------------------|-------|
| `vector`/`string` | **all** | at & after |
| `list`/`map`/`set` | never on insert | **only erased** |
| `unordered_*` | **all** on rehash | only erased |

---

## Pick container

```
index + scan          → vector
both-end queue+index  → deque
sorted / bounds       → set / map
fast hash lookup      → unordered_map
priority              → priority_queue
LIFO / FIFO           → stack / queue
```

---

## map vs unordered_map

| | `map` | `unordered_map` |
|-|-------|-----------------|
| Structure | RB-tree | Hash table |
| Complexity | O(log n) guaranteed | avg O(1), worst O(n) |
| Order | sorted keys | none |
| `[]` inserts default | yes | yes |
| Prefer | order / worst-case | raw speed |

---

## priority_queue

```cpp
std::priority_queue<int> maxh;  // max
std::priority_queue<int, std::vector<int>, std::greater<int>> minh;
```
push/pop O(log n), top O(1). No decrease-key.

---

## Algorithms flash

| Algo | Time |
|------|------|
| `sort` | O(n log n) |
| `lower_bound` / `upper_bound` | O(log n) sorted |
| `nth_element` | avg O(n) |
| `find` / `accumulate` | O(n) |
| `make_heap` | O(n) |

---

## Snippets

```cpp
v.reserve(n);
m.try_emplace(k, v);
m.find(k);  // not m[k] if you don't want insert
s.lower_bound(x);
std::sort(v.begin(), v.end());
```

---

## Hot Q&A

1. **vector growth?** Geometric (≈2×) → amort O(1) push_back  
2. **unordered collision?** Chaining (typical GCC/Clang)  
3. **map structure?** Red-black tree  
4. **string SSO?** Short strings inline  
5. **vector\<bool\>?** Bit proxy — avoid  
6. **STL thread-safe?** No for writes  
7. **stack underlying?** `deque` by default  
8. **When list > vector?** Stable iterators / O(1) splice — rare  

---

## Verbal (30s)

> “I default to `vector` for cache locality. Ordered associative containers are red-black trees at O(log n); unordered ones are hash tables at average O(1). Adapters like `priority_queue` are a binary heap on a `vector`. Nothing in the STL is internally locked — concurrent mutation needs a mutex.”
