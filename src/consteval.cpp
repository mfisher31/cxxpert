
#include <format>
#include <iostream>

consteval int square (int n) {
    return n * n;
}

constinit int sq7 { square (3) };

int main() {
    std::cout << std::format ("the square of seven is {}\n", sq7);
    return 0;
}
