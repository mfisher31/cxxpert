#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>

bool integer (std::string_view input, int64_t& output) {
    auto res = std::from_chars (input.begin(), input.end(), output);
    return res.ec == std::errc {} && res.ptr == input.begin() + input.size();
}

bool decimal (std::string_view input, double& output) {
    auto res = std::from_chars (input.begin(), input.end(), output);
    return res.ec == std::errc {} && res.ptr == input.begin() + input.size() && std::isfinite (output);
}

int main() {
    for (std::string_view str : { "42", "55.5", "2", "inf" }) {
        int64_t out = 0;
        if (integer (str, out)) {
            std::cout << "int=" << out << std::endl;
        } else {
            std::cerr << "error parsing integer\n";
        }
    }

    std::cout << std::endl;

    for (std::string_view str : { "42", "55.5", "2", "inf", "hello world" }) {
        double out = 0;
        if (decimal (str, out)) {
            std::cout << "out=" << std::fixed << std::setprecision (4) << out << std::endl;
        } else {
            std::cerr << "error parsing decimal\n";
        }
    }
}
