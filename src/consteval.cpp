#include <iostream>

consteval int square(int n) {
    return n * n;
}

int main() {
    constexpr int result = square(7);
    std::cout << result << '\n';
    return 0;
}
