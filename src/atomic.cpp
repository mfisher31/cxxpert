#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

// ============================================================================
// std::atomic — guided exercises
// ============================================================================
// Same format as the other lesson files: comments explain mechanics, TASKs
// tell you what to write, nothing is implemented for you. Memory-order
// basics (relaxed/acquire/release/seq_cst) were already covered in
// semaphore.cpp — this file assumes you remember that and focuses on the
// atomic-specific operations, especially compare_exchange.
// ============================================================================


// ----------------------------------------------------------------------------
// PART 1 — What std::atomic<T> actually guarantees
// ----------------------------------------------------------------------------
// std::atomic<T> guarantees that operations ON THAT SINGLE OBJECT are
// indivisible — no thread can ever observe a "torn" half-written value, and
// there's no data race on that object even with concurrent reads/writes
// from multiple threads without a mutex. It says NOTHING about any other
// variable; atomicity does not extend to "this atomic plus some other
// regular variable I update near it."
//
// Core operations:
//   a.load(order)         -> T       read the current value
//   a.store(v, order)     -> void    write a new value
//   a.fetch_add(n, order) -> T       atomically add n, RETURNS THE OLD value
//   ++a / a++ / a += n              convenience wrappers around fetch_add
//   a.is_lock_free()      -> bool    true if implemented without a mutex
//                                    (true for int/bool/pointer-sized types
//                                     on virtually every real platform)
//
// Every one of these takes an optional std::memory_order argument, defaulting
// to memory_order_seq_cst if you don't pass one.
//
// TASK:
//   Declare `std::atomic<int> counter{0};`. Spawn 4 threads, each of which
//   calls `counter.fetch_add(1)` 10000 times in a loop. Join all 4, then
//   print counter.load() and confirm it's exactly 40000. This is just to
//   see atomicity do its job: try it once more with a PLAIN `int counter`
//   (no atomic) incremented via `++counter` instead, and watch the final
//   value come out wrong/inconsistent across runs — that's the data race
//   std::atomic exists to prevent.


// ----------------------------------------------------------------------------
// PART 2 — exchange(): atomic "set new value, give me back the old one"
// ----------------------------------------------------------------------------
// a.exchange(new_value, order) atomically does two things as ONE indivisible
// step: it stores new_value into a, AND returns whatever a's value was
// immediately before the store. No other thread can observe or act on a
// value that exists "between" the read and the write, because there is no
// such gap — it's a single atomic operation, not a load followed by a
// store.
//
// This gives you a clean "consume a signal exactly once" idiom with a
// single atomic<bool>:
//
//   if (flag.exchange(false)) { /* act */ }
//
// Read that carefully: this call ALWAYS sets flag to false, no matter what
// it reads. The `if` only fires if the value exchange() returns (i.e. the
// value flag had right before this call) was true. So: "whoever happens to
// call exchange() while flag is true gets to act, and the act of checking
// simultaneously clears it for everyone else" — one atomic instruction does
// both the check and the clear.
//
// TASK:
//   Declare `std::atomic<bool> ready{false};`. From a second thread, after
//   sleeping 50ms, call `ready.store(true)`. On the main thread, spin in a
//   loop calling `ready.exchange(false)` repeatedly (sleep ~5ms between
//   checks) until it returns true, then print "saw it" and break. This
//   proves exchange() is doing a real read of the previous value, not just
//   blindly writing false every time.


// ----------------------------------------------------------------------------
// PART 3 — compare_exchange: the real CAS primitive, and its gotchas
// ----------------------------------------------------------------------------
// Signature (strong form):
//   bool a.compare_exchange_strong(T& expected, T desired,
//                                  memory_order success = seq_cst,
//                                  memory_order failure = seq_cst);
//
// Mechanically, as ONE atomic step:
//   - Compare a's CURRENT value against `expected`.
//   - If they are EQUAL: set a's value to `desired`. Return true.
//   - If they are NOT equal: do NOT touch a at all. Instead, OVERWRITE
//     `expected` (your local variable, passed by reference!) with a's
//     actual current value. Return false.
//
// That second bullet is the thing almost everyone misses the first time:
// `expected` is an IN/OUT parameter. On failure, the call doesn't just
// report "nope" — it hands you the real current value for free, already
// written into the variable you passed in, so you don't need a separate
// load() to find out what it actually was.
//
// This is what makes CAS the building block for lock-free "read, compute a
// new value from what you read, then commit it ONLY IF nobody else changed
// it out from under you in the meantime" logic — the classic retry loop:
//
//   T expected = a.load();
//   T desired;
//   do {
//       desired = /* compute new value from `expected` */;
//   } while (!a.compare_exchange_weak(expected, desired));
//
// Notice the loop never manually re-reads `a` on retry — a failed CAS
// already refreshed `expected` with the current value as a side effect, so
// the next loop iteration computes `desired` from up-to-date data
// automatically.
//
// weak vs strong: compare_exchange_WEAK may return false even when a's
// value WAS equal to expected — a "spurious failure." This happens on
// platforms that implement CAS via load-linked/store-conditional
// instructions (notably ARM), where the store-conditional step can fail for
// reasons unrelated to the value itself (e.g. an interrupt landed between
// the load-linked and the store). compare_exchange_STRONG never spuriously
// fails — internally it's allowed to retry the weak version in a loop until
// it gets a real answer, which costs a little extra on those platforms.
// Rule of thumb: use `weak` when you're already inside a retry loop (a
// spurious failure just costs one harmless extra iteration); use `strong`
// everywhere else, especially for a one-shot check you're not looping on.
//
// TASK:
//   Write `bool increment_if_below(std::atomic<int>& a, int cap)` that
//   atomically increments `a` by 1 and returns true, but ONLY if doing so
//   would keep it <= cap; if a is already at cap, it returns false and
//   leaves a untouched. You cannot do this with fetch_add alone — fetch_add
//   always adds, unconditionally, and you'd have to separately check/undo
//   after the fact, which isn't atomic as a pair. Use the retry-loop pattern
//   above with compare_exchange_weak: read the current value into
//   `expected`, compute `expected + 1` as `desired` only if that's <= cap
//   (return false immediately if not), and CAS it in, retrying on failure.
//
// TEST: spin up several threads all calling increment_if_below(counter, 100)
// in a loop as fast as they can; confirm counter never exceeds 100 no
// matter how many threads race on it.


// ----------------------------------------------------------------------------
// PART 4 — Capstone: realtime-thread-to-UI-thread signaling with CAS
// ----------------------------------------------------------------------------
// This is the pattern you asked about — the one JUCE (and basically every
// realtime audio framework) relies on: an audio callback running on a
// realtime thread must NEVER block (no mutex, no heap allocation, no
// syscalls that might block), so the only safe way for it to tell another
// thread "a parameter changed, go update the UI" is through atomics.
//
// The shape:
//   std::atomic<float> latestValue;              // the actual data
//   std::atomic<bool>  pending{false};           // the "something changed" signal
//
// Realtime thread, whenever the parameter changes:
//   latestValue.store(v, std::memory_order_relaxed);
//   pending.store(true, std::memory_order_release);
//
// Why release on the flag specifically (recall PART 2/3 of semaphore.cpp's
// memory-order lesson): a consumer that later reads `pending` with an
// ACQUIRE load is then guaranteed to see the latestValue write that
// happened-before this release store, even though latestValue itself was
// written with a cheap relaxed store. The flag's acquire/release pairing is
// what makes the VALUE's relaxed store safe to read afterward.
//
// Now, the part that actually needs compare_exchange rather than plain
// exchange(): suppose there could be MORE THAN ONE consumer trying to claim
// a pending update — e.g. a UI-refresh timer callback and a separate
// analytics/logging consumer, both polling the same flag. You want AT MOST
// ONE of them to "win" and act on any given signal; the other should see
// nothing pending.
//
// A naive two-step "load then store" is NOT safe for this, even though each
// individual call is atomic:
//   if (pending.load()) { ...read value, act...; pending.store(false); }
// Between your load() returning true and your store(false) executing,
// ANOTHER thread's load() can also return true and ALSO act on the same
// signal — two separate atomic calls are not atomic AS A PAIR, so a window
// exists where both consumers believe they won.
//
// compare_exchange collapses "check it's true, and if so set it false" into
// ONE atomic step, so only whichever thread's CAS call happens to execute
// first can possibly succeed:
//   bool expected = true;
//   if (pending.compare_exchange_strong(expected, false,
//                                       std::memory_order_acquire,
//                                       std::memory_order_relaxed)) {
//       float v = latestValue.load(std::memory_order_relaxed);
//       // act on v — exactly one thread reaches here per signal
//   }
//
// TASK:
//   1. Set up `std::atomic<float> latestValue` and `std::atomic<bool> pending`
//      as above.
//   2. Simulate the realtime thread: a std::thread that, in a loop of 20
//      iterations with a short sleep between each, stores an incrementing
//      value into latestValue (relaxed) then `pending.store(true, release)`.
//   3. Simulate TWO independent consumer threads, each spinning in a tight
//      loop (with a short sleep so you're not pegging a core) that attempts
//      the compare_exchange_strong claim shown above. When a consumer wins
//      a claim, have it print which thread won and what value it saw, and
//      increment a shared `std::atomic<int> totalClaims`.
//   4. After joining all three threads, assert/print that `totalClaims`
//      never exceeds the number of times the producer actually set
//      `pending` to true, and that no value was ever "claimed" twice.
//   5. As a sanity check, try temporarily replacing the CAS in step 3 with
//      the naive `if (pending.load()) { ...; pending.store(false); }` and
//      see if you can get totalClaims to occasionally double-count a
//      single signal when both consumer threads are spinning aggressively
//      (tight loop, no sleep) — this is the race CAS exists to close.


// ----------------------------------------------------------------------------
// main — uncomment/call as you complete each part
// ----------------------------------------------------------------------------
int main() {
    // PART 1

    // PART 2

    // PART 3

    // PART 4

    return 0;
}
