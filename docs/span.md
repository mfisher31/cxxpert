# std::span

Header: `<span>`. C++20.

Examples were compiled with `clang++ -std=c++20 -Wall -Wextra -Wpedantic`. Outputs and errors quoted below are what it printed.

Reference: [std::span](https://en.cppreference.com/w/cpp/container/span)

---

# Part 1: Theory and reference

## 1.1 What a span is

A `std::span<T>` is a **non-owning view of a contiguous sequence** of `T`. Internally it is a pointer and a length. It does not allocate, copy, or free anything.

It is to arrays and vectors what `std::string_view` is to strings. A function that takes a span can accept a `std::vector`, a `std::array`, a C array, or part of any of these, without being a template and without copying.

On this machine `sizeof(std::span<int>)` is 16 bytes, which is a pointer plus a size. Pass it **by value**.

## 1.2 The problem it solves

Before C++20 you had these choices for "a function that reads a run of ints":

| Option | Problem |
|---|---|
| `const std::vector<int>&` | Only accepts vectors, so arrays and sub-ranges need a copy. |
| `const int* p, size_t n` | Two parameters that can disagree. No bounds, no range-for. |
| A template on the container | Works, but is a template. Everything has to live in a header, and errors are noisy. |

`std::span<const int>` accepts all of them with one non-template function and one parameter.

## 1.3 Element type and constness

`std::span<T>` can read and write the elements. `std::span<const T>` can only read them. Prefer `span<const T>` for parameters that don't modify the data.

- A `span<T>` converts implicitly to `span<const T>`. The reverse does not compile.
- The span object itself is cheap to copy, and `const` on the span doesn't affect the elements, only `const` on `T` does.

## 1.4 Constructing one

All of these compile and behave as shown:

| From | Notes |
|---|---|
| A `std::vector<int>` (lvalue) | `std::span<int> s{v};` |
| A `std::array<int, 3>` | Works for `span<int>` and `span<const int>`. |
| A C array `int c[4]` | Works. |
| A `std::string` | Gives `std::span<const char>` (or `span<char>` for a non-const string). |
| A pointer and a count | `std::span<int> s{ptr, n}` |
| An iterator and a count | `std::span<int> s{v.begin() + 1, 2}` |
| A `span<int>` | Converts to `span<const int>` implicitly. |
| A temporary container | Allowed **only** for `span<const T>`, for example `f(std::vector<int>{1, 2, 3})` when `f` takes `std::span<const int>`. |

And these do **not** compile (I tested each):

| Attempt | Why |
|---|---|
| `std::span<int> s = std::vector<int>{1, 2};` | A temporary can't bind to a mutable span, because the data would vanish immediately. |
| `std::span<int> s{{1, 2, 3}};` | A braced list can't make a span in C++20. |
| `std::span<bool> s{vec_of_bool};` | `std::vector<bool>` packs bits and has no contiguous `bool`s. |
| `std::span<int, 4> s{c};` where `c` is `int[3]` | The sizes don't match, and the compiler knows. |

**Class template argument deduction** works. `std::span ds{c};` for `int c[4]` deduces `span<int, 4>`. For a vector it deduces a dynamic-extent span.

## 1.5 Dynamic vs static extent

The second template parameter is the **extent**, the number of elements.

| Form | Meaning |
|---|---|
| `std::span<int>` | Extent is `std::dynamic_extent`. The size is stored at runtime. |
| `std::span<int, 3>` | Exactly 3 elements, known at compile time. The size is not stored, so `sizeof` is 8 on 64-bit (just the pointer). |

I confirmed: `sizeof(std::span<int, 3>)` is 8 and `sizeof(std::span<int>)` is 16.

Use the dynamic form by default. Use the static form when the size is truly fixed and you want the compiler to check it. `std::dynamic_extent` is `size_t(-1)`.

## 1.6 Observing and accessing

| Member | Meaning |
|---|---|
| `size()` | Number of elements. |
| `size_bytes()` | Number of bytes. For a span of 5 ints, that's 20. |
| `empty()` | True if size is 0. |
| `operator[]` | Element access. **Not bounds-checked.** |
| `front()`, `back()` | First and last element. Undefined on an empty span. |
| `data()` | Pointer to the first element. |
| `begin()`, `end()`, `rbegin()`, `rend()` | Iterators, so range-for and the standard algorithms work. |

There is no `at()` in C++20.

## 1.7 Slicing

Slices are cheap. They return another span over the same memory, with no copy.

| Call | Result |
|---|---|
| `s.first(n)` | The first `n` elements. |
| `s.last(n)` | The last `n` elements. |
| `s.subspan(offset)` | Everything from `offset` to the end. |
| `s.subspan(offset, count)` | `count` elements starting at `offset`. |
| `s.first<N>()`, `s.last<N>()`, `s.subspan<Offset, Count>()` | Compile-time versions that return static-extent spans. |

For a span over `{2, 4, 6, 8, 10}`: `first(2)` has 2 elements, `last(2)` has 2, `subspan(1, 3)` has 3 starting at 4, and `subspan(2)` has 3 starting at 6.

An out-of-range request is **undefined behavior**, not an exception.

## 1.8 Bytes

`std::as_bytes(s)` gives a read-only `span<const std::byte>` over the same memory, and `std::as_writable_bytes(s)` a writable one. For 5 ints, the byte span has 20 elements. Use this for serialization or hashing.

## 1.9 Passing and returning

- **Parameters:** by value, `std::span<const T>`.
- **Return values:** be careful. A span is only valid while the owner is. Returning a span into a local container is a dangling reference.
- **Members:** storing a span in a class is fine, but the class must not outlive the data, and must not use the span after the owner resizes.

## 1.10 Danger: invalidation and lifetime

A span has no idea what it points into. These are your responsibility:

- **Reallocation.** If you build a span from a `std::vector` and then the vector grows (`push_back`, `reserve`, `resize` past capacity), the span points to freed memory. I demonstrated this: after `v.reserve(1000)`, a span built earlier still reports its old size, but its pointer is stale. Do not read it.
- **Temporaries.** A span made from a temporary is dangling as soon as the temporary dies. The language lets you pass a temporary to a `span<const T>` **function parameter**, which is safe because the temporary lives until the call returns. It is not safe to store that span.
- **Out of range.** `operator[]`, `front()`, `first()`, and `subspan()` have no bounds checks.

## 1.11 What a span doesn't do

- **No comparison.** `a == b` on two spans does not compile (I tested it). Compare with `std::ranges::equal` or `std::equal`, which compare elements.
- **No ownership.** It never frees memory.
- **Contiguous only.** It can't view a `std::list` or a `std::deque`.
- **No resizing.** You can narrow by slicing, but you can't grow.

## 1.12 A deduction gotcha in templates

A function like `template <class T> void f(std::span<const T>)` can't deduce `T` when you pass a vector, because template argument deduction does not apply conversions. You must write `f<int>(v)`, which I confirmed. Options when you want a generic function:

- Spell out `T` at the call site.
- Take a `std::ranges::contiguous_range` template parameter and constrain the element type with a concept (see [concepts.md](concepts.md)).
- Make the function non-generic (for example `std::span<const TimingSample>`), which deduces nothing, so every container converts.

This is the issue you'll hit in the `mean()` part of your warmup file.

## 1.13 Quick reference

| I want to... | Use |
|---|---|
| Accept any contiguous run, read-only | `std::span<const T>` parameter, by value |
| Accept and modify | `std::span<T>` |
| View part of a container | `first`, `last`, `subspan` |
| Loop over it | Range-for, or standard algorithms on `begin()`/`end()` |
| A fixed-size view checked at compile time | `std::span<T, N>` |
| View the bytes of an object array | `std::as_bytes` |
| Compare contents | `std::ranges::equal` |

---

# Part 2: Exercises

Write these in a scratch file, such as `src/span.cpp`.

1. **One function, many containers.** Write `void print_all(std::span<const int>)` and call it with a `std::vector<int>`, a `std::array<int, N>`, a C array, and a temporary `std::vector{1, 2, 3}`.

2. **Mutating through a span.** Write `void scale(std::span<int>, int factor)`. Call it on a vector, then print the vector to confirm it changed. Then try calling it with a temporary vector and read the compile error.

3. **Const conversion.** Make a `span<int>` and convert it to a `span<const int>`. Try the reverse and read the error.

4. **Slicing.** Given a vector of ten ints, print `first(3)`, `last(3)`, `subspan(4)`, and `subspan(2, 5)`. Verify each slice shares memory with the original by modifying one element through a slice.

5. **Windows.** Write `sum_windows(std::span<const int>, size_t window)` that returns the sum of each consecutive window of that size, using `subspan` in a loop.

6. **Static extent.** Make a `std::span<int, 3>` from a C array of three. Print `sizeof` for it and for a dynamic span of the same data. Try a mismatched size and read the compile error.

7. **Bytes.** Use `std::as_bytes` on a span of four `uint32_t` values, and print the byte span's size. Print each byte in hex using `std::format` (see [format.md](format.md)).

8. **Invalidation.** Build a span from a vector, then `push_back` until the vector reallocates. Compare `s.data()` with `v.data()` before and after to see that they differ. **Do not read through the stale span.**

9. **Comparison.** Show that `a == b` on two spans doesn't compile. Compare their contents with `std::ranges::equal`.

10. **Deduction.** Write `template <class T> double average(std::span<const T>)`. Call it with a vector and observe that you need `average<int>(v)`. Then rewrite it with a range concept so `average(v)` works with no explicit type.

11. **Warmup file.** Write `summarize(std::span<const TimingSample>)` for [src/00_cxx20_warmup.cpp](../src/00_cxx20_warmup.cpp), and call it with the full vector, with `first(10)`, and with `last(10)`.
