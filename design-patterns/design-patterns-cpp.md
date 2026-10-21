# Design Patterns in C++ (by Interview Frequency)

Ordered **most asked → less asked** for C++ developer roles.  
Each: intent → when → C++ example → interview tip.

---

## Tier 1 — Almost every C++ round

### 1. Singleton

**Intent:** Exactly one instance; global access point.  
**Asked because:** thread-safety, lifetime, and “why maybe avoid it.”

```cpp
// Meyers Singleton (C++11) — thread-safe init of function-local static
class Logger {
public:
    static Logger& instance() {
        static Logger inst;   // magic static
        return inst;
    }
    void log(const std::string& s) { /* ... */ }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
private:
    Logger() = default;
};

// use: Logger::instance().log("hi");
```

**Interview tips:**
- Prefer Meyers over raw pointer + double-checked locking (easy to get wrong pre-C++11).
- Cons: hidden dependency, hard to test/mock, lifetime order across TUs.
- Often better: pass dependency explicitly (DI).

---

### 2. Factory Method / Simple Factory

**Intent:** Create objects without exposing concrete types to the caller.

```cpp
#include <memory>
#include <string_view>

struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
};
struct Circle : Shape {
    double r;
    explicit Circle(double r) : r(r) {}
    double area() const override { return 3.14159 * r * r; }
};
struct Rect : Shape {
    double w, h;
    Rect(double w, double h) : w(w), h(h) {}
    double area() const override { return w * h; }
};

std::unique_ptr<Shape> make_shape(std::string_view type) {
    if (type == "circle") return std::make_unique<Circle>(1.0);
    if (type == "rect")   return std::make_unique<Rect>(2.0, 3.0);
    return nullptr;
}
```

**Interview tips:** Return `unique_ptr<Base>`; factory can later read config/plugin.  
**Abstract Factory** = family of related products (UIButton/UIWindow for Win vs Mac) — mention if asked “difference.”

---

### 3. Observer

**Intent:** Notify many dependents when subject changes (events, UI, market data).

```cpp
#include <functional>
#include <vector>
#include <algorithm>

class Subject {
public:
    using Callback = std::function<void(int)>;
    void subscribe(Callback cb) { observers_.push_back(std::move(cb)); }
    void set(int v) {
        value_ = v;
        for (auto& cb : observers_) cb(value_);
    }
private:
    int value_{0};
    std::vector<Callback> observers_;
};

// Subject s; s.subscribe([](int v){ std::cout << v; }); s.set(42);
```

**Interview tips:** Lifetime of observers (dangling callbacks) — use `weak_ptr` or unregister. Threading: notify under/after lock carefully (don’t hold lock into user code).

---

### 4. Strategy

**Intent:** Swap algorithms at runtime behind a common interface.

```cpp
struct CompressStrategy {
    virtual ~CompressStrategy() = default;
    virtual std::vector<char> compress(const std::vector<char>&) = 0;
};
struct Gzip : CompressStrategy { /* ... */ };
struct Lz4  : CompressStrategy { /* ... */ };

class Compressor {
public:
    explicit Compressor(std::unique_ptr<CompressStrategy> s) : s_(std::move(s)) {}
    void set(std::unique_ptr<CompressStrategy> s) { s_ = std::move(s); }
    auto run(const std::vector<char>& in) { return s_->compress(in); }
private:
    std::unique_ptr<CompressStrategy> s_;
};
```

**Modern C++ alternative:** `std::function` or template policy (compile-time strategy).  
**vs State:** Strategy = interchangeable algorithm; State = behavior changes with internal state.

---

### 5. RAII (C++ idiom — treated as a pattern in interviews)

**Intent:** Bind resource lifetime to object lifetime (lock, FD, memory).

```cpp
class File {
public:
    explicit File(const char* path) : f_(std::fopen(path, "r")) {
        if (!f_) throw std::runtime_error("open failed");
    }
    ~File() { if (f_) std::fclose(f_); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& o) noexcept : f_(o.f_) { o.f_ = nullptr; }
    FILE* get() const { return f_; }
private:
    FILE* f_;
};
```

**Interview tip:** Foundation of modern C++; `lock_guard`, `unique_ptr`, `fstream` are RAII.

---

### 6. Pimpl (Pointer to Implementation)

**Intent:** Hide private details → stable ABI, faster compiles, encapsulation.

```cpp
// widget.h
class Widget {
public:
    Widget();
    ~Widget();                      // need out-of-line dtor
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;
    void draw();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// widget.cpp
struct Widget::Impl { int data; void draw_impl(); };
Widget::Widget() : impl_(std::make_unique<Impl>()) {}
Widget::~Widget() = default;
void Widget::draw() { impl_->draw_impl(); }
```

**Interview tips:** Incomplete type in header → dtor must be in .cpp. Move ops usually needed. Cost: extra heap alloc + indirection.

---

## Tier 2 — Very common

### 7. Adapter

**Intent:** Make incompatible interface usable (wrap legacy API).

```cpp
class OldPrinter { public: void print_old(const char* s); };

class IPrinter {
public:
    virtual ~IPrinter() = default;
    virtual void print(const std::string& s) = 0;
};

class PrinterAdapter : public IPrinter {
public:
    explicit PrinterAdapter(OldPrinter& p) : p_(p) {}
    void print(const std::string& s) override { p_.print_old(s.c_str()); }
private:
    OldPrinter& p_;
};
```

---

### 8. Decorator

**Intent:** Add behavior dynamically without changing original class.

```cpp
struct Stream {
    virtual ~Stream() = default;
    virtual void write(const std::string&) = 0;
};
struct FileStream : Stream {
    void write(const std::string& s) override { /* write file */ }
};
struct EncryptedStream : Stream {
    explicit EncryptedStream(std::unique_ptr<Stream> inner) : inner_(std::move(inner)) {}
    void write(const std::string& s) override {
        auto enc = encrypt(s);
        inner_->write(enc);
    }
private:
    std::unique_ptr<Stream> inner_;
    static std::string encrypt(const std::string& s) { return s; /* ... */ }
};
```

**vs Inheritance:** Decorator composes wrappers; can stack (compress + encrypt).

---

### 9. Command

**Intent:** Encapsulate a request as an object (undo, queues, macros).

```cpp
struct Command {
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};
struct TextEditor { std::string text; };
struct InsertCmd : Command {
    TextEditor& ed; std::string chunk; size_t pos;
    void execute() override { ed.text.insert(pos, chunk); }
    void undo() override { ed.text.erase(pos, chunk.size()); }
};
```

---

### 10. Builder

**Intent:** Step-by-step construction of complex objects (many optional params).

```cpp
class HttpRequest {
public:
    class Builder {
    public:
        Builder& url(std::string u) { url_ = std::move(u); return *this; }
        Builder& method(std::string m) { method_ = std::move(m); return *this; }
        Builder& header(std::string k, std::string v) {
            headers_.emplace(std::move(k), std::move(v)); return *this;
        }
        HttpRequest build() { return HttpRequest{std::move(url_), std::move(method_), std::move(headers_)}; }
    private:
        std::string url_, method_{"GET"};
        std::map<std::string, std::string> headers_;
    };
private:
    HttpRequest(std::string u, std::string m, std::map<std::string, std::string> h)
        : url_(std::move(u)), method_(std::move(m)), headers_(std::move(h)) {}
    std::string url_, method_;
    std::map<std::string, std::string> headers_;
};

// auto req = HttpRequest::Builder().url("https://x").method("POST").build();
```

**vs telescoping ctors:** Builder scales with optional fields. In C++ also consider aggregate init / designated init (C++20) for simpler cases.

---

### 11. Template Method

**Intent:** Algorithm skeleton in base; subclasses override steps.

```cpp
class DataMiner {
public:
    void mine() {           // template method — not C++ template!
        open();
        auto data = extract();
        analyze(data);
        close();
    }
    virtual ~DataMiner() = default;
protected:
    virtual void open() = 0;
    virtual std::string extract() = 0;
    virtual void analyze(const std::string&) = 0;
    virtual void close() {}
};
```

---

## Tier 3 — Asked sometimes

### 12. Proxy

**Intent:** Surrogate controlling access (lazy load, remote, access control).

```cpp
struct Image {
    virtual ~Image() = default;
    virtual void display() = 0;
};
struct RealImage : Image {
    explicit RealImage(std::string path) { /* heavy load */ }
    void display() override { /* draw */ }
};
struct ImageProxy : Image {
    explicit ImageProxy(std::string path) : path_(std::move(path)) {}
    void display() override {
        if (!real_) real_ = std::make_unique<RealImage>(path_);
        real_->display();
    }
private:
    std::string path_;
    std::unique_ptr<RealImage> real_;
};
```

---

### 13. Visitor

**Intent:** Add operations over a type hierarchy without modifying types (double dispatch).

```cpp
struct Circle;
struct Rect;
struct Visitor {
    virtual void visit(Circle&) = 0;
    virtual void visit(Rect&) = 0;
    virtual ~Visitor() = default;
};
struct Shape {
    virtual void accept(Visitor&) = 0;
    virtual ~Shape() = default;
};
struct Circle : Shape {
    void accept(Visitor& v) override { v.visit(*this); }
};
```

**Interview tip:** Powerful but verbose; in modern C++ often replaced by `std::variant` + `std::visit`.

---

### 14. State

**Intent:** Object behavior changes when internal state changes.

```cpp
struct ConnState {
    virtual ~ConnState() = default;
    virtual void connect(class Connection&) = 0;
    virtual void disconnect(Connection&) = 0;
};
class Connection {
public:
    void change(std::unique_ptr<ConnState> s) { state_ = std::move(s); }
    void connect() { state_->connect(*this); }
private:
    std::unique_ptr<ConnState> state_;
};
```

---

### 15. Bridge

**Intent:** Decouple abstraction from implementation so both can vary independently.

```cpp
struct Renderer { virtual void draw_circle(double) = 0; virtual ~Renderer() = default; };
struct VectorRenderer : Renderer { void draw_circle(double r) override; };
struct Shape {
    explicit Shape(std::unique_ptr<Renderer> r) : r_(std::move(r)) {}
    virtual void draw() = 0;
protected:
    std::unique_ptr<Renderer> r_;
};
struct Circle : Shape {
    double radius;
    void draw() override { r_->draw_circle(radius); }
};
```

---

### 16. Flyweight

**Intent:** Share intrinsic state across many objects (memory).

```cpp
// Shared glyph metrics / texture keyed by character
struct Glyph { char ch; /* metrics */ };
class GlyphFactory {
public:
    Glyph& get(char c) {
        auto [it, _] = pool_.try_emplace(c, Glyph{c});
        return it->second;
    }
private:
    std::unordered_map<char, Glyph> pool_;
};
```

---

## Tier 4 — Nice to know / senior C++

### 17. CRTP (Curiously Recurring Template Pattern)

**Intent:** Static polymorphism — no vtable.

```cpp
template<typename Derived>
struct Base {
    void interface() { static_cast<Derived*>(this)->impl(); }
};
struct Widget : Base<Widget> {
    void impl() { /* ... */ }
};
```

Used in: expression templates, mixins, policy-based design.

---

### 18. Chain of Responsibility

**Intent:** Pass request along a chain of handlers until one handles it.

```cpp
struct Handler {
    std::unique_ptr<Handler> next;
    virtual bool handle(int req) {
        if (next) return next->handle(req);
        return false;
    }
    virtual ~Handler() = default;
};
```

---

## Quick compare (interview favorites)

| Pattern | One line |
|---------|----------|
| Singleton | One instance |
| Factory | Create without `new Concrete` at call site |
| Observer | Pub-sub notifications |
| Strategy | Pluggable algorithm |
| RAII | Lifetime = resource |
| Pimpl | Hide impl, stable ABI |
| Adapter | Interface translation |
| Decorator | Wrap to add behavior |
| Command | Request as object |
| Builder | Stepwise construction |

## How to answer any pattern Q
1. Intent (1 sentence)  
2. Tiny C++ sketch (interfaces + ownership with `unique_ptr`)  
3. Trade-off / when not to use  
4. Modern C++ alternative if any (`variant`, `function`, Meyers singleton)
