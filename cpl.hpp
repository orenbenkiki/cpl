/*

The MIT License (MIT)

Copyright (c) 2015 Oren Ben-Kiki

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/// @file
/// Implement the clever protection library (CPL).

#pragma once

#include <experimental/optional>

#ifndef CPL_WITHOUT_COLLECTIONS

#ifdef CPL_FAST
#include <bitset>
#include <map>
#include <set>
#include <string>
#include <vector>
#endif // CPL_FAST

#ifdef CPL_SAFE
#include <debug/bitset>
#include <debug/map>
#include <debug/set>
#include <debug/string>
#include <debug/vector>
#endif // CPL_SAFE

#endif // CPL_WITHOUT_COLLECTIONS

/// The Git-derived version number.
///
/// To update this, run `make version`. This should be done before every
/// commit. It should arguably be managed by git hooks, but it really isn't
/// that much of a hassle.
#define CPL_VERSION "0.0-dirty"

#ifdef DOXYGEN
/// The Clever Protection Library.
///
/// <em>"Clever people solve problems wise people avoid".</em>
///
/// ## Goals
///
/// CPL tries to achieve the following goals:
///
/// - Efficiency: The program should run at 100% speed without extra overheads.
///
/// - Safety: Pointers should always point to valid memory, vector element
///   indices should be within bounds, map keys should exist, there shouldn't
///   be unexpected aliasing or data races, etc.
///
/// These goals are contradictory. A notable attempt to (partially)
/// simultaneously satisfy both goals is the [Rust
/// language](http://www.rust-lang.org/). Most languages and tools (C, C++,
/// Java, Swift, Go, ...) pick a specific trade-off point (C/C++ focus more on
/// efficiency, Java/Swift/Go focus more on safety).
///
/// CPL accepts that there's no way to achieve both goals at once, so it sets
/// out to achieve them one at a time. That is, CPL allows compiling the same
/// C++ source code for two trade-off points: maximal efficiency, and maximal
/// safety.
///
/// Defining @ref CPL_FAST compiles into a fast version of the collections and
/// the pointers (which is based on raw pointers). This will provide the
/// maximal execution speed, at the cost of ignoring most lifetime/data race
/// errors.
///
/// Defining @ref CPL_SAFE, we compile into a safe version of the collections
/// and the pointers (which is based mainly on `weak_ptr`). This will detect
/// most lifetime/data race errors, at the cost of greatly reduced execution
/// speed (>10x run-time).
///
/// This meshes well with the standard practice of generating a debug and a
/// release version of the same library (or program). When the safe variant
/// detects a problem, it invokes @ref CPL_ASSERT to indicate the problem. By
/// default this is just a call to `assert` but you can `define` it to whatever
/// you want (`throw`, print a stack trace, etc.).
///
/// While this approach is far from perfect, it is lightweight and practical.
/// It also has the benefit of clearly documenting the programmer intent,
/// instead of using raw pointers. Just this benefit is almost worth the hassle
/// of using CPL all on its own.
///
/// ## Usage
///
/// To use CPL, simply include @file cpl.hpp and use the types defined in it.
///
/// ## Types
///
/// CPL provides the following set of types:
///
/// | Type           | May be null? | Data lifetime is as long as        | Fast
/// implementation is based on  |
/// | -------------- | ------------ | ---------------------------------- |
/// -------------------------------  |
/// | `cpl::is<T>`   | No           | The `opt` exists and is not reset  | `T` |
/// | `cpl::opt<T>`  | Yes          | The `is` exists                    |
/// `std::experimental::optional<T>` |
/// | `cpl::uref<T>` | No           | The `uref` exists                  |
/// `std::unique_ptr<T>`             |
/// | `cpl::uptr<T>` | Yes          | The `uptr` exists and is not reset |
/// `std::unique_ptr<T>`             |
/// | `cpl::sref<T>` | No           | The `sref` exists                  |
/// `std::shared_ptr<T>`             |
/// | `cpl::sptr<T>` | Yes          | The `sptr` exists                  |
/// `std::shared_ptr<T>`             |
/// | `cpl::wptr<T>` | Yes          | Some `sptr` exists                 |
/// `std::weak_ptr<T>`               |
/// | `cpl::ref<T>`  | No           | Someone else holds the data        |
/// `std::reference_wrapper`         |
/// | `cpl::ptr<T>`  | Yes          | Someone else holds the data        | `T*`
/// |
///
/// ## Implementation
///
/// The fast implementation has zero cost (assuming optimized compilation). In
/// some cases it is actually faster than the standard implementation (we
/// disable some checks as they will be done in safe mode).
///
/// The safe implementation is very slow (>10x slowdown), but is extremely safe
/// and will helpfully report (m)any memory issues in the earliest possible
/// moment. Most of the time it will point directly at the buggy line, making
/// it much easier to debug the code.
///
/// ## Interface
///
/// All the pointer-like types provide the usual `operator*`, `operator->` and
/// `operator bool` operators, as well as `get`, `value`, and `value_or` (which
/// behaves in the same way as for `std::experimental::optional`). The
/// `const`-ness of the pointer-like does not effect the `const`-ness of the
/// result (except for `opt` which holds the data internally).
///
/// All the reference-like types also provide `operator*`, `operator->` and
/// `operator T&`, as well as `get` and `value`. The `const`-ness of the
/// reference does effect the `const-ness of the result.
///
/// The interface of `wptr` is identical to `std::weak_ptr`, that is, it isn't
/// really a pointer but it allows to obtain an `sptr` (which may be null).
///
/// Having a uniform interface for all the types makes it more convenient to
/// interchangeably use the different types. Providing `operator->` for
/// references seems strange, but as long as we can't override `operator.` then
/// this is the only way to get convenient access to the data members of the
/// value being referred to.
///
/// ## Guidelines
///
/// The use of the raw `T&` should be reserved for function arguments when the
/// function does not store away a reference somewhere that outlives the
/// function invocation. When the argument is optional, `T*` may be used (this
/// should be very rare).
///
/// Otherwise (in data members, static variables, etc.) the CPL data types
/// should be used. Using `T*` and `T&` may be needed when interfacing with
/// non-CPL code.
///
/// CPL managed data should reside inside some CPL type. This means that it
/// needs to be created using `make_uptr`, `make_uref`, `make_sptr`,
/// `make_sref` or be held inside a normally-constructed `opt` or an `is`
/// object. Having to use the `is` type instead of a simple `T` is an
/// unfortunate price we have to pay to allow the safe implementation to track
/// the lifetime of the object and detect dangling pointers to it after it is
/// destroyed. As a consolation it provide the advantage that all objects that
/// have long-lasting pointers to them are clearly marked as such as the code.
///
/// It is possible to use `unsafe_ptr` and `unsafe_ref` to refer to arbitrary
/// data. This is only safe when the data is `static`; CPL will not be able to
/// detect invalid pointers to such data if it goes out of scope or is
/// otherwise deleted. Alas, it is not possible to express the "this data is
/// `static`" constraint in C++, so it is impossible for CPL to distinguish the
/// safe and unsafe cases.
///
/// ## Collections
///
/// Unless @ref CPL_WITHOUT_COLLECTIONS is defined, then CPL will provide the
/// `cpl::bitset`, `cpl::map`, `cpl::set`, `cpl::string` and `cpl::vector`
/// types. These will compile to the standard versions in fast mode and to the
/// (G++ specific) debug versions in safe mode.
///
/// Using these types instead of the `std` types will provide additional checks
/// in safe mode, detecting out-of-bounds and similar errors, while having zero
/// impact on the fast mode.
///
/// ## Caveats
///
/// The implementation is naive in many ways. Writing an STL-level library in
/// modern C++ is a daunting task: issues such as `constexpr`, `noexcept` and
/// perfect forwarding are fiendishly difficult to get right. Even simple
/// `const`-ness is tricky due to the existence of both `const ref<T>` and
/// `ref<const T>`.
///
/// It is also difficult to verify the library forbids all sort of invalid code
/// (even though @ref MUST_NOT_COMPILE helps a bit). In general CPL is relaxed
/// about this because the safe version will detect any foul play at run-time.
///
/// As always, YMMV, no warranty is implied, may contain nuts, etc.
namespace cpl {
  /// Implement the fast (unsafe) variant of the clever protection library
  /// (CPL).
  namespace fast {};
  /// Implement the safe (slow) variant of the clever protection library
  /// (CPL).
  namespace safe {};
};

/// The name of the CPL variant we are compiling
#define CPL_VARIANT

/// If this is defined, the fast (unsafe) variant will be compiled.
#define CPL_FAST

/// If this is defined, the safe (slow) variant will be compiled.
#define CPL_SAFE

/// If this is defined, do not provide the `cpl` version of the standard
/// collections, and do not even include their header files.
#define CPL_WITHOUT_COLLECTIONS

#else // DOXYGEN

// Allow using `cpl::foo` to access either `cpl::safe::foo` or `cpl::fast::foo`
// depending on which of @ref CPL_FAST or @ref CPL_SAFE were defined, and
// define @ref CPL_VARIANT accordingly.

#ifdef CPL_FAST

#ifdef CPL_SAFE
#error "Both CPL_FAST and CPL_SAFE are defined"
#endif // CPL_SAFE

#define CPL_VARIANT "fast"

namespace cpl {
  namespace fast {};
  using namespace fast;
};

#else // CPL_FAST

#ifdef CPL_SAFE
#define CPL_VARIANT "safe"
namespace cpl {
  namespace safe {};
  using namespace safe;
}
#else // CPL_SAFE
#error "No CPL variant chosen - neither CPL_FAST nor CPL_SAFE are defined"
#endif // CPL_SAFE

#endif // CPL_FAST

#endif // DOXYGEN

#ifndef CPL_ASSERT
/// Perform a run-time verification.
///
/// By default, if the condition is false, this throws a generic
/// `std::logic_error` with the specified message. Making this a macro allows
/// user code to override it with a custom mechanism.
#define CPL_ASSERT(CONDITION, MESSAGE) \
  if (CONDITION) {                     \
  } else                               \
  throw(std::logic_error(MESSAGE))
#endif // CPL_ASSERT

namespace cpl {
  namespace fast {}

  namespace safe {}

#ifndef CPL_WITHOUT_COLLECTIONS

#ifdef CPL_FAST
  /// Compiles to the standard version of a bitset.
  template <size_t N> using bitset = std::bitset<N>;

  /// Compiles to the standard version of a map.
  ///
  /// @todo Should we convert `at` to a faster, unchecked operation?
  template <typename K, typename T, typename C = std::less<K>, typename A = std::allocator<std::pair<const K, T>>>
  using map = std::map<K, T, C, A>;

  /// Compiles to the standard version of a set.
  template <typename T, typename C = std::less<T>, typename A = std::allocator<T>> using set = std::set<T, C, A>;

  /// Compiles to the standard version of a string.
  using string = std::string;

  /// Compiles to the standard version of a vector.
  ///
  /// @todo Should we convert `at` to a faster, unchecked operation?
  template <typename T, typename A = std::allocator<T>> using vector = std::vector<T, A>;
#endif // CPL_FAST

#ifdef CPL_SAFE
  /// Compiles to the debug version of a bitset.
  template <size_t N> using bitset = __gnu_debug::bitset<N>;

  /// Compiles to the debug version of a map.
  template <typename K, typename T, typename C = std::less<K>, typename A = std::allocator<std::pair<const K, T>>>
  using map = __gnu_debug::map<K, T, C, A>;

  /// Compiles to the debug version of a set.
  template <typename T, typename C = std::less<T>, typename A = std::allocator<T>> using set = __gnu_debug::set<T, C, A>;

  /// Compiles to the debug version of a string.
  using string = __gnu_debug::string;

  /// Compiles to the debug version of a vector.
  template <typename T, typename A = std::allocator<T>> using vector = __gnu_debug::vector<T, A>;
#endif // CPL_SAFE

#endif // CPL_WITHOUT_COLLECTIONS
}
