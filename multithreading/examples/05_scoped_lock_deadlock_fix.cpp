// g++ -std=c++17 -pthread 05_scoped_lock_deadlock_fix.cpp -o dl && ./dl
#include <iostream>
#include <mutex>
#include <thread>

std::mutex m1, m2;

void transfer_ab() {
    for (int i = 0; i < 10000; ++i) {
        std::scoped_lock lock(m1, m2);  // deadlock-free
        // critical section
    }
}

void transfer_ba() {
    for (int i = 0; i < 10000; ++i) {
        std::scoped_lock lock(m2, m1);  // order doesn't matter with scoped_lock
    }
}

int main() {
    std::thread t1(transfer_ab), t2(transfer_ba);
    t1.join();
    t2.join();
    std::cout << "ok\n";
}
