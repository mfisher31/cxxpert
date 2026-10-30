#include <iostream>

struct Point {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct Line {
    Point p1;
    Point p2;
};

int main() {
    Point p { .x = 1, .z = 3 };
    std::cout << p.x << ", " << p.y << ", " << p.z << '\n';

    Point p2 { 1, 2, 3 };
    std::cout << p2.x << ":" << p2.y << ":" << p2.z << std::endl;

    Point p3 { .x = 1, .y = 100, .z = 3 };
    std::cout << p3.y << std::endl;
    return 0;
}
