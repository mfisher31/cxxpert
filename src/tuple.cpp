#include <charconv>
#include <concepts>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

// Exercise 1
std::tuple<bool, int, std::string> parse (std::string_view str) {
    using namespace std::string_literals;
    int value       = 0;
    auto result     = std::from_chars (str.data(), str.data() + str.size(), value);
    auto ok         = result.ec == std::errc {} && result.ptr == str.data() + str.size();
    std::string err = ok ? "" : "could not parse string into an int";
    if (! ok)
        value = 0;
    return std::tie (ok, value, err);
}

void exercise_1() {
    auto [ok, value, err] = parse ("42");
    std::cout << std::format ("{0} {1} {2}\n", ok, value, err);

    std::tie (ok, value, err) = parse ("abc");
    std::cout << std::format ("{0} {1} {2}\n", ok, value, err);

    std::tie (ok, value, err) = parse ("12abc");
    std::cout << std::format ("{0} {1} {2}\n", ok, value, err);
}

// Exercise 2
void exercise_2_a() {
    int a, b;
    auto rhs        = std::make_pair (1, 2);
    std::tie (a, b) = rhs;
    std::cout << std::format ("{} {}\n", a, b);

    rhs.second                = 111;
    std::tie (std::ignore, b) = rhs;
    std::cout << std::format ("b={}\n", b);
}

void exercise_2_b() {
    struct Person {
        std::string lname;
        std::string fname;
        int age;

        // bool operator< (const Person& o) const {
        //     return std::tie(lname, fname, age)
        //         < std::tie(o.lname, o.fname, o.age);
        // }
        // clang-format off
        auto operator<=> (const Person&) const = default;
        // clang-format on
    };

    Person me { "Michael", "Fisher", 44 };
    Person you { "John", "Doe", 25 };
    bool res = you < me;
    std::cout << std::format ("I'm last? {}\n", res);
}

// Exercise 3
template <typename F, typename... Args>
requires std::invocable<F&, Args&...>&& std::is_void_v<std::invoke_result_t<F, Args...>> class VoidCaller {
public:
    VoidCaller (F caller, Args... args)
        : _caller (std::move (caller)),
          _args (std::move (args)...) {}

    void call() {
        std::apply (_caller, _args);
    }

private:
    F _caller;
    std::tuple<std::decay_t<Args>...> _args;
};

void exercise_3() {
    auto printvals = [] (int a, int b) -> void {
        std::cout << std::format ("a={} b={}\n", a, b);
    };
    VoidCaller ints (printvals, 1, 2);
    ints.call();
}

// Exercise 4
void exercise_4() {
    using T1 = decltype (std::make_tuple (3, std::string ("hello")));
    static_assert (std::is_same_v<T1, std::tuple<int, std::string>>, "T1 not the same");
    static_assert (std::is_same_v<std::tuple_element_t<1, T1>, std::string>, "not a string");
    T1 tpl { 100, "world" };

    std::cout << std::get<std::string> (tpl) << std::endl;
}

// Exercise 5
template <class Tpl>
void print_tuple (const Tpl& tpl) {
    bool first   = true;
    auto printer = [&] (auto& e) {
        if (! first)
            std::cout << ", ";
        std::cout << e;
        first = false;
    };

    auto helper = [&] (const auto&... xs) {
        (printer (xs), ...);
    };

    std::cout << "(";
    std::apply (helper, tpl);
    std::cout << ")" << std::endl;
}

void exercise_5() {
    auto tpl1 = std::make_tuple (1, 3.3, std::string ("hello there"));
    print_tuple (tpl1);
    print_tuple (std::make_tuple (1, 123.23, "hi there", 5u));
}

int main() {
    exercise_1();
    exercise_2_a();
    exercise_2_b();
    exercise_3();
    exercise_4();
    exercise_5();
    return 0;
}
