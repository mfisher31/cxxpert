#include <variant>
#include <string>
#include <iostream>

// ============================================================================
// std::variant — guided exercises
// ============================================================================
// Same format as variadic_function.cpp: each part explains the mechanics
// precisely, then gives you a task. Nothing is implemented for you — write
// the code yourself, wire calls into main(), and test as you go.
// ============================================================================


// ----------------------------------------------------------------------------
// PART 1 — What a variant actually is, and what "active alternative" means
// ----------------------------------------------------------------------------
// std::variant<T1, T2, ..., Tn> is a type-safe tagged union. Mechanically:
// it reserves enough storage for whichever of T1..Tn is largest, PLUS a
// hidden integer "index" (a tag) recording which one is currently live. At
// any given moment, exactly ONE of the Ts is "the active alternative" — the
// others don't exist in memory as constructed objects at all, same as a C
// union, except the variant tracks which one for you and enforces it.
//
// Default construction: std::variant<int, std::string> v; constructs the
// FIRST alternative (int) using its default constructor, and sets the index
// to 0. This is why the first alternative in the list must be default-
// constructible unless you explicitly initialize the variant with something
// else at the point of declaration (more on this in PART 4).
//
// Assignment: v = 42; picks whichever alternative type 42 converts to (int
// here) and reconstructs the variant to hold THAT alternative, destroying
// whatever was there before and updating the index. v = std::string("hi");
// then destroys the int and constructs a std::string in its place — note
// this is a real destruction + construction, not a reinterpretation.
//
// Two ways to inspect what's currently active, without extracting the value:
//   v.index()                    -> size_t, the 0-based index of the active
//                                    alternative (0 for T1, 1 for T2, ...)
//   std::holds_alternative<T>(v) -> bool, true iff T is the active type
//
// TASK:
//   Declare `std::variant<int, double, std::string> v;` and, step by step:
//     1. Print v.index() right after default construction. Confirm it's 0.
//     2. Assign a double to it. Print v.index() again — confirm it's now 1.
//     3. Assign a std::string to it. Confirm std::holds_alternative<std::string>(v)
//        is true and holds_alternative<double> is now false.
//   The point of this part is purely to see the index/tag change as you
//   reassign — nothing about *extracting* the value yet, that's PART 2.
void part_1() {
    using namespace std::string_literals;
    
    std::variant<int, double, std::string> v;
    std::cout << v.index() << std::endl;
    v = 100.5;
    std::cout << v.index() << std::endl;
    v = "helloworld"s;

    std::cout << std::boolalpha << "double=" << std::holds_alternative<double>(v)
        << " " << "string=" << std::holds_alternative<std::string>(v)
        << std::endl;
}

// ----------------------------------------------------------------------------
// PART 2 — Extracting the value: get<T> vs get_if<T>, and why both exist
// ----------------------------------------------------------------------------
// std::get<T>(v) returns a reference to the active alternative IF T is
// currently active. If T is NOT the active alternative, it doesn't return
// some garbage or a null reference — it THROWS std::bad_variant_access.
// This is checked at runtime against v's actual index; get<T> is only safe
// to call when you already know (from index() or holds_alternative<T>, or
// from program logic) that T is active.
//
// You can also index by position instead of type: std::get<0>(v) gets
// whatever the first template alternative is, by position rather than name.
// This matters when two alternatives have the same type (e.g.
// variant<int, int> — std::get<int> would be ambiguous, but get<0> and
// get<1> are not).
//
// std::get_if<T>(&v) is the non-throwing alternative: it takes a POINTER to
// the variant (note the &) and returns T* — a valid pointer to the value if
// T is active, or nullptr if it isn't. No exception, just a pointer you must
// check. This is the right tool when "wrong type active" is an expected,
// normal-flow outcome rather than a programming error.
//
// TASK:
//   Write a function `describe(const std::variant<int, double, std::string>& v)`
//   that:
//     1. Uses std::get_if<std::string>(&v) to check for a string, and if
//        non-null, prints its length.
//     2. Otherwise, tries std::get<int>(v) inside a try/catch that catches
//        std::bad_variant_access, so you can SEE the exception fire when
//        the active type isn't int. Print something in the catch block.
//   Call `describe` with the variant holding each of its three alternative
//   types in turn, so you exercise both the get_if success path and the
//   get() exception path.

void describe (const std::variant<int, double, std::string>& v) {
    if (auto res = std::get_if<std::string>(&v)) {
        std::cout << "string len = " << res->length() << std::endl;
    } else {
        try {
            std::cout << "int val = " << std::get<int> (v) << std::endl;
        } catch (const std::bad_variant_access& e) {
            std::cout << "err: " << e.what() << std::endl;
        }
    }
}

void part_2() {
    std::variant<int, double, std::string> v;
    v = 10;
    describe(v);
    v = 100.3;
    describe(v);
    v = std::string ("hello world, this is a variant string");
    describe (v);
}

// ----------------------------------------------------------------------------
// PART 3 — std::visit and the "overloaded lambda set" visitor pattern
// ----------------------------------------------------------------------------
// get/get_if require YOU to already know which type is active. std::visit
// flips this around: you give it a callable, and std::visit inspects the
// variant's index internally and calls your callable with a reference to
// whichever alternative is actually active — fully resolved to its real
// type, not some generic "variant value."
//
// The hard requirement: your callable must be invocable with EVERY
// alternative type in the variant, or it's a compile error (not a runtime
// one) — std::visit has to be able to generate a call for each possible
// active type, even the ones that don't happen to be active right now.
//
// The standard way to build "one callable that handles every type
// differently" is a trick that reuses exactly what you learned in
// variadic_function.cpp: a class template that inherits from a PACK of
// lambda types, pulling all their operator()s into one overload set:
//
//   template <typename... Ts>
//   struct Overloaded : Ts... {
//       using Ts::operator()...;
//   };
//
// Mechanically: each lambda you write (e.g. [](int i){...}) is its own
// unique, unnamed closure TYPE with its own operator(). Overloaded<L1, L2,
// L3> inherits from all three closure types at once (Ts... as a pack of
// BASE CLASSES — the `...` after Ts in `: Ts...` is a pack expansion in a
// base-specifier-list, same mechanism as everywhere else `...` appears).
// Each base's operator() is private to that base by default visibility
// rules for overload purposes across bases, so `using Ts::operator()...;`
// (a pack-expanded using-declaration) explicitly imports all of them into
// Overloaded's own scope as one combined overload set. Now an instance of
// Overloaded can be called with an int, a double, or a std::string, and
// ordinary overload resolution picks the matching lambda for whatever
// argument type you pass — including the one std::visit passes it.
//
// To construct one without spelling out `Overloaded<L1, L2, L3>` by hand,
// you rely on class template argument deduction (CTAD) from the braced
// list of lambdas: `Overloaded{lambda1, lambda2, lambda3}`. If your
// compiler/standard mode doesn't deduce this automatically, add an explicit
// deduction guide directly below the struct:
//   template <typename... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
//
// TASK:
//   1. Define the `Overloaded` template above, exactly as described.
//   2. Write three lambdas, one per alternative type (int, double,
//      std::string), each printing something different and type-specific
//      (e.g. the int's value doubled, the double's value halved, the
//      string's length).
//   3. Call `std::visit(Overloaded{lambda_int, lambda_double, lambda_string}, v)`
//      with v holding each alternative in turn, and confirm the right
//      lambda fires each time based purely on the variant's active type —
//      no index checks, no get/get_if anywhere in this part.

template<typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

void part_3() {
    auto intf = [](int val) { std::cout << "intval=" << val << std::endl; };
    auto dblf = [](double val) { std::cout << "dblval=" << val << std::endl; };
    auto strf = [](std::string val) { std::cout << "strval=" << val << std::endl; };
    Overloaded<decltype(intf), decltype(dblf), decltype(strf)> handler;
    std::variant<int, double, std::string> v;
    v = 100;
    std::visit(handler, v);
    v = 101.33;
    std::visit(handler, v);
    v = "one hundered and four";
    std::visit (handler, v);
}

// ----------------------------------------------------------------------------
// PART 4 — std::monostate: an explicit "empty" alternative
// ----------------------------------------------------------------------------
// Recall from PART 1: a default-constructed variant default-constructs its
// FIRST alternative. That's a problem if none of your real alternative
// types is default-constructible (or if being default constructed into a
// real type doesn't make semantic sense — e.g. you don't want a "default"
// std::string silently meaning "nothing has happened yet").
//
// std::monostate is a trivial, empty, always-default-constructible type
// that exists for exactly this purpose: put it FIRST in the alternative
// list, and a default-constructed variant lands in an explicit, obviously-
// empty state rather than accidentally default-constructing one of your
// real types.
//
// TASK:
//   Declare `std::variant<std::monostate, int, std::string> v2;` with no
//   initializer. Confirm `std::holds_alternative<std::monostate>(v2)` is
//   true immediately after declaration. Then write a function that takes
//   such a variant and, using std::visit with an Overloaded set (reuse
//   PART 3's machinery — add a lambda overload for std::monostate), prints
//   "empty" for the monostate case and something type-specific otherwise.


// ----------------------------------------------------------------------------
// PART 5 — C++20: variant operations are constexpr
// ----------------------------------------------------------------------------
// As of C++20, std::variant's core operations — construction, operator=,
// std::get, std::holds_alternative, std::visit, index() — are all usable in
// a constexpr context, PROVIDED every alternative type involved is itself a
// literal type with constexpr constructors/operations (plain int/double are
// fine; std::string is NOT, because it isn't a literal type — its
// constructor isn't constexpr before a later standard, so you can't use a
// variant<..., std::string> inside a constexpr evaluation).
//
// TASK:
//   Write a `constexpr` function template that takes a
//   `std::variant<int, double>` by value, uses std::visit with an
//   Overloaded set to convert whichever alternative is active to a double,
//   and returns that double. Call it inside a `static_assert` with a
//   variant holding an int, and again with one holding a double, proving
//   the whole get-the-active-value-out pipeline runs entirely at compile
//   time. This is only possible because int/double are literal types —
//   note you cannot do this exercise with std::string in the mix.


// ----------------------------------------------------------------------------
// PART 6 — In-place construction: emplace, in_place_type, in_place_index
// ----------------------------------------------------------------------------
// Assigning `v = SomeType{args...}` always constructs a temporary SomeType
// first, then move/copy-constructs the variant's internal storage from it —
// an extra move you may not want for an expensive-to-move type. Variant
// gives you ways to construct the active alternative DIRECTLY in its
// internal storage, with no intermediate temporary:
//
//   v.emplace<T>(args...)              -- construct T in place, by type
//   v.emplace<I>(args...)              -- construct the I-th alternative in place
//   std::variant<T1,T2> v{std::in_place_type<T2>, args...}   -- same, but at
//                                          construction time instead of after
//   std::variant<T1,T2> v{std::in_place_index<1>, args...}   -- by position
//
// These matter especially when an alternative type has no default
// constructor, or when two alternatives have the same type and you must
// disambiguate by position (in_place_index) rather than by name.
//
// TASK:
//   Declare `std::variant<int, std::string> v3` directly with
//   `std::in_place_type<std::string>` and constructor arguments that build
//   a std::string from a count and a char (the (size_t, char) std::string
//   constructor) — proving you can hand multiple constructor arguments
//   straight through without ever forming a temporary std::string. Then
//   call v3.emplace<int>(...) to switch it to the int alternative in place,
//   and confirm v3.index() reflects the change.


// ----------------------------------------------------------------------------
// main — uncomment/call as you complete each part
// ----------------------------------------------------------------------------
int main() {
    // PART 1
    part_1();

    // PART 2
    part_2();

    // PART 3
    part_3();

    // PART 4

    // PART 5

    // PART 6

    return 0;
}
