# <random>

Header: `<random>`. Available since C++11, with a C++20 concept for generators. The snippets below were compiled with `clang++ -std=c++20`. Where output is random I say so instead of showing a value.

Reference: [Pseudo-random number generation](https://en.cppreference.com/w/cpp/numeric/random)

---

# Part 1: Theory and reference

## 1.1 Why not `rand()`

The C function `std::rand()` from `<cstdlib>` has known problems:
- **Low quality.** The algorithm is unspecified, and many implementations are weak.
- **Small range.** It returns `0` to `RAND_MAX`, which is only guaranteed to be at least 32767.
- **Modulo bias.** `rand() % 6` makes some results slightly more likely than others whenever the range doesn't divide evenly.
- **One global state.** There's one hidden generator, seeded with `srand`, shared by everything.

`<random>` fixes all of these by splitting the job in two.

## 1.2 The model: engine plus distribution

| Piece | Job | Example |
|---|---|---|
| **Engine** (generator) | Produces a stream of uniformly random **raw integers**. | `std::mt19937` |
| **Distribution** | Turns raw integers into the values and shape you want. | `std::uniform_int_distribution<int>{1, 6}` |
| **Seed source** | Gives the engine its starting state. | `std::random_device` |

You create an engine once, create a distribution describing what you want, and call the distribution with the engine as its argument:

```cpp
std::random_device rd;
std::mt19937 gen{rd()};                       // engine, seeded
std::uniform_int_distribution<int> die{1, 6}; // distribution
int roll = die(gen);                          // draw one value
```

The engine is the source of randomness. The distribution does the math. Neither knows about the other until the call.

## 1.3 Engines

An engine is a pseudo-random number generator: it's deterministic, so the same seed gives the same sequence. Each call to `gen()` advances its internal state and returns the next integer.

| Engine | Notes |
|---|---|
| [`std::mt19937`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine) | **The default choice.** 32-bit Mersenne Twister. Fast, good statistical quality, huge period (2^19937 - 1). State is about 2.5 KB. |
| `std::mt19937_64` | Same algorithm with 64-bit output. Use when you need 64-bit values. |
| [`std::minstd_rand`](https://en.cppreference.com/w/cpp/numeric/random/linear_congruential_engine) | Small linear congruential engine. Tiny state, lower quality. |
| [`std::ranlux24`, `std::ranlux48`](https://en.cppreference.com/w/cpp/numeric/random/subtract_with_carry_engine) | Higher quality, slower. |
| `std::knuth_b` | Shuffled-output adaptor over `minstd_rand0`. |
| [`std::default_random_engine`](https://en.cppreference.com/w/cpp/numeric/random) | **Implementation-defined**, so it may be a different algorithm on different compilers. Avoid it if you care about reproducibility. |

Useful facts:
- Every engine has `min()` and `max()`. For `mt19937` that's 0 to 4294967295.
- `gen.discard(n)` skips `n` outputs.
- `mt19937` is **fully specified** by the standard. A default-constructed one (seed 5489) yields `3499211612` first and `4123659995` as its 10,000th value on every conforming implementation. I confirmed both here.
- Copying an engine copies its state, so the copy repeats the same numbers as the original. Pass engines by reference.

## 1.4 Seeding

**`std::random_device`** ([reference](https://en.cppreference.com/w/cpp/numeric/random/random_device)) is a source of non-deterministic randomness, usually from the OS. Call it to get a seed value. It's slow, so use it only to seed, never to produce every number.

- It may be deterministic on some platforms. `rd.entropy()` is meant to report this but is unreliable. On this Mac with libc++ it returned 32.
- Seeding `mt19937` with just `rd()` gives it one 32-bit number to fill a 624-word state. That is acceptable for practice and games. For better coverage, use a seed sequence.

**`std::seed_seq`** ([reference](https://en.cppreference.com/w/cpp/numeric/random/seed_seq)) mixes several values into a full state:

```cpp
std::random_device rd;
std::seed_seq seq{rd(), rd(), rd(), rd()};
std::mt19937 gen{seq};
```

**Fixed seed for reproducibility.** Construct with a literal, such as `std::mt19937 gen{42}`. Two engines with the same seed produce identical sequences. This is what you want for tests and debugging.

**Seed once, not repeatedly.** Do not create and seed a new engine for every number or inside a loop. Create it once and reuse it.

## 1.5 Distributions

A distribution is a small object holding its parameters. Calling it with an engine returns the next value.

| Distribution | What it produces |
|---|---|
| [`uniform_int_distribution<T>{a, b}`](https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution) | Integers in the **closed** range `[a, b]`. Both ends are possible. |
| [`uniform_real_distribution<T>{a, b}`](https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution) | Reals in the **half-open** range `[a, b)`. `b` itself is never produced. |
| [`bernoulli_distribution{p}`](https://en.cppreference.com/w/cpp/numeric/random/bernoulli_distribution) | `true` with probability `p`. A weighted coin. |
| [`binomial_distribution`](https://en.cppreference.com/w/cpp/numeric/random/binomial_distribution) | Number of successes in `n` trials. |
| [`geometric_distribution`](https://en.cppreference.com/w/cpp/numeric/random/geometric_distribution) | Number of failures before the first success. |
| [`poisson_distribution`](https://en.cppreference.com/w/cpp/numeric/random/poisson_distribution) | Event counts at a given average rate. |
| [`exponential_distribution`](https://en.cppreference.com/w/cpp/numeric/random/exponential_distribution) | Time between events. |
| [`normal_distribution{mean, stddev}`](https://en.cppreference.com/w/cpp/numeric/random/normal_distribution) | Bell curve. |
| [`lognormal_distribution`](https://en.cppreference.com/w/cpp/numeric/random/lognormal_distribution) | Values whose log is normal. |
| [`discrete_distribution`](https://en.cppreference.com/w/cpp/numeric/random/discrete_distribution) | Picks an index `0..n-1` using weights you give. A loaded die. |
| [`piecewise_constant_distribution`](https://en.cppreference.com/w/cpp/numeric/random/piecewise_constant_distribution), [`piecewise_linear_distribution`](https://en.cppreference.com/w/cpp/numeric/random/piecewise_linear_distribution) | Values from a histogram or a line-segment density. |

The rest, such as gamma, weibull, chi-squared, cauchy, and student-t, are listed on the [reference index](https://en.cppreference.com/w/cpp/numeric/random).

**Weighted choice example:**

```cpp
std::discrete_distribution<int> weighted{{50, 30, 20}};  // index 0: 50%, 1: 30%, 2: 20%
int pick = weighted(gen);                                // 0, 1, or 2
```

**Things to know about distributions:**
- They can hold internal state. `normal_distribution` often generates numbers in pairs and caches one. Create the distribution once and reuse it, rather than making a new one per draw.
- The standard does not specify the algorithm each distribution uses. The **same seed with the same engine can produce different results on different standard libraries** (libc++ vs libstdc++ vs MSVC). Engines are portable, distributions are not.
- `uniform_int_distribution` is not allowed for `char`, `signed char`, `unsigned char`, or `bool`. Use `int` and cast the result.
- Parameters are fixed at construction. To change them, construct a new one or use `dist.param(...)`.

## 1.6 Shuffling and sampling

These take a generator as an argument:

- [`std::shuffle(first, last, gen)`](https://en.cppreference.com/w/cpp/algorithm/random_shuffle) randomly reorders a range. `std::random_shuffle` was removed in C++17.
- [`std::ranges::shuffle(range, gen)`](https://en.cppreference.com/w/cpp/algorithm/ranges/shuffle) is the C++20 version that takes a whole range.
- [`std::sample(first, last, out, n, gen)`](https://en.cppreference.com/w/cpp/algorithm/sample) copies `n` randomly chosen elements into an output. The picked elements keep their relative order, and it's C++17.

## 1.7 The C++20 piece: `uniform_random_bit_generator`

C++20 adds the concept [`std::uniform_random_bit_generator`](https://en.cppreference.com/w/cpp/numeric/random/uniform_random_bit_generator). It describes anything usable as a generator: callable with no arguments, returns an unsigned integer, and has static `min()` and `max()`. Use it to constrain a template parameter instead of an unconstrained `typename G`:

```cpp
template <std::uniform_random_bit_generator G>
int roll(G& gen) {
    std::uniform_int_distribution<int> d{1, 6};
    return d(gen);
}
```

Passing something that isn't a generator then fails with a short, readable constraint error.

## 1.8 Threads

Engines are **not thread-safe**. Do not share one engine between threads without locking. The usual fix is one engine per thread:

```cpp
thread_local std::mt19937 gen{std::random_device{}()};
```

## 1.9 Common mistakes

| Mistake | Fix |
|---|---|
| `rand() % n` | Use `uniform_int_distribution`. |
| Creating and seeding an engine inside a loop or per call | Create it once, pass it by reference. |
| Passing an engine **by value** | You get a copy, so the caller's engine doesn't advance, and repeated calls repeat the same numbers. |
| Seeding with `time(nullptr)` | Two runs in the same second produce the same sequence. Use `random_device`. |
| Expecting `uniform_real_distribution{0, 1}` to ever return `1.0` | The upper bound is exclusive. |
| Expecting `uniform_int_distribution{0, n}` to exclude `n` | The upper bound is inclusive. For a container index use `{0, size - 1}`. |
| Using `uniform_int_distribution<char>` | Not allowed. Use `int`. |
| Expecting identical output across compilers from a distribution | Only engines are portable. |
| Sharing one engine across threads | One engine per thread. |
| Relying on `default_random_engine` for reproducibility | Name the engine explicitly. |

## 1.10 Quick reference

| I want to... | Use |
|---|---|
| A random integer in a range | `uniform_int_distribution<int>{a, b}` |
| A random double in a range | `uniform_real_distribution<double>{a, b}` |
| A coin flip with probability `p` | `bernoulli_distribution{p}` |
| A bell curve | `normal_distribution{mean, stddev}` |
| A weighted pick | `discrete_distribution` |
| A reproducible sequence | `std::mt19937 gen{fixed_seed}` |
| A different sequence each run | Seed from `std::random_device` |
| Shuffle a container | `std::shuffle` or `std::ranges::shuffle` |
| Pick N random elements | `std::sample` |
| Constrain a generic function to take a generator | `std::uniform_random_bit_generator` |

---

# Part 2: Exercises

Write these in a scratch file, such as `src/random.cpp`.

1. **Dice.** Roll a six-sided die 60,000 times and count how many times each face comes up. Print the counts. They should all be close to 10,000. Then roll two dice and print a histogram of the sums 2 through 12.

2. **Seeding.** Make two `mt19937` engines with the same fixed seed and confirm they produce the same ten numbers. Then make one from `random_device` and confirm that two runs of the program differ.

3. **Engine by reference.** Write a function that takes the engine by value and rolls a die, and call it three times with the same engine. Observe the repeated values. Change the parameter to a reference and compare.

4. **Closed vs half-open.** Draw 100,000 values from `uniform_int_distribution{1, 3}` and verify 3 appears. Draw from `uniform_real_distribution{0.0, 1.0}` and track the max. Explain why it can never be exactly 1.0.

5. **Normal.** Draw 100,000 values from `normal_distribution{100, 15}`. Compute the mean and standard deviation yourself and compare them to the parameters. Count how many fall within one standard deviation of the mean (expect about 68%).

6. **Weighted.** Use `discrete_distribution` with weights 50, 30, 20 and verify over 100,000 draws that the counts are in about that ratio.

7. **Shuffle and sample.** Make a vector of the numbers 1 to 10. Shuffle it with `std::shuffle` and again with `std::ranges::shuffle`. Then use `std::sample` to pick three elements and note that they come out in their original relative order.

8. **Concept.** Write a function template that takes a generator constrained with `std::uniform_random_bit_generator` and returns a random int in a range. Pass it an `mt19937`, and then pass it an `int` to see the error message.

9. **Random string.** Write a function that returns a random lowercase string of length N. Remember `uniform_int_distribution<char>` is not allowed.

10. **Threads (stretch).** Start four threads that each draw 1,000 numbers using a `thread_local` engine. Explain why a single shared engine would be a data race.

11. **Portability (stretch).** Print the first five outputs of `mt19937{1}` and the first five from `uniform_int_distribution{0, 99}` using that engine. If you can, compare against a different compiler or standard library. The engine output should match and the distribution output may not.
