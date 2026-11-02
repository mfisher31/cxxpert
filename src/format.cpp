
#include <format>
#include <iostream>
#include <iterator>
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

void exercise_1() {
    for (const auto& item : line_items())
        print_item (item);
    std::cout << std::endl;
}

void exercise_2() {
    std::cout << std::format ("{0:d} {0:#x} {0:o} {0:#b}\n", 123);
}

void exercise_3() {
    std::string out;
    std::format_to (std::back_inserter (out), "{} {} {}", 1, 2, 3);
    std::cout << "exercise 3: " << out << std::endl;
}

void exercise_4() {
    char out[16]   = {};
    auto r         = std::format_to_n (out, sizeof (out), "{} {} {} abcdefghijklmnopqrsyz", 1, 2, 3);
    *r.out         = '\0';
    bool truncated = r.size > 16;
    std::cout << std::format ("out={} truncated={}\n", out, truncated);
}

struct Person {
    std::string name;
    int age;
};

template <>
struct std::formatter<Person> : std::formatter<std::string> {
    auto format (const Person& person, std::format_context& ctx) const {
        return std::formatter<std::string>::format (
            std::format ("{} ({})", person.name, person.age), ctx);
    }
};

void exercise_5() {
    std::cout << std::format ("{:>30}\n", Person { "Mike", 44 });
    std::cout << std::format ("{:>30}\n", Person { "Chuck", 60 });
}

enum class Color { Red,
                   Green,
                   Blue };

struct Pixel {
    Color color;
    uint32_t alpha;
};

template <>
struct std::formatter<Color> : std::formatter<std::string_view> {
    auto format (Color c, std::format_context& ctx) const {
        std::string_view text = "";
        switch (c) {
            case Color::Red:
                text = "Red";
                break;
            case Color::Green:
                text = "Green";
                break;
            case Color::Blue:
                text = "Blue";
                break;
        }
        return std::formatter<std::string_view>::format (text, ctx);
    }
};

template <>
struct std::formatter<Pixel> : std::formatter<std::string_view> {
    std::formatter<Color> cf;
    auto format (const Pixel& pixel, std::format_context& ctx) const {
        auto it = cf.format (pixel.color, ctx);
        *it++   = ',';
        *it++   = ' ';
        it      = std::format_to (it, "{:#x}", pixel.alpha);
        return it;
    }
};

void exercise_6() {
    std::cout << std::format ("color = {} {} {}\n",
                              Color::Red,
                              Color::Green,
                              Color::Blue);
    Pixel px { Color::Green, 100 };
    std::cout << std::format ("px = {}\n", px);
}

struct Point {
    double x;
    double y;
};

template <>
struct std::formatter<Point> {
    bool _v = false;
    constexpr auto parse (std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == 'v') {
            _v = true;
            ++it;
        }
        if (it != ctx.end() && *it != '}')
            throw std::format_error ("invalid point format");
        return it;
    }

    auto format (const Point& pt, std::format_context& ctx) const {
        return _v ? std::format_to (ctx.out(), "x={}, y={}", pt.x, pt.y)
                  : std::format_to (ctx.out(), "{}, {}", pt.x, pt.y);
    }
};

void exercise_7() {
    Point pt { 10, 10 };
    std::cout << std::format ("{}\n", pt);
    std::cout << std::format ("{:v}\n", pt);
}

void exercise_8() {
    // uncomment to see compile time format error
    // std::cout << std::format ("{:d}", "hello world");
    try {
        std::cout << std::vformat ("{:d}\n", std::make_format_args ("hello world"));
    } catch (const std::format_error& e) {
        std::cerr << std::format ("error: {}\n", e.what());
    }
}

int main() {
    exercise_1();
    exercise_2();
    exercise_3();
    exercise_4();
    exercise_5();
    exercise_6();
    exercise_7();
    exercise_8();
    return 0;
}
