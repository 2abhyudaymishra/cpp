# C++11 vs C++17 vs C++20 — Interview Comparison

Focus: features C++ developer interviews actually ask about.

---

## Quick timeline

| Standard | Theme | Interview weight |
|----------|-------|------------------|
| **C++11** | Modern C++ foundation | **Must know** |
| **C++14** | Polish on 11 | Nice to know |
| **C++17** | Practical everyday upgrades | **Must know** |
| **C++20** | Big language leap | Strong plus (know headlines) |

---

## Side-by-side (high impact)

| Area | C++11 | C++17 | C++20 |
|------|-------|-------|-------|
| **Auto / type** | `auto`, `decltype`, trailing return | Class template arg deduction (CTAD), `auto` in non-type template params (partial) | Concepts constrain templates |
| **Smart ptrs** | `unique_ptr`, `shared_ptr`, `weak_ptr` | `make_unique` (was C++14), `shared_ptr` for arrays | `atomic<shared_ptr>` improvements |
| **Move** | Move ctor/assign, rvalue refs, `std::move` | Guaranteed copy elision (prvalues) | More refinements |
| **Lambda** | Basic lambdas, capture | Capture `*this`, `constexpr` lambda | Template lambdas, pack expansions |
| **Concurrency** | `thread`, `mutex`, `cv`, `future`, `atomic` | `shared_mutex`, `scoped_lock` | `jthread`, `stop_token`, `latch`, `barrier`, `semaphore` |
| **Optional/err** | — | `optional`, `variant`, `any`, `string_view` | `expected` not std yet (23); better constexpr |
| **Filesystem** | Boost-era | `<filesystem>` | Stable |
| **Algo** | `<algorithm>` | Parallel algorithms (`execution::par`) | Ranges (`std::ranges`) |
| **Init** | Uniform init `{}` | Structured bindings, `if/switch` init | Designated initializers |
| **Templates** | Variadic templates | Fold expressions | Concepts, requires |
| **Modules** | Headers only | — | Modules |
| **Coroutines** | — | — | Coroutines (`co_await`) |
| **Constexpr** | Limited | Much more (incl. lambda) | Almost unrestricted (`constexpr` new/virtual) |

---

## C++11 — must-know list

```cpp
// auto + range-for
for (const auto& x : vec) {}

// nullptr (not NULL/0)
void f(int*); f(nullptr);

// move semantics
std::vector<int> a = std::move(b);

// smart pointers
auto p = std::make_shared<Widget>();
auto u = std::unique_ptr<Widget>(new Widget);  // prefer make_unique (C++14)

// lambda
auto add = [](int a, int b) { return a + b; };

// threads
std::thread t(fn); t.join();

// enum class
enum class Color { Red, Green };

// delete / default
Widget(const Widget&) = delete;
Widget() = default;

// delegating / inheriting ctors, override/final
struct D : B { void f() override; };
```

**Interview staples:** RAII, Rule of 0/3/5, move vs copy, `unique_ptr` vs `shared_ptr`, data races.

---

## C++14 (brief)

- Generic lambdas: `[](auto x){ return x; }`
- `std::make_unique`
- Relaxed constexpr
- `[[deprecated]]`

Usually folded into “modern C++” answers; rarely tested alone.

---

## C++17 — must-know list

```cpp
// structured bindings
auto [it, ok] = map.insert({k, v});

// if with initializer
if (auto it = m.find(k); it != m.end()) { use(it->second); }

// string_view — non-owning view (don't store dangling)
void f(std::string_view sv);

// optional / variant
std::optional<int> parse(const std::string&);
std::variant<int, std::string> v;

// filesystem
namespace fs = std::filesystem;
fs::path p = "a/b"; fs::exists(p);

// scoped_lock — multi-mutex RAII
std::scoped_lock lock(m1, m2);

// shared_mutex
std::shared_lock read(sm);
std::unique_lock write(sm);

// CTAD
std::pair p{1, 2.0};          // pair<int,double>
std::lock_guard g(m);         // lock_guard<mutex>

// fold expressions
template<class... T>
auto sum(T... xs) { return (xs + ...); }

// inline variables (header-friendly)
struct S { static inline int count = 0; };
```

**Also:** `std::byte`, `std::invoke`, `std::apply`, parallel STL (`std::execution::par`).

---

## C++20 — know the headlines + 1 example each

### Concepts
```cpp
template<typename T>
concept Addable = requires(T a, T b) { a + b; };

template<Addable T>
T add(T a, T b) { return a + b; }
```

### Ranges
```cpp
#include <ranges>
for (int x : vec | std::views::filter([](int n){ return n > 0; })
                 | std::views::transform([](int n){ return n * n; })) {}
```

### `jthread` + stop token
```cpp
void worker(std::stop_token st) {
    while (!st.stop_requested()) { /* ... */ }
}
std::jthread t(worker);  // auto join + cooperative cancel
```

### Coroutines (conceptual)
```cpp
// co_await / co_yield / co_return — async generators, lazy tasks
// Interviews: know what they solve; deep impl rarely required for SDE1
```

### Other C++20
- Modules (`import std;`) — tooling still maturing
- Calendar/timezone (`<chrono>`)
- `span` — contiguous view (`std::span<int>`)
- `constexpr` containers improvements
- Three-way comparison `<=>` (spaceship)
- Designated init: `Point p{.x=1, .y=2};`

---

## Concurrency feature map

| Feature | Since |
|---------|-------|
| `std::thread`, `mutex`, `cv`, `future` | C++11 |
| `std::shared_timed_mutex` | C++14 |
| `std::shared_mutex`, `scoped_lock` | C++17 |
| Parallel algorithms | C++17 |
| `jthread`, `latch`, `barrier`, `counting_semaphore` | C++20 |
| `std::atomic_ref` | C++20 |

---

## What to say in interviews

**“What's modern C++?”**  
→ RAII, smart pointers, move semantics, STL algorithms, avoid raw `new`/`delete`, prefer `std::` concurrency primitives, value semantics where possible.

**“11 vs 17?”**  
→ 11 introduced modern core; 17 adds daily ergonomics (`optional`, `string_view`, structured bindings, `scoped_lock`) without changing the object model.

**“When 20?”**  
→ Concepts/ranges for cleaner generic code; `jthread` for safer threads; coroutines for async pipelines (domain-dependent).

---

## Compile flags (mention confidently)

```bash
g++ -std=c++11 ...
g++ -std=c++17 ...
g++ -std=c++20 ...
# Clang same: -std=c++20
```

For C++ roles: default to **C++17** in coding rounds unless told otherwise; mention C++20 features when relevant.
