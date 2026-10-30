/* 
    Features: concepts and requires, defaulted <=>, std::span, consteval / constinit, 
        designated initializers, std::format with a custom std::formatter.
    Build: a TimingSample struct ({.run = 3, .micros = 1840, .cold = true}) with 
        defaulted <=>, a std::formatter so it prints nicely, and a 
        summarize(std::span<const TimingSample>) function. Write a concept 
        Measurable (has .micros convertible to double) and constrain a generic 
        mean() with it. Build a small lookup table (for example, microsecond-to-millisecond 
        bucket labels) with a consteval function and store it constinit.
    Done when: sorting a vector of samples works with no handwritten comparison, passing 
        a type that isn't Measurable gives a short, readable error, and a static_assert proves 
        the table is built at compile time.
    Stretch: a requires expression that checks for a .label() member and uses it in formatting 
        when present.
*/

#include <array>
#include <concepts>
#include <format>
#include <iostream>
#include <random>
#include <span>

template<class T>
concept Measurable = requires (T obj) {
    { obj.micros } -> std::convertible_to<double>;
};

struct BucketLabel {
    int micros;
    std::string_view label;
};

consteval std::array<BucketLabel, 3> make_bucket_lookup() {
    return {
        BucketLabel { 1'000, "<1ms" },
        BucketLabel { 10'000, "<10ms" },
        BucketLabel { 100'000, "<100ms" },
    };
}

constinit auto micro_to_milli = make_bucket_lookup();
static_assert(make_bucket_lookup()[0].micros == 1000);

constexpr std::string_view bucket_label(int millis) {
    if (millis < 1000)
        return micro_to_milli[0].label;
    else if (millis < 10'000)
        return micro_to_milli[1].label;
    else if (millis < 100'000)
        return micro_to_milli[2].label;
    return ">=100ms";
}

struct TimingSample {
    int run;
    int micros;
    bool cold { true };

    auto operator<=> (const TimingSample&) const = default;
};

// specialize a formatter
template <>
struct std::formatter<TimingSample> {
    constexpr auto parse (std::format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format (const TimingSample& sample, std::format_context& ctx) const {
        return std::format_to (ctx.out(), "TimingSample: run={:<4} micros={:<7} bucket: {:<7} cold={}",
            sample.run, sample.micros, bucket_label(sample.micros), sample.cold);
    }
};

namespace {
    TimingSample random_sample() {
        std::random_device device;
        std::mt19937 engine { device() };
        std::uniform_int_distribution<int> gen1 { 1, 9999 };
        std::uniform_int_distribution<int> gen2 { 99, 9999999 };
        return {
            gen1(engine),
            gen2(engine),
            true
        };
    }

    template<class Col>
    double mean (const Col& col) {
        static_assert (Measurable<typename Col::value_type>, "T needs a .micros convertible to double");
        double total = 0.0;
        for (const auto& e : col)
            total += static_cast<double> (e.micros);
        return total / static_cast<double> (col.size());
    }

    void summarize (std::span<TimingSample> span) {
        std::cout << std::format ("Timings total={} mean={}\n", span.size(), mean(span));
    }
}

int main() {
    std::vector<TimingSample> samples;
    for (int i = 0; i < 100; ++i)
        samples.emplace_back (random_sample());
    std::sort (samples.begin(), samples.end());
    for (const auto& sample : samples)
        std::cout << std::format ("{}\n", sample);
    std::cout << std::endl;
    summarize (samples);
    return 0;
}
