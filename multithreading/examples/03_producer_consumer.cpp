// g++ -std=c++17 -pthread 03_producer_consumer.cpp -o pc && ./pc
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;
bool done = false;
constexpr int CAP = 5;

void producer() {
    for (int i = 1; i <= 10; ++i) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [] { return static_cast<int>(q.size()) < CAP; });
        q.push(i);
        std::cout << "produced " << i << "\n";
        lk.unlock();
        cv.notify_all();
    }
    {
        std::lock_guard<std::mutex> lk(m);
        done = true;
    }
    cv.notify_all();
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [] { return !q.empty() || done; });
        if (q.empty() && done) break;
        int x = q.front();
        q.pop();
        lk.unlock();
        cv.notify_all();
        std::cout << "consumed " << x << "\n";
    }
}

int main() {
    std::thread p(producer), c(consumer);
    p.join();
    c.join();
}
