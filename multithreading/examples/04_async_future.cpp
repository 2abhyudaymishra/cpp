// g++ -std=c++17 -pthread 04_async_future.cpp -o fut && ./fut
#include <future>
#include <iostream>
#include <numeric>
#include <vector>

long long sum_range(const std::vector<int>& v, size_t lo, size_t hi) {
    return std::accumulate(v.begin() + static_cast<long>(lo),
                           v.begin() + static_cast<long>(hi), 0LL);
}

int main() {
    std::vector<int> v(1'000'000, 1);
    size_t mid = v.size() / 2;

    auto f1 = std::async(std::launch::async, sum_range, std::cref(v), 0, mid);
    auto f2 = std::async(std::launch::async, sum_range, std::cref(v), mid, v.size());

    std::cout << "sum=" << f1.get() + f2.get() << "\n";
}
