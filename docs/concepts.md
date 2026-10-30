# Concepts and `requires`

Header: `<concepts>` for the standard library's ready-made concepts. The language feature itself needs no header. C++20.

Examples were compiled with `clang++ -std=c++20 -Wall -Wextra -Wpedantic`, and the error messages quoted below are what clang printed.

Reference: [Constraints and concepts](https://en.cppreference.com/w/cpp/language/constraints) · [requires expressions](https://en.cppreference.com/w/cpp/language/requires) · [library concepts](https://en.cppreference.com/w/cpp/concepts)

---

# Part 1: Basics

## 1.1 The problem concepts solve

A plain template accepts any type, then fails deep inside the function body when the type doesn't support what the body does. The error shows up far from the real mistake, and often as pages of text.

A **concept** gives a requirement a name. A **constraint** attaches it to a template. A bad type is then rejected at the call, and the error says which requirement it failed.

## 1.2 Defining a concept

A concept is a named compile-time `bool`:

```cpp
template <class T>
concept Name = constraint-expression;
```

The constraint expression can be:
- A type trait: `std::is_integral_v<T>`
- Another concept: `std::integral<T>`
- A `requires` expression (see 1.4)
- Any of those joined with `&&` or `||`

Using it is just `Name<SomeType>`, which is `true` or `false`. So you can test one directly with `static_assert(Name<int>);`.

## 1.3 Four ways to apply a concept

All four mean the same thing for simple cases. Pick whichever reads best.

| Style | Looks like |
|---|---|
| Constrained type parameter | `template <Measurable T> double mean(...)` |
| `requires` clause after the template header | `template <class T> requires Measurable<T> double mean(...)` |
| Trailing `requires` after the function declarator | `template <class T> double mean(...) requires Measurable<T>` |
| Abbreviated function template | `double mean(Measurable auto const& x)` (each `auto` is its own anonymous template parameter) |

Notes:
- In a `requires` clause, a bare concept name or a parenthesized expression is fine. Anything more complex (like a call) needs parentheses.
- Join conditions with `&&` and `||`. They are checked left to right and stop at the first failure.

## 1.4 `requires` expressions

A `requires` expression is a `bool` that is `true` if everything inside it would compile:

```cpp
requires (parameter list) {
    requirement;
    requirement;
}
```

The parameter list declares pretend variables to use in the requirements. Nothing runs; it is only checked. There are four kinds of requirement:

| Kind | Form | Checks that... |
|---|---|---|
| Simple | `expr;` | the expression is valid |
| Type | `typename T::value_type;` | the type exists |
| Compound | `{ expr } -> concept;` | the expression is valid **and** its type satisfies a concept. Add `noexcept` after the braces to also require it not to throw. |
| Nested | `requires condition;` | an extra boolean condition holds |

Example of a made-up concept (not the one you need) that shows the compound form:

```cpp
template <class T>
concept HasSize = requires (const T& t) {
    { t.size() } -> std::convertible_to<std::size_t>;
};
```

Read the compound requirement as: "`t.size()` must compile, and its result must be convertible to `size_t`."

How the `->` part works: the type that gets tested is `decltype((expr))`, so an lvalue expression gives a reference type. That's why you use `std::convertible_to<...>` (which handles references) rather than `std::same_as<...>` (which doesn't) for most checks.

## 1.5 Standard library concepts

Don't write a concept for something the standard already has. In `<concepts>`:

| Group | Concepts |
|---|---|
| Type relationships | `same_as`, `derived_from`, `convertible_to`, `common_with` |
| Type categories | `integral`, `signed_integral`, `unsigned_integral`, `floating_point` |
| Object properties | `movable`, `copyable`, `default_initializable`, `constructible_from`, `regular`, `semiregular` |
| Comparison | `equality_comparable`, `totally_ordered` |
| Callables | `invocable`, `regular_invocable`, `predicate` |

Others live in the headers that own them: `std::ranges::range` and friends in `<ranges>`, iterator concepts in `<iterator>`, and `std::uniform_random_bit_generator` in `<random>`.

## 1.6 Overload resolution: more constrained wins

If two templates are both viable and one has a stricter constraint, the compiler picks the stricter one, with no ambiguity. This only works when the constraint is built from **named concepts** (or the same atomic expressions), not from two separately written boolean expressions that merely mean the same thing.

## 1.7 Concepts in `if constexpr`

A `requires` expression is just a `bool`, so it works as an `if constexpr` condition. That lets one function adapt to what a type supports:

```cpp
template <class T>
std::string describe (const T& t) {
    if constexpr (requires { t.label(); })
        return std::format("{} [{}]", t.micros, t.label());
    else
        return std::format("{}", t.micros);
}
```

**Important catch:** this only works inside a **template**, where `t` has a dependent type. In a normal function, the discarded branch is still compiled, so a missing `label()` is a hard error. I tested this: a non-template function with the same shape fails with `no member named 'label' in 'S'`.

## 1.8 Reading the error message

Given a constrained function and a type that doesn't satisfy it, clang says:

```
note: candidate template ignored: constraints not satisfied [with T = N]
note: because 'N' does not satisfy 'Measurable'
note: because 't.micros' would be invalid: no member named 'micros' in 'N'
```

Three lines, naming the concept and the exact failing expression. That's the "short, readable error" your warmup asks for. It stays short when the concept expresses the requirement directly.

## 1.9 Common mistakes

| Mistake | Fix |
|---|---|
| Using `same_as` in a compound requirement for an lvalue | The tested type is a reference. Use `convertible_to`, or add the reference to the type you compare. |
| Using the `requires`-in-`if constexpr` trick in a non-template function | Move the logic into a function template. |
| Writing the same condition twice instead of naming it | Define a concept. You get better errors and subsumption. |
| Forgetting that `T` may not deduce through a conversion | See 2.5 below. |
| Writing a concept so broad it accepts everything | Test it with `static_assert(!Name<BadType>)` too. |

---

# Part 2: What the warmup file asks for

The comment at the top of [src/00_cxx20_warmup.cpp](../src/00_cxx20_warmup.cpp) lists the features and the requirements. Here is where the file stands, and what each remaining piece needs.

## 2.1 Status

| Requirement | Status |
|---|---|
| `TimingSample` with defaulted `<=>` | Done |
| Sorting a vector with no handwritten comparison | Done (see the include note below) |
| `std::formatter<TimingSample>` | Works, with one correctness fix (below) |
| Designated initializers | Not used yet |
| `summarize(std::span<const TimingSample>)` | Not started |
| `Measurable` concept | Not started |
| Constrained `mean()` | Not started |
| `consteval` table stored `constinit` | Not started |
| `static_assert` that the table is compile-time | Not started |
| Stretch: `.label()` detection in formatting | Not started |

**Two things to fix in what you have:**
- **Includes.** The file uses `std::vector` and `std::sort` but doesn't include `<vector>` or `<algorithm>`. It builds only because `<random>` happens to pull in `<vector>` for you. Add both. Add `<span>` when you get to `summarize`.
- **The formatter returns the wrong iterator.** `std::format_to(out, ...)` returns the iterator past what it wrote, and you throw it away and return the original `out`. It happens to work with this standard library, but the correct pattern is to return `format_to`'s result directly. Doing so also removes the need for the `out` variable. See section 2.1 of [format.md](format.md).

## 2.2 Designated initializers

**What:** initialize an aggregate by field name, as in `TimingSample{.run = 3, .micros = 1840, .cold = true}`. See [reference](https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers).

**Rules (stricter than C):**
- Fields must appear in **declaration order**.
- You can omit trailing fields. An omitted field takes its default member initializer, so `.cold` becomes `true` if you leave it out.
- You can't mix named and positional initializers in the same list.

**To do:** convert `random_sample()` from positional to designated, and use designated initializers when building samples in `main`.

## 2.3 `std::span` and `summarize`

[`std::span<T>`](https://en.cppreference.com/w/cpp/container/span) is a non-owning view of a contiguous sequence: a pointer and a length.

- Implicitly constructs from a `std::vector`, `std::array`, or C array, so one function accepts all of them.
- `std::span<const T>` means the function can read but not modify.
- Pass it **by value**. It's two words.
- Useful members: `size()`, `empty()`, `operator[]`, `begin()`/`end()`, `front()`/`back()`, `first(n)`, `last(n)`, `subspan(off, n)`.
- It doesn't own anything. The underlying data must outlive the span.

**What `summarize` should do** isn't specified in the comment. A reasonable design: return a small struct, or print, with the count, the minimum, the maximum, the mean, and how many samples were cold. Handle the empty span.

**To do:** write `summarize` taking `std::span<const TimingSample>` and call it from `main` with the vector.

## 2.4 The `Measurable` concept

**Requirement from the comment:** "has `.micros` convertible to double."

That is the compound-requirement form from 1.4: pretend a `const T&` exists, require that `.micros` on it is valid, and require its type to satisfy `std::convertible_to<double>`. Look at the `HasSize` example in 1.4 and swap in your requirement.

**Check it with `static_assert`:**
- `Measurable<TimingSample>` should be true.
- A struct that has no `.micros`, such as one with only `ms`, should be false.
- A struct whose `.micros` is a `std::string` should be false (not convertible to `double`).

## 2.5 Constrained `mean()`

**Requirements:** a generic `mean()` that only accepts `Measurable` types.

**Decisions:**
- **Constraint style:** use the constrained type parameter form from 1.3.
- **Parameter type:** a `std::span<const T>` is natural, but there's a catch. Template argument deduction does **not** apply conversions. Calling `mean(vec)` with a `std::vector<TimingSample>` can't deduce `T` from the span, so you have to write `mean<TimingSample>(vec)`. I compiled this to confirm. Options:
  - Accept the explicit template argument.
  - Take a `const std::vector<T>&`.
  - Take any range and constrain it, using [`std::ranges::range`](https://en.cppreference.com/w/cpp/ranges/range) and checking the element type with `Measurable`. This is the most general.
- **Return type:** `double`.
- **Empty input:** decide what to do (return 0, or require non-empty). Dividing by `size()` of zero is not an option.
- **Summing:** accumulate in `double`, or in a wider integer, to avoid overflowing `int`. [`std::accumulate`](https://en.cppreference.com/w/cpp/algorithm/accumulate) or a loop both work.

**Done-when check:** call `mean` with a type that isn't `Measurable` and confirm the error is the short three-line kind from 1.8, not pages of template errors.

## 2.6 `consteval` table stored `constinit`

Three keywords that sound alike:

| Keyword | Meaning |
|---|---|
| [`constexpr`](https://en.cppreference.com/w/cpp/language/constexpr) | May run at compile time **or** runtime. On a variable: it is `const` and usable in constant expressions. |
| [`consteval`](https://en.cppreference.com/w/cpp/language/consteval) | A function that **must** run at compile time. Any call that isn't a constant expression is an error. |
| [`constinit`](https://en.cppreference.com/w/cpp/language/constinit) | A static or thread-local variable whose initializer must be a constant expression. It guarantees no runtime initialization, which avoids the static initialization order problem. It does **not** make the variable `const`. |

**Building the table:**
- Make a small struct for one bucket: an upper bound in microseconds and a label.
- The container: [`std::array`](https://en.cppreference.com/w/cpp/container/array) of those. A `std::vector` or `std::string` can't be stored as a compile-time constant in C++20, because their allocations can't survive to runtime.
- Labels: use `std::string_view` pointing at string literals.
- A `consteval` function builds and returns the array.
- The variable: `constinit` plus `const` (or just `constinit` if you want it mutable). I compiled `constinit const auto table = build();` and it works.

**Writing the `static_assert`:** this is the tricky part. A `constinit const` variable of class type is **not** usable in constant expressions, so `static_assert(table[0].upper == 1000)` fails with "read of non-constexpr variable". I confirmed this. Assert on the `consteval` function's result instead, for example `static_assert(build_table()[1].label == "<10ms")`. That call can only compile if it is evaluated at compile time, which is the proof you want. If you'd rather assert on the variable itself, declare it `constexpr` instead of `constinit`.

**Lookup:** write a small function that takes microseconds and returns the label of the first bucket whose upper bound is not exceeded. The last bucket's bound can be the largest `int`, so every value matches something.

## 2.7 Stretch: `.label()` detection

**Requirement:** if the type has a `.label()` member, include it in the formatted output.

**How:** the `if constexpr (requires { t.label(); })` pattern from 1.7. Because of the template-only catch, you can't put that directly in `TimingSample`'s non-template `format`. Options:
- Write a function template, taking any `T`, that does the check and returns the formatted text. Call it from the formatter.
- Make the formatter itself a constrained partial specialization over `Measurable` types: `template <Measurable T> struct std::formatter<T>`. This formats all measurable types at once.

To test it, you need a second type that has `.label()` and `.micros`, since `TimingSample` has no label yet.

## 2.8 Done-when checklist (from the comment)

- [ ] Sorting a vector of samples works with no handwritten comparison.
- [ ] Passing a type that isn't `Measurable` gives a short, readable error.
- [ ] A `static_assert` proves the table is built at compile time.

---

# Part 3: Exercises

1. Write a concept `Addable` that requires `a + b` to be valid and have the same type as `a`, and test it with `static_assert` on `int`, `std::string`, and a struct with no `operator+`.
2. Write the same function three times, once for each of the first three styles in 1.3, and confirm they behave identically.
3. Write two overloads of a function, one for `std::integral` and one for `std::floating_point`, and call it with `int`, `double`, and `std::string` to see which compile.
4. Write an overload pair where one is constrained by a concept and the other by a stricter concept built from it. Show that the stricter one wins with no ambiguity.
5. Use a compound requirement with `noexcept` to make a concept that only accepts types whose `swap` doesn't throw.
6. Reproduce the "non-template `if constexpr`" error from 1.7, then fix it by making the function a template.
7. Finish every item in section 2 and check off the list in 2.8.
