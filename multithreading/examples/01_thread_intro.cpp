// g++ -std=c++17 -pthread 01_thread_intro.cpp -o intro && ./intro
#include <iostream>
#include <thread>
#include <vector>

void work(int id) {
    std::cout << "thread " << id << " id=" << std::this_thread::get_id() << "\n";
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i)
        threads.emplace_back(work, i);

    for (auto& t : threads)
        if (t.joinable()) t.join();
}
