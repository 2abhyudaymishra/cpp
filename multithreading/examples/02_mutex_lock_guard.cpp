// g++ -std=c++17 -pthread 02_mutex_lock_guard.cpp -o mtx && ./mtx
#include <iostream>
#include <mutex>
#include <thread>

std::mutex m;
int counter = 0;

void add(int n) {
    for (int i = 0; i < n; ++i) {
        std::lock_guard<std::mutex> lock(m);
        ++counter;
    }
}

int main() {
    std::thread t1(add, 100000), t2(add, 100000);
    t1.join();
    t2.join();
    std::cout << "counter=" << counter << "\n";  // expect 200000
}
