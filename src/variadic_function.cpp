#include <cstddef>
#include <iostream>
#include <type_traits>

// ============================================================================
// VARIADIC TEMPLATES & PARAMETER PACKS — guided exercises
// ============================================================================
// Work through these in order. Each section explains one idea and gives you
// a task. Write the code yourself under each comment block; nothing here is
// implemented for you. Uncomment the calls in main() as you complete each
// part so you can test incrementally.
// ============================================================================

// ----------------------------------------------------------------------------
// PART 1 — The basic syntax: a template parameter pack
// ----------------------------------------------------------------------------
// `template <typename... Ts>` declares a PACK of zero or more types, named Ts.
// `void foo(Ts... args)` declares a matching pack of function parameters,
// named args — one args for each type in Ts.
//
// `...` means different things depending on where it sits:
//   - to the LEFT of a name in a declaration  -> "this is a pack"
//   - to the RIGHT of a pack name (expansion)  -> "unpack me here"
//
// TASK:
//   Write a function template `count_args` that takes a parameter pack of
//   any types and returns how many arguments were passed, using the
//   `sizeof...(Ts)` operator (note: sizeof... is a single operator, distinct
//   from sizeof). It should work for 0, 1, or many arguments of any type.
//
template <typename... Ts>
constexpr size_t count_args (Ts... args) {
    return sizeof...(args);
}

// TEST (uncomment once written):
static_assert (count_args() == 0);
static_assert (count_args (1, 2, 3) == 3);
static_assert (count_args ("a", 2, 3.0, 'x') == 4);

// ----------------------------------------------------------------------------
// PART 2 — Expanding a pack recursively
// ----------------------------------------------------------------------------
// Before C++17 fold expressions existed, the *only* way to "do something"
// with every element of a pack was recursion. "Peel off one argument and
// recurse on the rest" is not a metaphor — here is literally what happens,
// mechanically, at the compiler level.
//
// The recursive signature looks like this:
//
//   template <typename First, typename... Rest>
//   void print_all(First first, Rest... rest)
//
// Template argument DEDUCTION is what does the "peeling." Given a call's
// argument list, the compiler must decide what First and Rest are. First
// binds to exactly one type: whatever the first argument's type is.
// Everything after that gets bundled into the pack Rest. That split happens
// automatically, every single time this template is instantiated.
//
// Trace a call: print_all(1, "two", 3.0, 'f')
//
//   instantiation 1: First=int,         Rest={const char*, double, char}
//                    first=1, rest...="two", 3.0, 'f'
//                    -> print `first`, then call print_all(rest...)
//                    -> rest... expands back into a plain argument list:
//                       print_all("two", 3.0, 'f')   (a brand new call)
//
//   instantiation 2: First=const char*, Rest={double, char}
//                    -> print "two", then call print_all(3.0, 'f')
//
//   instantiation 3: First=double,      Rest={char}
//                    -> print 3.0, then call print_all('f')
//
//   instantiation 4: First=char,        Rest={}      <- empty pack is legal
//                    -> print 'f', then call print_all(rest...)
//                       where rest... expands to NOTHING, i.e. print_all()
//
// The subtlety: at instantiation 4 the template still requires at least one
// parameter (First first), so it CANNOT match a zero-argument call. If the
// recursive template were the only overload in existence, print_all() would
// fail to compile — no matching function. That is exactly why you need a
// separate overload (or specialization) that takes ZERO arguments: it isn't
// there to handle "the first element," it's the only thing capable of
// matching once the pack has bottomed out to nothing. Each "recursive call"
// is really a distinct instantiation of the same template with a smaller
// Rest; the chain only terminates because a *different* function exists
// that the final, empty call can bind to.
//
// TASK:
//   Write `print_all(Args... args)` that prints each argument followed by a
//   space, using the peel-off-the-front recursive pattern described above.
//   Figure out why you need a base-case overload that takes no arguments
//   (or how else you'd stop the recursion).
//

void print_all() {}

template <typename First, typename... Args>
void print_all (First current, Args... args) {
    std::cout << current << " ";
    print_all (args...);
}

void part_2() {
    // TEST:
    print_all (1, "two", 3.0, 'f'); // expect: 1 two 3 f
    std::cout << std::endl;
}

// ----------------------------------------------------------------------------
// PART 3 — Fold expressions (C++17, but essential in C++20 code)
// ----------------------------------------------------------------------------
// A fold expression lets you apply a binary operator ACROSS a pack without
// writing recursion. Syntax forms:
//   (pack op ...)        -- unary right fold
//   (... op pack)        -- unary left fold
//   (pack op ... op init) -- binary right fold
//   (init op ... op pack) -- binary left fold
//
// Example shape (don't copy verbatim, understand the shape):
//   (args + ...)        sums a pack left-to-right-ish via +
//   (... && conditions)  ANDs a pack of bools together
//
// TASK:
//   Rewrite PART 2's `print_all` using a fold expression over the comma
//   operator instead of recursion:
//     (expr, ...)
//   where `expr` does the printing for one `args` element. This is the
//   classic "comma-fold" trick — figure out what `expr` must be so that the
//   fold expands into "print(args1), print(args2), print(args3), ...".
//

template <typename T>
void print (T val) {
    std::cout << val << " ";
}

template <typename... Args>
void print_all_folded (Args... args) {
    (print (args), ...);
}

void part_3_1() {
    print_all_folded (1, 2, 3, "four", 'f');
    std::cout << std::endl;
}

// TASK 2:
//   Write `bool all_true(Args... args)` that returns true only if every
//   argument is truthy, using `(args && ...)`.
//

template <typename... Args>
constexpr bool all_true (Args... args) {
    return (args && ...);
}

// TEST:
static_assert (all_true (true, 1, 2.5));
static_assert (! all_true (true, 0, 2.5));

// ----------------------------------------------------------------------------
// PART 4 — Perfect forwarding with a parameter pack
// ----------------------------------------------------------------------------
// Pairing a parameter pack with forwarding references lets you write a
// function that passes its arguments through to another function WITHOUT
// unnecessary copies, and while preserving whether each argument was an
// lvalue or rvalue. This is exactly how std::make_unique, std::vector::
// emplace_back, etc. are implemented.
//
// Syntax: `template <typename... Args> void wrapper(Args&&... args)`
// followed by `inner(std::forward<Args>(args)...)`.
//
// Notice there are TWO different `...` uses in that call:
//   `std::forward<Args>(args)` is the pattern to repeat, and the trailing
//   `...` expands it once per element of the pack — producing
//   `std::forward<Args0>(args0), std::forward<Args1>(args1), ...`
//
// TASK:
//   Write a small `struct Logger` with a constructor that takes a
//   std::string. Then write a function template `make_logger_and_log`
//   that takes a parameter pack by forwarding reference and perfectly
//   forwards it into `Logger{std::forward<Args>(args)...}`. Add a print
//   statement inside Logger's constructor so you can SEE it only gets
//   constructed once (no extra copies).
//
// Think about: why `Args&&...` instead of `Args...` or `const Args&...`?
// What breaks if you use plain `Args...` and pass an rvalue std::string?
//
// IMPORTANT GOTCHA: the wrapper places NO constraint on how many arguments
// you pass. Whatever you call it with, Args deduces to that many types, and
// the pack expansion forwards all of them into Logger{...}. Logger's
// constructor here takes exactly ONE std::string — so you must call
// make_logger_and_log with exactly one argument. Pass two or three and
// you'll get "no matching constructor," not because forwarding is broken,
// but because you asked Logger to construct from an arity it doesn't have.
// This wrapper is generic over constructor arity in general (that's the
// whole point of the pattern) — it's just that THIS Logger only supports
// arity 1, so that's the only call shape that will work here.
struct Logger {
    Logger (std::string what) {
        std::cout << "log: " << what << std::endl;
    };
};

template <typename... Args>
void make_logger_and_log (Args&&... args) {
    Logger (std::forward<Args> (args)...);
}

// ----------------------------------------------------------------------------
// PART 5 — C++20: abbreviated function templates and template lambdas
// ----------------------------------------------------------------------------
// C++20 lets you write a variadic generic lambda with explicit template
// syntax (no more having to fake it with `auto...` guesswork):
//   []<typename... Ts>(Ts... args) { ... }
//
// This is strictly more powerful than the older `[](auto... args){}` style
// because you get a NAME (Ts) for the types, so you can do things like
// `sizeof...(Ts)` or `std::same_as<...>` checks inside the lambda body.
//
// TASK:
//   Write a lambda using the `[]<typename... Ts>(Ts... args)` syntax that
//   returns `sizeof...(Ts)`, and assign it to a variable. Call it with a
//   few different argument lists to confirm it behaves like PART 1.
void part_5() {
    auto doit = []<typename... Ts> (Ts... args) -> auto {
        return sizeof...(args);
    };

    struct Six {
        int val { 0 };
    };
    std::cout << "lambda arg count is " << doit (1, 2, 3, 4, "five", Six {}) << std::endl;
}

// ----------------------------------------------------------------------------
// PART 6 — C++20: constraining a parameter pack with concepts
// ----------------------------------------------------------------------------
// You can require every element of a pack to satisfy a concept by folding
// the concept-check with `&&`:
//   template <typename... Ts>
//       requires (std::same_as<Ts, int> && ...)
//   void only_ints(Ts... args) { ... }
//
// Read that `requires` clause carefully: it's a fold expression (PART 3)
// whose "op" is `&&` and whose per-element expression is a concept check.
//
// TASK:
//   Write `sum_integral(Args... args)` constrained so it only compiles when
//   EVERY argument type satisfies `std::integral` (from <concepts>). Inside,
//   sum the arguments with a fold expression from PART 3. Then try calling
//   it with a double argument and read the compiler error — it should be
//   about the constraint, not some obscure deep template-instantiation
//   error. That's the whole point of constraining packs with concepts.
//
template <typename... Args>
requires (std::is_integral_v<Args>&&...) constexpr int sum_integral (Args... args) {
    const auto sum = (args + ...);
    return static_cast<int> (sum);
}

// TEST:
static_assert (sum_integral (1, 2, 3) == 6);
//   sum_integral(1, 2.0, 3); // should fail to compile — try it and read why

// ----------------------------------------------------------------------------
// main — uncomment calls as you complete each part
// ----------------------------------------------------------------------------
int main() {
    part_2();

    part_3_1();

    // PART 4
    make_logger_and_log ("hi");
    make_logger_and_log ("there");
    make_logger_and_log (std::string ("friend"));

    // PART 5
    part_5();

    // PART 6
    std::cout << "sum=" << sum_integral (1, 2, 3, 4) << std::endl;
    ;

    return 0;
}
