#include <format>
#include <iostream>
#include <queue>
#include <random>
#include <vector>

struct Item {
    int num { 0 };
    int priority { 0 };
};

bool by_priority (const Item& a, const Item& b) {
    return a.priority < b.priority;
};

using ItemQueue = std::priority_queue<Item, std::vector<Item>, decltype (&by_priority)>;

int main() {
    std::random_device rd;
    std::mt19937 gen { rd() };
    gen = std::mt19937 { rd() };

    std::uniform_int_distribution<int> rand { 0, 255 };

    ItemQueue queue { by_priority };
    for (int i = 0; i < 100; ++i) {
        queue.push ({ i, rand (gen) });
    }

    while (! queue.empty()) {
        const auto& ref = queue.top();
        std::cout << std::format ("priority={:5} num={}\n", ref.priority, ref.num);
        queue.pop();
    }

    return 0;
}
