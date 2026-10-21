# Multithreading in C++

Playlist: [CppNuts — Mulithreading In C++](https://www.youtube.com/playlist?list=PLk6CEY9XxSIAeK-EAh3hB4fgNvYkYmghp)

| Doc | Use |
|-----|-----|
| [learning-notes.md](learning-notes.md) | Learn (playlist §1–16 + interview §17–30) |
| [revision-cheat-sheet.md](revision-cheat-sheet.md) | Day-before flash revision |
| [examples/](examples/) | Runnable demos |

## Topic map

| # | Topic |
|---|-------|
| 1–3 | Threads, create ways, join/detach |
| 4–8 | Mutex, try_lock, timed, recursive |
| 9–10 | lock_guard, unique_lock |
| 11–14 | CV, deadlock, sync, std::lock |
| 15–16 | promise/future, async |
| 17–18 | Data race, atomics / memory order |
| 19–22 | shared_mutex, call_once, thread_local, packaged_task |
| 23–24 | Bounded queue, thread pool |
| 25–30 | False sharing, spinlock, jthread, parallel algos, design tips |

```bash
g++ -std=c++17 -pthread examples/03_producer_consumer.cpp -o /tmp/pc && /tmp/pc
```
