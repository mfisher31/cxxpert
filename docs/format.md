# std::format

Header: `<format>`. Available in C++20. Every example below was compiled with `clang++ -std=c++20` and the output shown is what it printed.

`std::format` builds a `std::string` from a format string with `{}` placeholders, like Python's `str.format`. It is type-safe (no `%d` vs `%s` mismatches), doesn't depend on locale by default, and can be extended to your own types.

Reference: [std::format](https://en.cppreference.com/w/cpp/utility/format/format) · [format string syntax](https://en.cppreference.com/w/cpp/utility/format/spec)

---

# Part 1: Basics

## 1.1 Placeholders

```cpp
std::format("{} {} {}", 1, 2.5, "hi");     // "1 2.5 hi"
std::format("{1} {0} {1}", "a", "b");      // "b a b"
std::format("{{literal}} {}", 5);          // "{literal} 5"
```

- `{}` takes the next argument, in order.
- `{0}`, `{1}` pick an argument by position, and you can reuse one. Don't mix automatic and numbered placeholders in one string.
- `{{` and `}}` print literal braces.
- The number of placeholders must match the number of arguments. Extra arguments are an error too.

## 1.2 The format spec

Inside a placeholder, everything after the `:` is the spec. The general shape is:

```
{ index : fill align sign # 0 width .precision type }
```

Every part is optional. Order matters.

| Part | Meaning | Example | Result |
|---|---|---|---|
| width | minimum field width | `{:5}` of `42` | `   42` |
| align | `<` left, `>` right, `^` center | `{:^8}` of `"c"` | `   c    ` |
| fill | character before the align | `{:*^8}` of `"m"` | `***m****` |
| `0` | zero-pad numbers | `{:05}` of `42` | `00042` |
| sign | `+` always show, `-` negatives only (default), space | `{:+}` of `42` | `+42` |
| `#` | alternate form (prefixes) | `{:#x}` of `255` | `0xff` |
| precision | digits after the point for floats, max chars for strings | `{:.2f}` of `3.14159` | `3.14` |
| type | how to render the value | see below | |

Defaults: numbers align right, strings align left.

**Type letters**

| Type | Letters |
|---|---|
| Integers | `d` decimal, `x` / `X` hex, `o` octal, `b` / `B` binary |
| Floats | `f` fixed, `e` scientific, `g` general, `a` hex-float |
| Strings | `s` |
| Characters | `c` as a character, `d` as its number |
| Booleans | `s` as `true`/`false`, `d` as `1`/`0` |
| Pointers | `p` (and only `void*` or `nullptr`; cast other pointers) |

Examples:

```cpp
std::format("{:x} {:X} {:#x}", 255, 255, 255);     // "ff FF 0xff"
std::format("{:b} {:#b} {:o} {:#o}", 5, 5, 8, 8);  // "101 0b101 10 010"
std::format("{:.2f} {:10.3f}", 3.14159, 3.14159);  // "3.14      3.142"
std::format("{:e} {:g}", 31415.9, 0.0001);         // "3.141590e+04 0.0001"
std::format("{:.3} {:.3}", "truncated", 3.14159);  // "tru 3.14"
std::format("{} {:d} {:c}", 'x', 'x', 65);         // "x 120 A"
std::format("{} {:d} {:s}", true, true, true);     // "true 1 true"
```

Floating-point rounding is exact on the binary value: `{:.1f}` of `2.25` prints `2.2` (ties go to even).

## 1.3 Dynamic width and precision

A width or precision doesn't have to be written into the format string. Put a nested `{}` where the number would go, and pass the number as an argument. This is how you set a column width from a variable at runtime.

The two calls below do the same thing. The first fixes the width at 6, the second reads it from an argument:

```cpp
std::format("[{:6}]", "w");     // "[w     ]"
std::format("[{:{}}]", "w", 6); // "[w     ]"
```

**The square brackets are not part of the syntax.** `[` and `]` are ordinary text that gets copied to the output. They're here only to make the padding visible, because trailing spaces are otherwise hard to see. Without them the result is the same, just harder to read:

```cpp
std::format("{:{}}|", "w", 6);  // "w     |"  (the | marks where the padding ends)
```

Reading `"[{:{}}]"` piece by piece:

| Piece | Meaning |
|---|---|
| `[` | literal text |
| `{` ... `}` (outer) | placeholder for the value, takes the first argument, `"w"` |
| `:` | start of the spec |
| `{}` (inner) | the width, takes the next argument, `6` |
| `]` | literal text |

**Argument order.** The value comes first and the width or precision comes **after** it, because the outer placeholder opens first and arguments are taken left to right:

```cpp
std::format("[{:{}}]", "w", 6);            // "[w     ]"   value, width
std::format("[{:.{}f}]", 3.14159, 1);      // "[3.1]"      value, precision
std::format("[{:{}.{}f}]", 3.14159, 8, 2); // "[    3.14]" value, width, precision
std::format("[{:{}}] [{:{}}]", "a", 3, "b", 5); // "[a  ] [b    ]"
```

Numbered placeholders let you choose any order:

```cpp
std::format("[{1:{0}}]", 6, "w");  // "[w     ]"
```

The nested argument must be an integer. Swapping the order in `std::format("[{:{}}]", 6, "w")` tries to use `"w"` as the width, which is a compile error when the format string is a literal. With `std::vformat` it throws `std::format_error` at runtime instead. With libc++ the message in both cases is:

```
Replacement argument isn't a standard signed or unsigned integer type
```

## 1.4 The functions

| Function | What it does |
|---|---|
| [`std::format`](https://en.cppreference.com/w/cpp/utility/format/format) | Returns a `std::string`. |
| [`std::format_to`](https://en.cppreference.com/w/cpp/utility/format/format_to) | Writes to an output iterator, such as `std::back_inserter(str)`. No new string allocated. |
| [`std::format_to_n`](https://en.cppreference.com/w/cpp/utility/format/format_to_n) | Same, but stops after N characters. Returns the end iterator **and** the full size it would have needed. It does not add a null terminator. |
| [`std::formatted_size`](https://en.cppreference.com/w/cpp/utility/format/formatted_size) | Tells you how many characters the result would have, without producing it. |
| [`std::vformat`](https://en.cppreference.com/w/cpp/utility/format/vformat) | Takes the format string at **runtime**, with arguments packed by [`std::make_format_args`](https://en.cppreference.com/w/cpp/utility/format/make_format_args). |

```cpp
std::string buf;
std::format_to(std::back_inserter(buf), "{}+{}", 3, 4);    // buf == "3+4"

char arr[8];
auto r = std::format_to_n(arr, sizeof arr - 1, "{}", "abcdefghijkl");
*r.out = '\0';          // arr == "abcdefg", r.size == 12 (what was wanted)

std::formatted_size("{}", 12345);                           // 5

std::string fmt = "{} and {}";
int n = 5;
std::vformat(fmt, std::make_format_args(n, n));             // "5 and 5"
```

## 1.5 Compile-time checking

When the format string is a **string literal** (or other constant expression), the compiler checks it against the argument types. A wrong placeholder count, a bad spec like `{:d}` on a string, or an unclosed brace is a compile error, not a runtime surprise.

A format string held in a normal variable can't be checked at compile time, so `std::format(fmt, ...)` with a non-constant `fmt` won't compile. Use `std::vformat` for those.

## 1.6 What's not in C++20

- `std::print` / `std::println` are C++23 ([reference](https://en.cppreference.com/w/cpp/io/print)). In C++20 you print with `std::cout << std::format(...)`.
- Formatting ranges and tuples directly, as in `std::format("{}", vec)`, is C++23.

---

# Part 2: Formatting your own types with `std::formatter`

To make `std::format("{}", my_struct)` work, specialize [`std::formatter<T>`](https://en.cppreference.com/w/cpp/utility/format/formatter) for your type.

## 2.1 The two members you provide

```cpp
template <>
struct std::formatter<MyType> {
    constexpr auto parse(std::format_parse_context& ctx);
    auto format(const MyType& value, std::format_context& ctx) const;
};
```

- **`parse`** reads the spec between `:` and `}`. It receives a [`format_parse_context`](https://en.cppreference.com/w/cpp/utility/format/basic_format_parse_context). It must return an iterator pointing at the closing `}`. If you don't support any spec, return `ctx.begin()` unchanged. It should be `constexpr`, so the compile-time check from 1.5 can run it.
- **`format`** writes the value. It receives a [`format_context`](https://en.cppreference.com/w/cpp/utility/format/basic_format_context), whose `out()` is the output iterator. Write to it and **return the iterator after the last character you wrote**. Make it `const`.

Specializations are written in namespace `std`. The `template <> struct std::formatter<...>` form lets you do that from outside the namespace.

## 2.2 Easiest route: inherit from an existing formatter

If your type can be represented as a string, derive from `std::formatter<std::string>`, build the text yourself, and hand it to the base class. You skip writing `parse`, and the user's width, fill, and alignment (`{:>14}`) work for free.

```cpp
struct Person { std::string name; int age; };

template <>
struct std::formatter<Person> : std::formatter<std::string> {
    auto format(const Person& p, std::format_context& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{} ({})", p.name, p.age), ctx);
    }
};

std::format("{}", Person{"Ann", 30});            // "Ann (30)"
std::format("[{:>14}]", Person{"Bob", 25});      // "[      Bob (25)]"
```

## 2.3 Enums

The same trick with `std::string_view`:

```cpp
enum class Color { Red, Green, Blue };

template <>
struct std::formatter<Color> : std::formatter<std::string_view> {
    auto format(Color c, std::format_context& ctx) const {
        std::string_view name = "?";
        switch (c) {
            case Color::Red:   name = "Red";   break;
            case Color::Green: name = "Green"; break;
            case Color::Blue:  name = "Blue";  break;
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

std::format("{} {:>8} {:<8}|", Color::Green, Color::Red, Color::Blue);
// "Green      Red Blue    |"
```

## 2.4 Forwarding the spec to your members

When your type is made of formattable parts, you can let the user's spec apply to each part. Here a `Point` accepts any float spec, such as `{:.1f}` or `{:8.2f}`, and applies it to both coordinates. The formatter owns a `std::formatter<double>`, hands `parse` over to it, and calls its `format` once per coordinate.

```cpp
struct Point { double x, y; };

template <>
struct std::formatter<Point> {
    std::formatter<double> inner;

    constexpr auto parse(std::format_parse_context& ctx) {
        return inner.parse(ctx);                  // let double read the spec
    }

    auto format(const Point& p, std::format_context& ctx) const {
        auto out = ctx.out();
        *out++ = '(';
        ctx.advance_to(out);                      // tell ctx where we are
        out = inner.format(p.x, ctx);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);
        out = inner.format(p.y, ctx);
        *out++ = ')';
        return out;
    }
};

Point p{1.5, 2.25};
std::format("{}", p);          // "(1.5, 2.25)"
std::format("{:.1f}", p);      // "(1.5, 2.2)"
std::format("[{:8.2f}]", p);   // "[(    1.50,     2.25)]"
```

Notes on this one:
- Whenever you hand `ctx` to another formatter, call `ctx.advance_to(out)` first, so it continues writing from your current position and doesn't overwrite.
- The width in `{:8.2f}` applies to each coordinate separately, not to the whole point. If you want the width for the whole string, use the 2.2 approach instead.

## 2.5 Choosing an approach

| You want | Use |
|---|---|
| The simplest thing for a string-like representation, with width/alignment working | 2.2 (inherit from `formatter<std::string>`) |
| A name for an enum | 2.3 (inherit from `formatter<std::string_view>`) |
| The user's spec applied to each member | 2.4 (own a member formatter and forward `parse`) |
| Your own spec letters, like `{:v}` for verbose | Write `parse` yourself: look at `ctx.begin()`, record the letter, return the iterator at `}`. Then branch on it in `format`. |

## 2.6 Common mistakes

- **`parse` returns the wrong iterator.** It must stop at the `}`, not past it, and not at the start if you consumed characters.
- **Forgetting `const` on `format`.** Newer standard libraries require it, and older ones accept both.
- **Not returning the iterator from `format`.** Return the one after your last character.
- **Skipping `advance_to` before delegating.** The output is overwritten or garbled.
- **Declaring the specialization after first use.** The compiler must see it before any `std::format` call that formats your type.

---

# Exercises

1. Print a table of three columns (name, quantity, price) with fixed widths, name left-aligned, quantity right-aligned, and price with two decimals. Then make the widths dynamic.
2. Print an integer in decimal, hex with a `0x` prefix, octal, and binary with a `0b` prefix, using a single `format` call.
3. Use `format_to` with `std::back_inserter` to build one string from a loop, without making a temporary string per iteration.
4. Use `format_to_n` into a fixed `char[16]` buffer, and check whether the output was truncated using the returned size.
5. Write a formatter for your `Person` struct from `tuple.cpp` that prints `Last, First (age)` and honors `{:>30}`.
6. Write a formatter for a `Color` enum, and one for a struct that contains a `Color`, so formatting the struct reuses the enum's formatter.
7. Give `Point` a custom spec letter: `{:v}` prints `x=1.5 y=2.25` and the default prints `(1.5, 2.25)`. This needs your own `parse`.
8. Make a deliberate mistake (wrong placeholder count, `{:d}` on a string) and read the compile error. Then trigger the same mistake via `vformat` and see that it fails at runtime instead.
