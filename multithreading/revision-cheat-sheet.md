# Multithreading — Revision Cheat Sheet (C++)

Flash revision. Full explanations: [learning-notes.md](learning-notes.md)  
Playlist base: CppNuts Multithreading In C++ (+ interview extras §17–30).

---

## Instant recall

| Concept | One-liner |
|---------|-----------|
| Thread vs process | Shared memory vs separate address space |
| Data race | Unsynced non-atomic shared write → **UB** |
| `std::thread` | Must `join` or `detach` before dtor |
| Mutex | Exclusive critical section |
| `lock_guard` | Simple RAII lock |
| `unique_lock` | Flexible; required for CV |
| `scoped_lock` | C++17 multi-lock, deadlock-free |
| CV | Wait on predicate; no busy spin |
| Deadlock | Circular wait → fixed order / `scoped_lock` |
| `promise`/`future` | One-shot result (+ exception) |
| `async` | Task → future; dtor may block |
| `atomic` | Lock-free shared primitive |
| `shared_mutex` | N readers XOR 1 writer |
| `call_once` | Thread-safe one-time init |
| `thread_local` | Per-thread storage |
| False sharing | Adjacent writes fight for cache line |
| Thread pool | N workers + job queue + CV |

---

## API flashcards

```cpp
std::thread t(fn, args...);
if (t.joinable()) t.join();

std::mutex m;
std::lock_guard<std::mutex> g(m);
std::unique_lock<std::mutex> lk(m);
std::scoped_lock lock(m1, m2);

cv.wait(lk, [&]{ return ready; });
cv.notify_one();  // or notify_all

std::atomic<int> n{0}; n.fetch_add(1);
ready.store(true, std::memory_order_release);
ready.load(std::memory_order_acquire);

std::shared_lock r(sm);   // readers
std::unique_lock w(sm);   // writer

std::call_once(flag, init_fn);
auto f = std::async(std::launch::async, fn, args...);
f.get();

std::promise<int> p; auto fut = p.get_future();
p.set_value(42);
```

---

## Create threads (5 ways)

1. Function pointer  
2. Lambda  
3. Functor  
4. Non-static member (`&Cls::fn`, `&obj`, args)  
5. Static member  

Args **copied** by default → `std::ref(x)` for references.

---

## Join / detach rules

| Rule | Detail |
|------|--------|
| Joinable at dtor | `std::terminate` |
| `join` | Wait + reclaim |
| `detach` | Independent; no later join |
| Detach hazard | Don’t pass refs to stack locals |

---

## Which lock?

```
simple critical section     → lock_guard / scoped_lock
need unlock/relock or CV    → unique_lock
lock many mutexes           → scoped_lock (or std::lock + adopt)
read-heavy map/cache        → shared_mutex
try without blocking        → try_lock / timed_mutex
same thread re-enters       → recursive_mutex (prefer redesign)
```

---

## Condition variable checklist

1. Shared state protected by mutex  
2. Always `wait(lock, predicate)` (spurious wakeups)  
3. Change state, then `notify_*`  
4. `notify_one` vs `notify_all` — wake only who you need  
5. Don’t hold lock during heavy work after wakeup if avoidable  

---

## Deadlock — 4 Coffman conditions

1. Mutual exclusion  
2. Hold and wait  
3. No preemption  
4. Circular wait  

**Break:** global lock order · `scoped_lock` · avoid nested locks · don’t call callbacks under lock.

---

## Future family

| Type | Role |
|------|------|
| `promise` | set_value / set_exception |
| `future` | get once / wait |
| `shared_future` | multiple getters |
| `packaged_task` | wrap fn → future |
| `async` | launch + future |

**async policies:** `async` (new thread) · `deferred` (lazy on get) · default = implementation choice.  
**Gotcha:** ignored `async` future → blocking dtor.

---

## Memory orders (say this)

| Order | When |
|-------|------|
| `seq_cst` | Default answer; simplest |
| `release`/`acquire` | Publish flag + payload |
| `relaxed` | Pure counters, no sync |

“Release-acquire synchronizes-with → prior writes become visible.”

---

## Patterns to code blind

| Pattern | Core pieces |
|---------|-------------|
| Safe counter | `atomic` or `mutex` |
| Bounded PC queue | mutex + not_empty + not_full |
| Thread pool | workers + queue + CV + stop flag |
| RW cache | `shared_mutex` |
| Lazy singleton | Meyers / `call_once` |
| Transfer $$ | `scoped_lock(m1,m2)` |

---

## Performance / systems talking points

- False sharing → `alignas(64)` pad hot atomics  
- Spinlock only for tiny, low-contention sections  
- Prefer sharding / message passing over one giant mutex  
- Thread pool > unbounded `std::thread` creation  
- TSan: `-fsanitize=thread`  

---

## Common bugs (scan before submit)

- [ ] Data race (no sync)  
- [ ] Forgotten join  
- [ ] Detach + dangling stack ref  
- [ ] CV wait without predicate  
- [ ] Opposite lock order  
- [ ] `future::get()` twice  
- [ ] Holding lock across I/O / callback  
- [ ] Ignoring `async` future lifetime  

---

## 90-second verbal summary

> “C++11 gives `std::thread` plus mutexes and condition variables. Always join or detach. Protect shared data with RAII locks — `lock_guard` for simple cases, `unique_lock` with CVs, `scoped_lock` for multiple mutexes to avoid deadlock. Use atomics for flags/counters with acquire/release when publishing data. Higher level: `async`/`future`/`promise`, and in production a thread pool with a job queue. Watch for data races, deadlock, false sharing, and test with ThreadSanitizer.”

---

## Compile

```bash
g++ -std=c++17 -O2 -pthread file.cpp -o app
g++ -std=c++17 -g -O1 -fsanitize=thread -pthread file.cpp -o app
```
