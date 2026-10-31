
#include <format>
#include <iostream>
#include <string>

struct LineItem {
    std::string name;
    int quantity;
    double price;
};

auto line_items() {
    const std::vector<LineItem> items {
        { "A Book", 3, 19.99 },
        { "Dog Toy", 10, 4.99 },
        { "Dog Treat", 20, 12.99 }
    };
    return items;
}

void print_item (const LineItem& item) {
    const double total = item.quantity * item.price;
    std::cout << std::format ("{:12} {:>6.2f} {:4} {:6.2f}\n", item.name, item.price, item.quantity, total);
}

void print_items() {
    for (const auto& item : line_items())
        print_item (item);
    std::cout << std::endl;
}

int main() {
    print_items();
    return 0;
}
