# Modern C++ — Learning Notes

Easy-to-follow notes for C++ developer interviews. Prefer **C++17** in answers; mention C++20 when useful.  
Pair with: [revision-cheat-sheet.md](revision-cheat-sheet.md) · [cpp-11-17-20-comparison](../cpp-standards/cpp-11-17-20-comparison.md)

---

## 1. What “modern C++” means

Write code that is:
1. **Safe by default** — RAII, no raw owning pointers
2. **Clear ownership** — `unique_ptr` / values / references
3. **Efficient** — moves, no unnecessary copies
4. **Expressive** — STL algorithms, lambdas, `auto` where it helps

**Avoid as default:** bare `new`/`delete`, C-style arrays for ownership, macros for constants.

---

## 2. RAII (most important idea)

**Resource Acquisition Is Initialization:** acquire resource in constructor, release in destructor. Scope exit (including exceptions) cleans up.

```cpp
#include <mutex>
#include <fstream>
#include <memory>

void example(std::mutex& m) {
    std::lock_guard<std::mutex> lock(m);   // lock now, unlock at end of scope
    std::ifstream in("data.txt");          // close file at end of scope
    auto buf = std::make_unique<int[]>(1024); // delete[] at end of scope
}  // all released here, even if exception thrown
```

**Why interviews love it:** Exception safety + no leaked locks/FDs/memory.

**You implement RAII when:** wrapping sockets, `FILE*`, GPU handles, mmap, custom locks.

---

## 3. Ownership model

Ask for every pointer/reference: **who owns it?**

| Tool | Ownership | Use when |
|------|-----------|----------|
| Value (`T obj`) | Automatic | Default for most types |
| `unique_ptr<T>` | Sole owner | Polymorphic / heap object, one owner |
| `shared_ptr<T>` | Shared | Truly shared lifetime (costly) |
| `weak_ptr<T>` | Non-owning observer | Break `shared_ptr` cycles |
| Raw `T*` / `T&` | Non-owning | Borrow; never `delete` |
| `string_view` / `span` | Non-owning view | Temporary read of contiguous data |

**Rule:** Owning raw pointer ≈ code smell in modern C++.

---

## 4. Smart pointers (deep)

### 4.1 `unique_ptr` — default choice

```cpp
#include <memory>

struct Widget { int id; explicit Widget(int i) : id(i) {} };

auto a = std::make_unique<Widget>(1);   // preferred
// auto b = a;                          // ERROR: not copyable
auto b = std::move(a);                  // OK: transfer ownership
// a is now empty (nullptr)

void take(std::unique_ptr<Widget> p);   // callee takes ownership
take(std::move(b));

void borrow(const Widget& w);           // non-owning
borrow(*a);  // only if a non-null
```

**Custom deleter** (e.g. FILE*):
```cpp
auto close_file = [](FILE* f) { if (f) fclose(f); };
std::unique_ptr<FILE, decltype(close_file)> fp(fopen("a.txt", "r"), close_file);
```

**Array:**
```cpp
auto arr = std::make_unique<int[]>(10);  // delete[]
```

### 4.2 `shared_ptr` — use sparingly

Control block holds **strong refcount** (+ weak count). Copy increments; dtor decrements; at 0 deletes object.

```cpp
auto p = std::make_shared<Widget>(2);  // one alloc for object+control block
auto q = p;                            // refcount 2
```

**Cost:** atomic refcount ops, possible cache traffic, harder to reason about lifetime.  
**Prefer** `unique_ptr` or values unless sharing is required.

### 4.3 `weak_ptr` — break cycles

Classic cycle: `Parent` ↔ `Child` both `shared_ptr` → never freed.

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;   // weak edge breaks cycle
};

std::weak_ptr<Widget> w = p;
if (auto s = w.lock()) {
    // s is shared_ptr; object still alive
} else {
    // expired
}
```

### 4.4 Interview favorites
- Why `make_shared` / `make_unique`? Exception safety + (shared) one allocation.
- Can `unique_ptr` be stored in container? Yes.
- Passing `shared_ptr` by value vs `const&` — by value shares ownership; prefer `const&` or raw/borrow if not extending life.

---

## 5. Move semantics

**Problem:** Copying big objects (vector, string) is expensive.  
**Move:** steal resources from a temporary / explicitly moved object.

```cpp
std::vector<int> make_big() {
    std::vector<int> v(1'000'000);
    return v;   // NRVO or move — cheap
}

std::vector<int> a = make_big();
std::vector<int> b = std::move(a);  // a valid but unspecified (usually empty)
```

### 5.1 rvalue references
```cpp
void sink(std::string&& s);     // only binds to temporary / moved
std::string x = "hi";
// sink(x);                     // error
sink(std::move(x));             // OK — you promise not to use x meaningfully
sink(std::string{"tmp"});       // OK
```

### 5.2 Rule of thumb for `std::move`
- Use when transferring ownership into a sink / container / another object.
- After move: object is **valid but unspecified** — don’t read old content; may reassign.
- Don’t `std::move` something you still need.
- Moving `const` objects falls back to copy.

### 5.3 Move constructor / assignment
```cpp
class Buffer {
public:
    Buffer(Buffer&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr;
        o.size_ = 0;
    }
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_; size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }
    ~Buffer() { delete[] data_; }
private:
    char* data_{nullptr};
    size_t size_{0};
};
```

**`noexcept` on moves** matters: `vector` reallocation only moves if noexcept; else may copy.

---

## 6. Rule of 0 / 3 / 5

| Rule | Meaning |
|------|---------|
| **0** | Don’t write special members; use RAII members (`string`, `vector`, `unique_ptr`) |
| **3** | If you manage a raw resource: destructor + copy ctor + copy assign |
| **5** | Also move ctor + move assign |

```cpp
// Rule of 0 — preferred
class Person {
    std::string name_;
    std::vector<int> ids_;
    std::unique_ptr<Widget> w_;
};
```

If you `= delete` copy, usually still provide/define moves or `= default` them carefully.

---

## 7. Value categories & perfect forwarding (interview level)

Simplified:
- **lvalue** — has a name / identity (`int x;`)
- **rvalue** — temporary (`x + 1`, `std::move(x)`)

**Forwarding reference** (in templates):
```cpp
template<typename T>
void wrapper(T&& arg) {              // not "rvalue ref" — forwarding ref
    actual(std::forward<T>(arg));    // preserve lvalue/rvalue-ness
}
```

- `T&&` + deduced `T` → forwarding reference  
- `std::forward<T>` → cast back to original value category  
Use in generic factories / emplace-style APIs.

---

## 8. `const` correctness

```cpp
void print(const std::string& s);     // won't modify; no copy
int Widget::id() const { return id_; } // promises not to modify *this

const int* p;   // pointee const
int* const q;   // pointer const
const int* const r;
```

**Mutable:** rare — for mutexes inside logically const methods / caches.
```cpp
mutable std::mutex m_;
int get() const {
    std::lock_guard lock(m_);
    return value_;
}
```

---

## 9. `constexpr` & compile-time

```cpp
constexpr int sq(int x) { return x * x; }
constexpr int k = sq(4);   // compile-time

// C++17: more liberally constexpr; C++20: even dynamic alloc in constexpr in limited forms
```

Interview: “work done at compile time when args are constant expressions.”

---

## 10. `noexcept`

Marks function as non-throwing. Enables optimizations; `vector` moves prefer noexcept moves.

```cpp
void f() noexcept;
void g() noexcept(noexcept(h()));  // conditional
```

Don’t mark `noexcept` if you might throw — `std::terminate` on throw.

---

## 11. Lambdas

```cpp
int factor = 2;
auto mul = [factor](int x) { return x * factor; };      // capture by value
auto add = [&](int x) { return x + factor; };           // capture all by ref (careful!)
auto own = [v = std::vector<int>{1,2}]( ) { return v.size(); }; // init-capture (C++14)

std::vector<int> a{3,1,2};
std::sort(a.begin(), a.end(), [](int x, int y){ return x > y; });
```

**Pitfall:** capturing locals by ref then storing lambda past scope → dangling.

Generic lambda (C++14): `[](auto x){ return x; }`

---

## 12. References, views, lifetimes

```cpp
// OK
const std::string& f();           // if returns ref to stable storage

// DANGLING — UB
std::string_view bad() {
    std::string tmp = "hello";
    return tmp;                   // view into destroyed string
}
std::string_view sv = std::string("tmp");  // temporary dies at end of statement
```

**`string_view` / `span`:** non-owning. Great as **function parameters**, dangerous as **stored members** unless lifetime guaranteed.

---

## 13. STL essentials (modern use)

### Prefer algorithms over raw loops when clear
```cpp
#include <algorithm>
#include <numeric>

std::sort(v.begin(), v.end());
auto it = std::find(v.begin(), v.end(), 42);
int sum = std::accumulate(v.begin(), v.end(), 0);
```

### Iterator invalidation (must know)
| Container | Invalidates when |
|-----------|------------------|
| `vector` | reallocation; erase invalidates at/after point |
| `deque` | insert/erase extremes special-cased; middle invalidates |
| `list` | only erased element |
| `unordered_*` | rehash invalidates all iterators |

```cpp
std::vector<int> v{1,2,3};
int* p = &v[0];
v.push_back(4);   // may reallocate → p dangling
```

### `reserve` when size known
```cpp
std::vector<int> v;
v.reserve(n);
```

---

## 14. `optional`, `variant`, `string_view` (C++17)

```cpp
#include <optional>
#include <variant>
#include <string_view>

std::optional<int> parse(std::string_view s);  // empty = failure

std::variant<int, std::string> v = 42;
std::visit([](auto&& x){ /* ... */ }, v);
```

Use `optional` instead of magic `-1` / null pointers for “maybe a value.”

---

## 15. Structured bindings & if-init (C++17)

```cpp
std::map<int, std::string> m;
if (auto [it, ok] = m.insert({1, "a"}); ok) {
    // inserted
} else {
    // existed
}
```

---

## 16. Templates (interview depth)

```cpp
template<typename T>
T maxv(const T& a, const T& b) {
    return a < b ? b : a;
}

// specialization vs overload — know the difference exists
template<>
const char* maxv(const char* const& a, const char* const& b) = delete; // example
```

**CRTP / SFINAE / Concepts:** overview enough for many SDE1 roles; deeper for generic-library teams.  
C++20 concepts = readable constraints on `T`.

---

## 17. Undefined behavior (UB)

Common C++ UB:
1. Data race  
2. Use-after-free / dangling ref/view  
3. Out-of-bounds access  
4. Signed integer overflow  
5. Strict aliasing violations  
6. Invalidated iterator use  
7. Double free / delete mismatch (`new[]` / `delete`)  

**Tools:**
```bash
g++ -std=c++17 -g -fsanitize=address,undefined,thread -pthread main.cpp
```

---

## 18. Exception safety (levels)

| Guarantee | Meaning |
|-----------|---------|
| No-throw | Never throws (`noexcept`) |
| Strong | Failed op has no effect (rollback) |
| Basic | No leaks / invariants hold; state may change |
| None | Avoid |

RAII is how you get **basic** safety easily.

---

## 19. Casting (prefer safest)

| Cast | Use |
|------|-----|
| None / implicit | Ideal |
| `static_cast` | Related types, explicit numeric |
| `dynamic_cast` | Polymorphic downcast (needs RTTI) |
| `const_cast` | Remove const — rare, dangerous |
| `reinterpret_cast` | Bit pattern — last resort |

Avoid C-style `(T)x` in new code.

---

## 20. Mental checklist before submitting code

1. Who owns each resource?  
2. Any raw `new`? → `make_unique` / container  
3. Can I `const` this?  
4. Large object passed by value? → `const&` or move  
5. Views stored? → lifetime OK?  
6. Threads sharing data? → sync  
7. Exceptions? → RAII held locks/resources  

---

## Mini practice

1. Rewrite a class using raw `new`/`delete` → Rule of 0 + `unique_ptr`  
2. Implement move-only `Buffer`  
3. Fix a dangling `string_view` bug  
4. Explain why `shared_ptr` to `this` needs `enable_shared_from_this`  
5. Show iterator invalidation with `vector::push_back`
