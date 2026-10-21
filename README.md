# C++ Developer Interview Prep

Target: **C++ Developer / Systems SDE**.  
Pillars: STL · Modern C++ · Multithreading · Design Patterns · System Design · Linux · Standards.

```
Interview/
├── README.md
├── stl/
│   ├── stl-learning-notes.md      ← containers: complexity + internals + thread safety
│   └── stl-revision-cheat-sheet.md
├── modern-cpp/
│   ├── learning-notes.md
│   └── revision-cheat-sheet.md
├── multithreading/
│   ├── learning-notes.md
│   ├── revision-cheat-sheet.md
│   └── examples/*.cpp
├── design-patterns/
│   └── design-patterns-cpp.md
├── system-design/system-design-cpp.md
├── cpp-standards/cpp-11-17-20-comparison.md
└── linux/linux-interview-guide.md
```

OOPS notes omitted — use your own. Old DSA roadmap replaced by **STL-focused** notes.

---

## Study order

| Priority | Folder | Why |
|----------|--------|-----|
| P0 | `stl/` | Containers/algorithms every C++ round expects |
| P0 | `modern-cpp/` | RAII, ownership, moves |
| P0 | `multithreading/` | Systems / C++ roles |
| P1 | `design-patterns/` | Design rounds |
| P1 | `cpp-standards/` | 11 / 17 / 20 |
| P1 | `linux/` | Processes, tools |
| P1 | `system-design/` | Systems design |

---

## How to use learning vs revision

| Topic | Learn | Revise |
|-------|-------|--------|
| STL | `stl/stl-learning-notes.md` | `stl/stl-revision-cheat-sheet.md` |
| Modern C++ | `modern-cpp/learning-notes.md` | `modern-cpp/revision-cheat-sheet.md` |
| Multithreading | `multithreading/learning-notes.md` | `multithreading/revision-cheat-sheet.md` |
| Patterns | `design-patterns/design-patterns-cpp.md` | Tier-1 section |

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pthread main.cpp -o main
g++ -std=c++17 -g -fsanitize=address,undefined,thread -pthread main.cpp -o main
```
