#include <chrono>
#include <iostream>
#include <semaphore>
#include <thread>

constexpr int num = 100;
std::counting_semaphore<20> _sem { 0 };
std::atomic<int> _count { 0 };

static void worker() {
    while (_count.load() < num) {
        _sem.acquire();
        std::cout << "count = " << _count.load() << std::endl;
        std::this_thread::sleep_for (std::chrono::milliseconds (100));
        ++_count;
    }
}

int main() {
    std::thread work_thread (worker);

    for (int i = 0; i < num; ++i) {
        _sem.release (1);
    }

    work_thread.join();
    return 0;
}
