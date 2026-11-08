#include <array>
#include <format>
#include <iostream>
#include <ostream>
#include <random>

template <std::uniform_random_bit_generator G>
int roll_die (G& gen) {
    std::uniform_int_distribution<int> dist { 1, 6 };
    return dist (gen);
}

void exercise_1_a() {
    std::array<int, 6> data {};
    std::random_device rd;
    std::mt19937 gen { rd() };
    for (int i = 0; i < 60000; ++i) {
        auto r = roll_die (gen) - 1;
        ++data[r];
    }

    for (int i = 0; i < 6; ++i) {
        auto v = i + 1;
        std::cout << v << "=" << data[i] << "\n";
    }

    std::flush (std::cout);
}

void exercise_1_b() {
    std::array<int, 12> data {};
    std::random_device rd;
    std::mt19937 gen { rd() };

    for (int i = 0; i < 60000; ++i) {
        auto r1 = roll_die (gen) - 1;
        auto r2 = roll_die (gen) - 1;
        ++data[r1 + r2];
    }

    for (int i = 10; i >= 0; --i) {
        auto didx    = i + 2;
        auto nblocks = data[i] / 200;

        std::cout << std::format ("[{:2}] {:5} {:#<{}}\n", didx, data[i], "", nblocks);
    }

    std::flush (std::cout);
}

void exercise_2() {
    std::mt19937 mt1 { 11 };
    std::mt19937 mt2 { 11 };
    std::uniform_int_distribution<int> dist { 100, 999 };
    for (int i = 0; i < 10; ++i) {
        if (dist (mt1) != dist (mt2)) {
            std::cerr << "same seed different value\n";
            break;
        }
    }

    std::mt19937 mt3 { std::random_device {}() };
    for (int i = 0; i < 10; ++i) {
        std::cout << "rand=" << dist (mt3) << std::endl;
    }
}

void exercise_3() {
    // skipped. already done in #1
}

void exercise_4() {
}

int main() {
    exercise_1_a();
    exercise_1_b();
    exercise_2();
    exercise_3();
    exercise_4();
    return 0;
}
