# Modern C++ — Revision Cheat Sheet

Flash revision before interviews. Details: [learning-notes.md](learning-notes.md)

---

## Instant recall

| Topic | Remember |
|-------|----------|
| Modern C++ | RAII + ownership + moves + STL |
| RAII | acquire in ctor, release in dtor |
| Default owning ptr | `unique_ptr` / `make_unique` |
| Shared lifetime | `shared_ptr` (costly) |
| Cycles | `weak_ptr` |
| Non-owning | raw `T*`, `T&`, `string_view`, `span` |
| Move | steal resources; source valid-but-unspecified |
| Rule of 0 | prefer; members manage resources |
| Rule of 5 | dtor + copy + move + copy= + move= |
| Forwarding | `T&&` + `std::forward<T>` in templates |
| Views | great as params; dangerous as stored fields |
| UB | race, UAF, OOB, signed overflow, bad iterators |

---

## Ownership decision tree

```
Need heap / polymorphism?
  no  → store by value
  yes → one owner? → unique_ptr
        shared?    → shared_ptr (+ weak_ptr for observers/cycles)
Borrow only?       → T& / const T& / T* / string_view / span
```

---

## Snippets to memorize

```cpp
auto p = std::make_unique<T>(args...);
auto s = std::make_shared<T>(args...);
if (auto locked = w.lock()) { /* use */ }

std::vector<int> b = std::move(a);
void f(std::string s);  f(std::move(local));

template<class T>
void wrap(T&& x) { sink(std::forward<T>(x)); }

std::lock_guard<std::mutex> g(m);
std::optional<int> o;
auto [it, ok] = map.insert({k,v});
void foo(std::string_view sv);   // param OK
```

---

## Smart pointer Q&A

| Q | A |
|---|---|
| unique vs shared? | Sole vs shared ownership; prefer unique |
| Why make_shared? | One alloc; exception-safe |
| When weak_ptr? | Observe / break cycles |
| Pass unique_ptr how? | `std::move` to transfer; `&`/`get` to borrow |
| shared_ptr cost? | Atomic refcount + indirection |

---

## Move Q&A

| Q | A |
|---|---|
| What does move do? | Transfer resources |
| After move? | Valid but unspecified |
| Why noexcept move? | `vector` can move on reallocation |
| Return local by value? | Yes — NRVO/move |
| move const object? | Copies instead |

---

## Rule of 0/3/5

```
Write no special members if members are RAII        → Rule of 0
Manage raw resource                                 → Rule of 3 (+ prefer 5)
C++11+: also move ctor & move assign                → Rule of 5
```

---

## Lifetime landmines

```cpp
// BAD
std::string_view sv = std::string("x");
const std::string& r = std::string("x");
auto* p = &v[0]; v.push_back(1);  // p may dangle

// GOOD
void f(std::string_view sv);      // callee doesn't store
v.reserve(n);                     // reduce realloc
```

---

## const / constexpr / noexcept

| Keyword | Meaning |
|---------|---------|
| `const` method | won’t modify observable state |
| `constexpr` | can run at compile time |
| `noexcept` | won’t throw (or terminate) |

---

## Lambda captures

| Capture | Meaning |
|---------|---------|
| `[=]` | copy all used |
| `[&]` | ref all used — dangling risk if stored |
| `[a]` / `[&a]` | specific |
| `[v = std::move(x)]` | init-capture |

---

## Iterator invalidation (shortest)

- `vector`: realloc / erase at-or-after  
- `unordered_map`: rehash → all iterators  
- `list`: only erased node  

---

## Casts preference

`implicit` → `static_cast` → `dynamic_cast` → avoid `const_cast` / `reinterpret_cast` / C-casts

---

## Exception safety

RAII ⇒ basic guarantee for free. Strong = commit/rollback style.

---

## Tooling one-liners

```bash
g++ -std=c++17 -Wall -Wextra -Werror
g++ -std=c++17 -g -fsanitize=address,undefined,thread -pthread
```

---

## 60-second verbal answer

> “Modern C++ centers on RAII and clear ownership: prefer values and `unique_ptr`, use `shared_ptr` only when shared lifetime is required, and `weak_ptr` to break cycles. Use moves to avoid copies, keep APIs `const`-correct, pass views carefully so they don’t outlive data, and rely on the STL with awareness of iterator invalidation. Sanitizers catch UB like races and use-after-free.”
