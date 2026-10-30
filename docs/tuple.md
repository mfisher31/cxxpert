# std::tuple

Practice file: `src/tuple.cpp`. Build with C++20.

- **Part 1** explains the standard library pieces you'll use, with links to cppreference.
- **Part 2** is the exercises. Each one lists the Part 1 sections to read first.

---

# Part 1: Theory and reference

## 1.1 What a tuple is

A [`std::tuple`](https://en.cppreference.com/w/cpp/utility/tuple) (header `<tuple>`) is a fixed-size collection of values with different types. Think of it as a struct whose fields have no names, only positions. The size and the types are part of the tuple's type, so `std::tuple<int, std::string>` and `std::tuple<std::string, int>` are different types.

**When to use one:**
- Returning several values from a function.
- Short-lived grouping, such as a map key or comparison key.
- Generic code, where a parameter pack has to be stored somewhere (see 1.7).

**When not to:** if the values have meaning and the data lives for a while, use a named struct. Positions like `get<1>` tell the reader nothing.

**Related:** [`std::pair`](https://en.cppreference.com/w/cpp/utility/pair) (header `<utility>`) is the two-element version. Tuples interoperate with it, so a pair converts to a two-element tuple, and most of the functions below accept pairs too.

## 1.2 Creating tuples

| Tool | What it gives you |
|---|---|
| Constructor with CTAD | A tuple that holds copies. `CTAD` is [class template argument deduction](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction): you don't write the template arguments, the compiler deduces them. |
| [`std::make_tuple`](https://en.cppreference.com/w/cpp/utility/tuple/make_tuple) | A tuple that holds copies of its arguments. It decays the types (see 1.7), so references are stripped. |
| [`std::tie`](https://en.cppreference.com/w/cpp/utility/tuple/tie) | A tuple of **lvalue references** to your variables. Arguments must be lvalues, so you can't pass a literal or a temporary. |
| [`std::forward_as_tuple`](https://en.cppreference.com/w/cpp/utility/tuple/forward_as_tuple) | A tuple of references that keeps each argument's value category: lvalue arguments become `T&`, rvalue arguments become `T&&`. Used for passing argument packs along without copying. |
| [`std::tuple_cat`](https://en.cppreference.com/w/cpp/utility/tuple/tuple_cat) | Concatenates any number of tuples (or pairs) into one bigger tuple. The result's type is the element types of all the inputs, in order. |

**Key difference to remember:** `make_tuple` copies, `tie` and `forward_as_tuple` refer. A tuple of references doesn't own anything, so don't keep one around after the variables it refers to are gone.

## 1.3 Accessing elements

- **[`std::get`](https://en.cppreference.com/w/cpp/utility/tuple/get)** reads an element by index or by type.
  - By index, the number is a template argument and must be known at compile time. A runtime index doesn't work, because each position can have a different type.
  - By type, the call only compiles if that type appears **exactly once** in the tuple. Zero or two matches is a compile error.
- **[Structured bindings](https://en.cppreference.com/w/cpp/language/structured_binding)** declare new names, one per element, in one go. You must name every element, and the names are new declarations, so you can't reuse the same names in the same scope.
- **[`std::tie`](https://en.cppreference.com/w/cpp/utility/tuple/tie)** on the **left** of an assignment unpacks into variables that already exist. Use this when structured bindings are the wrong fit, for example reusing existing names.
- **[`std::ignore`](https://en.cppreference.com/w/cpp/utility/tuple/ignore)** is a placeholder you pass to `tie` in the position of an element you don't want. Assigning anything to it does nothing.

## 1.4 Asking questions about tuple types

These are compile-time only. No tuple object has to exist.

- [`std::tuple_size`](https://en.cppreference.com/w/cpp/utility/tuple/tuple_size) and its shortcut `std::tuple_size_v<T>`: number of elements.
- [`std::tuple_element`](https://en.cppreference.com/w/cpp/utility/tuple/tuple_element) and its shortcut `std::tuple_element_t<I, T>`: the type of the element at index `I`.
- [`decltype`](https://en.cppreference.com/w/cpp/language/decltype) gives you the type of an expression without evaluating it. Wrapping a call like `tuple_cat` in `decltype` tells you the resulting type at no runtime cost.
- [`std::is_same`](https://en.cppreference.com/w/cpp/types/is_same) and `std::is_same_v<A, B>`: is type A exactly type B?
- [`static_assert`](https://en.cppreference.com/w/cpp/language/static_assert) turns a compile-time condition into a compile error if it is false. Combine it with `is_same_v` to verify types.

All of the above also work for `std::pair` and `std::array`, and for your own types if you specialize them (see bonus B2).

## 1.5 Calling functions with tuples

- **[`std::apply`](https://en.cppreference.com/w/cpp/utility/apply)** takes a callable and a tuple, and calls the callable with the tuple's elements as separate arguments. It's the bridge from "arguments stored in a tuple" to "an actual function call".
- **[`std::make_from_tuple`](https://en.cppreference.com/w/cpp/utility/make_from_tuple)** does the same but constructs an object of a given type from the elements.

These two are why tuples matter in generic code. A parameter pack (1.7) can't be stored on its own, but a tuple of the pack can, and `apply` turns it back into arguments later.

## 1.6 Comparing tuples

Tuples compare **element by element, left to right**, stopping at the first difference. That's lexicographic ordering, the same way words are alphabetized.

- Supported operators: `==`, and in C++20 `<=>`, from which `<`, `<=`, `>`, `>=` are derived. See [tuple comparison](https://en.cppreference.com/w/cpp/utility/tuple/operator_cmp).
- **The `tie` trick:** to give a struct an ordering, compare `tie(a.x, a.y, ...)` against `tie(b.x, b.y, ...)`. List members in priority order.
- **The C++20 way:** the three-way comparison operator [`<=>`](https://en.cppreference.com/w/cpp/language/operator_comparison#Three-way_comparison). Declaring it as `= default` makes the compiler generate it (see [defaulted comparisons](https://en.cppreference.com/w/cpp/language/default_comparisons)).
  - The return type must be `auto` or a comparison category such as `std::strong_ordering`, not `bool`.
  - As a member it must be `const`.
  - It compares members in **declaration order**.
  - A defaulted `<=>` also gives you a defaulted `==`.
- Because tuples are comparable, they work as keys in [`std::map`](https://en.cppreference.com/w/cpp/container/map) with no extra code.

## 1.7 Language features you'll run into

**Parameter packs.** `typename... Args` declares a pack of types, and `Args... args` a pack of values. Writing `args...` expands the pack into a comma-separated list. The ellipsis goes **after** the pack name. See [parameter packs](https://en.cppreference.com/w/cpp/language/parameter_pack).

**Fold expressions.** Apply an operator across a pack, such as summing every element or calling something once per element with the comma operator. See [fold expressions](https://en.cppreference.com/w/cpp/language/fold).

**Decay.** [`std::decay_t<T>`](https://en.cppreference.com/w/cpp/types/decay) converts a type to what you'd get by passing it by value: it strips references and top-level `const`, and turns arrays and functions into pointers. Use it when storing types in a tuple, so the tuple owns copies instead of holding dangling references.

**`std::move` vs `std::forward`.**
- [`std::move`](https://en.cppreference.com/w/cpp/utility/move) casts to an rvalue so the value can be moved from. Use it on by-value parameters you're about to store.
- [`std::forward`](https://en.cppreference.com/w/cpp/utility/forward) preserves lvalue-ness or rvalue-ness, but only works on a **forwarding reference**, which is a deduced `T&&` on a function template. A class template's parameter doesn't make one.
- Background: [value categories](https://en.cppreference.com/w/cpp/language/value_category).

**Concepts and `requires`.** A [`requires` clause](https://en.cppreference.com/w/cpp/language/constraints) puts a condition on a template, giving a clear error when it's not met.
- [`std::invocable<F, Args...>`](https://en.cppreference.com/w/cpp/concepts/invocable) (header `<concepts>`): can `F` be called with those arguments?
- [`std::invoke_result_t<F, Args...>`](https://en.cppreference.com/w/cpp/types/result_of) (header `<type_traits>`): the type returned by calling `F` with `Args...`. It gives a type, not a `bool`, and the extra arguments are the argument types you call with.
- [`std::is_void_v<T>`](https://en.cppreference.com/w/cpp/types/is_void): is `T` exactly `void`?
- Join conditions with `&&`. They're checked left to right and stop at the first failure.
- Use `F&` and `Args&...` in the checks when your code calls the stored function with lvalues.

**Parsing numbers.** [`std::from_chars`](https://en.cppreference.com/w/cpp/utility/from_chars) (header `<charconv>`) converts text to a number without exceptions or allocation. It takes two `const char*` pointers, not iterators (use a `string_view`'s `data()` and `data() + size()`), plus a variable to fill. It returns the error code and a pointer to where it stopped. Success means the error code is "no error" **and** the stop pointer reached the end.

**String literals.** The `s` suffix on `""` makes a `std::string` instead of a `const char*`. You need `#include <string>` and `using namespace std::string_literals;`. See [`operator""s`](https://en.cppreference.com/w/cpp/string/basic_string/operator%22%22s). In a braced return to a known type you often don't need it, because the conversion happens automatically.

**Formatting.** [`std::format`](https://en.cppreference.com/w/cpp/utility/format/format) (header `<format>`) builds strings with `{}` placeholders.

**`constexpr`.** [`constexpr`](https://en.cppreference.com/w/cpp/language/constexpr) lets a value or function be evaluated at compile time. Tuples support it.

## 1.8 Quick reference

| I want to... | Use | Section |
|---|---|---|
| Return several values | `std::tuple` return type, structured binding at the call | 1.1, 1.3 |
| Unpack into existing variables | `std::tie` on the left of `=` | 1.3 |
| Skip an element | `std::ignore` inside `tie` | 1.3 |
| Compare a struct field by field | `tie` on both sides, or defaulted `<=>` | 1.6 |
| Store arguments to call later | `tuple<decay_t<Args>...>` plus `std::apply` | 1.5, 1.7 |
| Join tuples | `std::tuple_cat` | 1.2 |
| Count elements / get an element's type | `tuple_size_v` / `tuple_element_t` | 1.4 |
| Check a type at compile time | `static_assert` plus `is_same_v` | 1.4 |
| Read an element by type | `std::get<T>` | 1.3 |
| Build an object from a tuple | `std::make_from_tuple` | 1.5 |

---

# Part 2: Exercises

Write your solutions in `src/tuple.cpp`. Each exercise lists the sections of Part 1 to read first.

## Exercise 1: Returning multiple values

**Read first:** 1.1, 1.3, 1.7 (parsing numbers, string literals)

**Task:** Write a function that parses a string into an int and reports three things: whether it worked, the value, and an error message.

- **Use:** a `std::tuple` return type and `std::from_chars`.
- **How:** `from_chars` gives back an error code and a pointer to where it stopped. Success means the error code is "no error" **and** the pointer reached the end of the string. Without that second check, `"12abc"` would count as a success.
- **Caller side:** unpack the result with a structured binding. Remember you can't reuse the same binding names in the same scope. Put the repeated part in a helper function, or use `tie` into existing variables.
- **Test with:** `"42"`, `"abc"`, `"12abc"`, and the empty string.

## Exercise 2: `tie` and `ignore`

**Read first:** 1.2, 1.3, 1.6

**Task A:** Take a pair or tuple and copy its elements into variables that already exist.

- **Use:** `std::tie` on the left side of an assignment.
- **How:** `tie` builds a tuple of references, so assigning a tuple to it writes through to your variables.
- **Then:** pull out only one element and skip the rest with `std::ignore`.

**Task B:** Make a struct with last name, first name, and age sortable.

- **Use:** `tie` on both sides of a `<` comparison.
- **How:** tuples compare element by element, left to right, so listing the members in priority order gives you the ordering.
- **Test with:** same last name with different first names, different last names, and comparing an object with itself (it must not be less than itself).
- **Stretch:** delete your `operator<` and use a defaulted `operator<=>` instead. Member declaration order now decides the ordering. Check that nothing else breaks.

## Exercise 3: `std::apply`

**Read first:** 1.5, 1.7 (parameter packs, decay, `move` vs `forward`, concepts and `requires`)

**Task:** Build a small class template that stores a callable and its arguments now, and runs the call later.

- **Shape:** `template <typename F, typename... Args>`.
  - `F` is the type of the callable. It's a template parameter, so lambdas work.
  - The constructor takes the callable **first**, then the args, and stores both. List the members in the init list in the same order they are declared.
  - Class template argument deduction fills in `F` and `Args...` for you when you construct it.
- **Use:** a `std::tuple` member to hold the args, and `std::apply` to call the stored function.
- **How:** a parameter pack can't be stored directly, but a tuple of the pack can. `apply` takes the callable and the tuple and passes the elements as separate arguments.
- **Moving args in:** the constructor takes `Args...` by value, so initialize the tuple from `std::move(args)...`. `std::forward` doesn't apply, because `Args` belongs to the class, not the constructor.
- **Decay:** if you ever take the args as references, store `std::decay_t<Args>` in the tuple so the tuple owns copies.
- **Constrain it:** add a `requires` clause so the class only accepts a callable that can be invoked with the stored args. Then add a second condition that its return type is `void`. Order the conditions so the invocable check comes first.
- **Test with:** a lambda taking two ints, and one taking a string and an int.
- **Stretch:** call `apply` directly on a three-int tuple with a generic lambda that sums its arguments using a fold expression.

## Exercise 4: Compile-time introspection

**Read first:** 1.2, 1.3, 1.4

**Task:** Answer questions about tuple types without creating any values.

- **Combine types:** wrap `std::tuple_cat` in `decltype` and give it default-constructed tuples. `decltype` doesn't evaluate, so this costs nothing.
- **Type of element N:** `std::tuple_element_t<N, T>`.
- **Number of elements:** `std::tuple_size_v<T>`.
- **Access by type:** `std::get<T>(t)`. Then try it on a tuple with two ints and read the error.
- **Check your answers:** use `static_assert` with `std::is_same_v`.
- **Stretch:** for `int x`, work out the types that `make_tuple(x)`, `tie(x)`, and `forward_as_tuple(std::move(x))` return.
  - `make_tuple` copies and decays.
  - `tie` makes an lvalue reference.
  - `forward_as_tuple` keeps the value category, so it gives an rvalue reference here.
  - Confirm each with `static_assert`.

## Exercise 5: Printing any tuple

**Read first:** 1.5, 1.7 (parameter packs, fold expressions)

**Task:** Print a tuple as `(1, hello, 2.5)`, including the empty tuple as `()`.

- **Use:** `std::apply` with a generic lambda that takes a pack, and a fold expression over the comma operator to visit each element.
- **How:** the fold calls a small helper once per element. The helper keeps a "first" flag and prints the separator only between items.
- **Test with:** a mixed tuple, an empty one, and a single element.
- **Stretch:** write `for_each_in_tuple(t, f)` the same way and rebuild the printer on top of it.

## Bonus

- **B1, `make_from_tuple`** (1.5): build a struct from a tuple's elements. C++20 allows parenthesized aggregate init, so you may not need a constructor.
- **B2, custom structured bindings** (1.3, 1.4): give a class with private members the tuple protocol.
  - Specialize `std::tuple_size` and `std::tuple_element`.
  - Provide a `get<I>` that the compiler can find.
- **B3, tuple as map key** (1.6): use `std::map` with a three-int tuple key. It works with no extra code because tuples define comparison. Explain why for yourself.
- **B4, constexpr** (1.7): redo exercise 4 with `constexpr` values and `static_assert`.
