# Multithreading Learning Notes (C++)

Based on [CppNuts — Mulithreading In C++](https://www.youtube.com/playlist?list=PLk6CEY9XxSIAeK-EAh3hB4fgNvYkYmghp).  
Header: `#include <thread>`, `#include <mutex>`, `#include <condition_variable>`, `#include <future>`.  
Link with `-pthread`.

---

## 1. Threads Introduction

**Process** = OS process with own address space.  
**Thread** = lightweight unit of execution inside a process; shares memory with siblings.

| | Process | Thread |
|-|---------|--------|
| Memory | separate | shared |
| Creation cost | high | low |
| Crash isolation | better | one bad thread can corrupt process |
| IPC | pipes/sockets/shm | shared variables + sync |

**Why multithread?** Overlap I/O & CPU, use multi-core, keep UI/responsive, parallel algorithms.

**C++11 model:** `std::thread` wraps OS threads. Race on shared data without sync = **undefined behavior**.

```cpp
#include <iostream>
#include <thread>

void work(int id) {
    std::cout << "hello from thread " << id << "\n";
}

int main() {
    std::thread t(work, 1);
    t.join();  // wait for finish
}
```

**Interview:** Thread vs process; when multithreading helps vs hurts (contention, GIL-like issues don't apply in C++, but lock contention and false sharing do).

---

## 2. Five Ways to Create Threads

```cpp
#include <thread>
#include <iostream>

void free_func(int x) { std::cout << "free " << x << "\n"; }

class Functor {
public:
    void operator()(int x) const { std::cout << "functor " << x << "\n"; }
};

class Worker {
public:
    void run(int x) { std::cout << "member " << x << "\n"; }
    static void srun(int x) { std::cout << "static " << x << "\n"; }
};

int main() {
    // 1) Function pointer
    std::thread t1(free_func, 1);

    // 2) Lambda
    std::thread t2([](int x) { std::cout << "lambda " << x << "\n"; }, 2);

    // 3) Functor
    std::thread t3(Functor{}, 3);

    // 4) Non-static member function (need object)
    Worker w;
    std::thread t4(&Worker::run, &w, 4);

    // 5) Static member function
    std::thread t5(&Worker::srun, 5);

    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();
}
```

**Pass by ref:** use `std::ref(obj)` — otherwise args are **copied**.

```cpp
void incr(int& n) { ++n; }
int x = 0;
std::thread t(incr, std::ref(x));
t.join();  // x == 1
```

---

## 3. Join / Detach / Joinable

| Call | Effect |
|------|--------|
| `join()` | Block until thread finishes; then thread is no longer joinable |
| `detach()` | Thread runs independently; you **cannot** join later |
| `joinable()` | `true` if associated with an active thread of execution |

**Rule:** If `joinable()` is true when `std::thread` destructor runs → **`std::terminate`**. Always `join` or `detach` before destroy.

```cpp
std::thread t([]{ /* ... */ });
if (t.joinable()) t.join();

// Detach example (rare in apps — prefer join or thread pool)
std::thread bg([]{ /* fire and forget */ });
bg.detach();  // careful: don't touch stack locals after main continues
```

**Don't** pass references/pointers to locals into a detached thread — UB after scope ends.

---

## 4. Mutex

**Mutex** = mutual exclusion. Only one thread holds the lock at a time.

```cpp
#include <mutex>
#include <thread>
#include <iostream>

std::mutex m;
int counter = 0;

void add() {
    for (int i = 0; i < 100000; ++i) {
        m.lock();
        ++counter;
        m.unlock();
    }
}

int main() {
    std::thread t1(add), t2(add);
    t1.join(); t2.join();
    std::cout << counter << "\n";  // 200000
}
```

**Critical section** = code between lock/unlock. Keep it **small**.  
Prefer RAII locks (`lock_guard` / `unique_lock`) over raw `lock`/`unlock` (exception-safe).

---

## 5. `mutex::try_lock`

Non-blocking attempt. Returns `true` if lock acquired.

```cpp
std::mutex m;

void try_work() {
    if (m.try_lock()) {
        // do work
        m.unlock();
    } else {
        // do something else / retry later
    }
}
```

Use when you'd rather not block (UI, opportunistic updates). Still prefer RAII when you succeed.

---

## 6. `std::try_lock`

Tries to lock **multiple** Lockable objects **atomically** (all or none).  
Returns `-1` on success, or **0-based index** of the first mutex it failed to lock (others unlocked).

```cpp
#include <mutex>

std::mutex m1, m2;

void f() {
    int r = std::try_lock(m1, m2);
    if (r == -1) {
        // both locked — unlock manually (or use adopt_lock + unique_lock)
        m1.unlock();
        m2.unlock();
    } else {
        // failed on mutex index r
    }
}
```

Better pattern with `std::lock` + `unique_lock` / `lock_guard` + `adopt_lock` (see §14).

---

## 7. Timed Mutex

`std::timed_mutex` / `std::recursive_timed_mutex`: try lock with timeout.

```cpp
#include <mutex>
#include <chrono>

std::timed_mutex tm;

void worker() {
    using namespace std::chrono_literals;
    if (tm.try_lock_for(100ms)) {
        // work
        tm.unlock();
    }
    if (tm.try_lock_until(std::chrono::steady_clock::now() + 50ms)) {
        tm.unlock();
    }
}
```

Useful to avoid unbounded waits / detect contention.

---

## 8. Recursive Mutex

Same thread can lock multiple times; must unlock same number of times.

```cpp
std::recursive_mutex rm;

void a() {
    std::lock_guard<std::recursive_mutex> g(rm);
    // may call b() which also locks rm
}

void b() {
    std::lock_guard<std::recursive_mutex> g(rm);
}
```

**Prefer redesign** (single lock at top) over recursive mutex — harder to reason about, slightly slower.

---

## 9. `lock_guard`

RAII wrapper: locks in ctor, unlocks in dtor. **Cannot** unlock early, **cannot** move.

```cpp
std::mutex m;
int shared = 0;

void safe_inc() {
    std::lock_guard<std::mutex> lock(m);
    ++shared;  // auto unlock even if exception
}
```

**Default choice** for simple critical sections.

---

## 10. `unique_lock`

Flexible RAII lock: can defer, try, timed, unlock/relock, move, use with condition variables.

```cpp
std::mutex m;

void demo() {
    std::unique_lock<std::mutex> lk(m, std::defer_lock);  // not locked yet
    lk.lock();
    // ...
    lk.unlock();   // early unlock OK
    lk.lock();

    std::unique_lock<std::mutex> lk2(std::move(lk));  // transferable
}
```

| Policy | Meaning |
|--------|---------|
| (default) | lock immediately |
| `std::defer_lock` | don't lock |
| `std::try_to_lock` | try_lock |
| `std::adopt_lock` | assume already locked |

**Need CV or deferred lock → `unique_lock`. Else → `lock_guard` (C++17: `scoped_lock`).**

---

## 11. Condition Variable

Wait for a condition without busy-spinning.

```cpp
#include <condition_variable>
#include <queue>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;
bool done = false;

void producer() {
    for (int i = 0; i < 5; ++i) {
        {
            std::lock_guard<std::mutex> lk(m);
            q.push(i);
        }
        cv.notify_one();
    }
    {
        std::lock_guard<std::mutex> lk(m);
        done = true;
    }
    cv.notify_one();
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []{ return !q.empty() || done; });  // wait + predicate (handles spurious wakeups)
        while (!q.empty()) {
            int x = q.front(); q.pop();
            lk.unlock();
            // process x
            lk.lock();
        }
        if (done && q.empty()) break;
    }
}
```

**Rules:**
1. Always wait with a **predicate** (spurious wakeups).
2. Wait needs `unique_lock` (releases mutex while waiting).
3. `notify_one` / `notify_all` after changing shared state (usually while holding mutex or right after).

---

## 12. Deadlock

**Deadlock:** two+ threads each hold a lock the other needs → all wait forever.

```cpp
// BAD: opposite lock order
std::mutex m1, m2;

void t1() {
    std::lock_guard<std::mutex> a(m1);
    std::lock_guard<std::mutex> b(m2);
}
void t2() {
    std::lock_guard<std::mutex> a(m2);  // opposite order
    std::lock_guard<std::mutex> b(m1);
}
```

**Prevention (Coffman):**
1. Always acquire locks in a **global fixed order**
2. Use `std::lock` / `std::scoped_lock` (deadlock-avoiding multi-lock)
3. Avoid nested locks when possible
4. Timeouts (`timed_mutex`) as last resort
5. Lock hierarchy / try-and-backoff

---

## 13. Thread / Process Synchronization

**Goal:** coordinate access to shared resources; establish **happens-before**.

| Tool | Use |
|------|-----|
| Mutex / locks | mutual exclusion |
| Condition variable | wait for event |
| Atomic (`std::atomic`) | lock-free flags/counters |
| Future/promise | one-shot result handoff |
| Barriers (C++20 `std::barrier`) | wait for N threads |

**Memory visibility:** without sync, writes may not be visible to other threads (compiler/CPU reordering). Mutex/atomic provide ordering guarantees.

```cpp
std::atomic<bool> ready{false};
int data = 0;

void producer() {
    data = 42;                 // A
    ready.store(true, std::memory_order_release);  // B
}
void consumer() {
    while (!ready.load(std::memory_order_acquire)) {}
    // data == 42 guaranteed
}
```

---

## 14. `std::lock`

Locks multiple mutexes **without deadlock** (internal algorithm).

```cpp
std::mutex m1, m2;

void safe() {
    std::lock(m1, m2);
    std::lock_guard<std::mutex> g1(m1, std::adopt_lock);
    std::lock_guard<std::mutex> g2(m2, std::adopt_lock);
    // critical section
}

// C++17 — preferred
void safer() {
    std::scoped_lock lock(m1, m2);  // locks both, deadlock-free, RAII
}
```

---

## 15. `std::promise` / `std::future`

One-shot channel: producer sets value/exception; consumer waits via future.

```cpp
#include <future>
#include <thread>

void compute(std::promise<int> p) {
    try {
        p.set_value(21 * 2);
    } catch (...) {
        p.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    std::thread t(compute, std::move(prom));  // promise is move-only
    int result = fut.get();  // blocks until set; get() once only
    t.join();
}
```

| Type | Role |
|------|------|
| `promise` | set value/exception |
| `future` | get / wait |
| `shared_future` | multiple waiters |

---

## 16. `std::async`

Higher-level task launch; returns `std::future`. Internally manages promise.

```cpp
#include <future>
#include <iostream>

int fib(int n) { return n <= 1 ? n : fib(n-1) + fib(n-2); }

int main() {
    // Launch policies:
    // std::launch::async    → new thread
    // std::launch::deferred → lazy, runs on wait/get in calling thread
    // async | deferred      → implementation chooses (default)

    auto f = std::async(std::launch::async, fib, 30);
    std::cout << f.get() << "\n";
}
```

**Caveat:** if you ignore the returned future, destructor of `future` from `async` with `async` policy **blocks** until task finishes. Store/get futures intentionally.

**Prefer** thread pools / task systems in production over spawning unbounded `async` threads.

---

## 17. Data Race vs Race Condition

| Term | Meaning |
|------|---------|
| **Data race** | Concurrent access to non-atomic, at least one write, no sync → **UB in C++** |
| **Race condition** | Correctness depends on timing (logic bug); may not be UB |

```cpp
int x = 0;
// Thread A: x = 1;
// Thread B: std::cout << x;   // DATA RACE if no sync
```

**Fix:** mutex, or `std::atomic`, or don't share.

---

## 18. `std::atomic` & Memory Orders

```cpp
#include <atomic>

std::atomic<int> counter{0};
counter.fetch_add(1);                 // default: sequential consistency
counter.fetch_add(1, std::memory_order_relaxed);

std::atomic<bool> ready{false};
int data = 0;

void producer() {
    data = 42;  // non-atomic payload
    ready.store(true, std::memory_order_release);
}
void consumer() {
    while (!ready.load(std::memory_order_acquire)) { /* spin */ }
    // synchronized-with producer release → data == 42 visible
}
```

| Order | Use in interviews |
|-------|-------------------|
| `seq_cst` | Default; simplest; strongest global order |
| `release` / `acquire` | Publish / consume data patterns |
| `acq_rel` | RMW that both acquire & release |
| `relaxed` | Counters only; **no** sync of other data |

**Also know:** `compare_exchange_weak/strong` for lock-free CAS loops.

```cpp
std::atomic<int> val{0};
int expected = 0;
while (!val.compare_exchange_weak(expected, 42)) {
    // expected updated with current value on failure
}
```

---

## 19. `std::shared_mutex` (Reader–Writer Lock)

Multiple readers **or** one writer. C++17.

```cpp
#include <shared_mutex>
#include <unordered_map>

class ThreadSafeMap {
public:
    std::optional<int> get(const std::string& k) const {
        std::shared_lock lock(m_);           // shared = read
        if (auto it = map_.find(k); it != map_.end()) return it->second;
        return std::nullopt;
    }
    void set(std::string k, int v) {
        std::unique_lock lock(m_);           // unique = write
        map_[std::move(k)] = v;
    }
private:
    mutable std::shared_mutex m_;
    std::unordered_map<std::string, int> map_;
};
```

**When:** read-heavy caches. **When not:** short critical sections (mutex may be faster).

---

## 20. `std::call_once` / `once_flag`

Run initialization exactly once across threads (thread-safe lazy init).

```cpp
#include <mutex>

std::once_flag flag;
std::unique_ptr<Widget> widget;

void init() {
    std::call_once(flag, []{
        widget = std::make_unique<Widget>();
    });
}
```

Alternative: Meyers singleton (`static` local). Prefer these over hand-rolled double-checked locking.

---

## 21. `thread_local`

Each thread gets its own instance.

```cpp
thread_local int tls_id = 0;

void worker(int id) {
    tls_id = id;   // no race on tls_id across threads
}
```

Use: per-thread caches, scratch buffers, RNGs. Lifetime: until thread exits.

---

## 22. `std::packaged_task`

Wraps callable → linked `future`. Good for thread pools.

```cpp
#include <future>

std::packaged_task<int(int)> task([](int x){ return x * x; });
std::future<int> fut = task.get_future();
std::thread t(std::move(task), 7);   // task is move-only
std::cout << fut.get() << "\n";      // 49
t.join();
```

---

## 23. Bounded Producer–Consumer (full pattern)

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

template<typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t cap) : cap_(cap) {}

    void push(T item) {
        std::unique_lock lock(m_);
        not_full_.wait(lock, [&]{ return q_.size() < cap_ || closed_; });
        if (closed_) throw std::runtime_error("closed");
        q_.push(std::move(item));
        not_empty_.notify_one();
    }

    bool pop(T& out) {
        std::unique_lock lock(m_);
        not_empty_.wait(lock, [&]{ return !q_.empty() || closed_; });
        if (q_.empty()) return false;  // closed & drained
        out = std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard lock(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }
private:
    size_t cap_;
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable not_empty_, not_full_;
    bool closed_{false};
};
```

**Interview talking points:** backpressure (bounded), spurious wakeups (predicate), shutdown via `close`.

---

## 24. Thread Pool (must-code)

```cpp
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t n) {
        workers_.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this]{
                while (true) {
                    std::function<void()> job;
                    {
                        std::unique_lock lock(m_);
                        cv_.wait(lock, [&]{ return stop_ || !jobs_.empty(); });
                        if (stop_ && jobs_.empty()) return;
                        job = std::move(jobs_.front());
                        jobs_.pop();
                    }
                    job();
                }
            });
        }
    }

    template<class F>
    void submit(F&& f) {
        {
            std::lock_guard lock(m_);
            jobs_.emplace(std::forward<F>(f));
        }
        cv_.notify_one();
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(m_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex m_;
    std::condition_variable cv_;
    bool stop_{false};
};
```

**Extensions interviewers ask:** return `future` via `packaged_task`, bounded queue, graceful drain vs discard.

---

## 25. False Sharing

Two atomics/vars on the **same cache line** (typically 64 bytes) written by different cores → cache-line ping-pong → huge slowdown.

```cpp
struct alignas(64) PaddedCounter {
    std::atomic<int> value{0};
};
PaddedCounter counters[4];  // each on its own cache line
```

**Symptom:** parallel code scales poorly; fix by padding / sharding.

---

## 26. Spinlock vs Mutex

| | Spinlock | Mutex |
|-|----------|-------|
| Wait | busy-loop | sleep / park |
| Best for | tiny critical sections, low contention | longer waits, I/O |
| Cost | burns CPU | syscall / context switch |

```cpp
// conceptual — prefer std::mutex in apps
std::atomic_flag flag = ATOMIC_FLAG_INIT;
while (flag.test_and_set(std::memory_order_acquire)) { /* spin */ }
// critical section
flag.clear(std::memory_order_release);
```

---

## 27. `std::jthread` (C++20)

Auto-joins on destruction + cooperative cancellation via `stop_token`.

```cpp
#include <thread>
#include <stop_token>

void worker(std::stop_token st) {
    while (!st.stop_requested()) {
        // work...
    }
}

{
    std::jthread t(worker);  // dtor requests stop + joins
}
```

Mention if asked about modern cancellation; many codebases still on C++17 `thread`.

---

## 28. Parallel Algorithms (C++17)

```cpp
#include <execution>
#include <algorithm>

std::sort(std::execution::par, v.begin(), v.end());
std::for_each(std::execution::par_unseq, v.begin(), v.end(), [](int& x){ x *= 2; });
```

Policies: `seq`, `par`, `par_unseq`. Don’t access shared non-atomic data unsafely inside.

---

## 29. Classic Interview Problems Checklist

| Problem | Tools |
|---------|-------|
| Thread-safe counter | `mutex` or `atomic` |
| Bounded queue | mutex + 2 CVs |
| Readers–writers | `shared_mutex` |
| Lazy init | `call_once` / Meyers singleton |
| Deadlock bank transfer | `scoped_lock(a,b)` |
| Parallel sum | threads / `async` / parallel algos |
| Thread pool | queue + CV + workers |
| Print in order / barrier | CV or C++20 `latch`/`barrier` |
| Dining philosophers | lock order / `scoped_lock` / hierarchy |

---

## 30. Concurrency Design Tips (C++ roles)

1. **Prefer less sharing** — shard by key, thread-local, message passing  
2. **Keep critical sections tiny** — compute outside the lock  
3. **Never call unknown code under lock** — deadlock / priority inversion  
4. **Document happens-before** — who publishes, who consumes  
5. **Test with TSan** — `-fsanitize=thread`  
6. **Production:** thread pool > unbounded `std::thread` spam  

```bash
g++ -std=c++17 -g -O1 -fsanitize=thread -pthread app.cpp -o app
```
