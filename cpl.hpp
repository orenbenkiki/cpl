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
#define CPL_VERSION "0.0.2"

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
/// | Type           | May be null? | Data lifetime is as long as        | Fast implementation is based on  |
/// | -------------- | ------------ | ---------------------------------- | -------------------------------  |
/// | `cpl::is<T>`   | No           | The `opt` exists and is not reset  | `T`                              |
/// | `cpl::opt<T>`  | Yes          | The `is` exists                    | `std::experimental::optional<T>` |
/// | `cpl::uref<T>` | No           | The `uref` exists                  | `std::unique_ptr<T>`             |
/// | `cpl::uptr<T>` | Yes          | The `uptr` exists and is not reset | `std::unique_ptr<T>`             |
/// | `cpl::sref<T>` | No           | The `sref` exists                  | `std::shared_ptr<T>`             |
/// | `cpl::sptr<T>` | Yes          | The `sptr` exists                  | `std::shared_ptr<T>`             |
/// | `cpl::wptr<T>` | Yes          | Some `sptr` exists                 | `std::weak_ptr<T>`               |
/// | `cpl::ref<T>`  | No           | Someone else holds the data        | `std::reference_wrapper`         |
/// | `cpl::ptr<T>`  | Yes          | Someone else holds the data        | `T*`                             |
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
/// ## Casting
///
/// The `cpl::cast_clever` function works similarly to `std::dynamic_cast`, but
/// it is faster since it assumes that the pointer value does not change (this
/// assumption is checked in safe mode, of course).
///
/// If you are using virtual base classes etc. you'll need to use the real
/// `cpl::cast_dynamic` function. CPL also provides the `cpl::cast_const`
/// and `cpl::cast_reinterpret` functions.
///
/// The names are all reversed since the normal names are keywords making them
/// impossible to use as function names.
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
  ///
  /// This namespace only exists in Doxygen's imagination. In reality we
  /// compile the correct variant directly into the @ref cpl namespace. Using
  /// a fake nested namespace allows us to document both variants together.
  namespace fast {};

  /// Implement the safe (slow) variant of the clever protection library
  /// (CPL).
  ///
  /// This namespace only exists in Doxygen's imagination. In reality we
  /// compile the correct variant directly into the @ref cpl namespace. Using
  /// a fake nested namespace allows us to document both variants together.
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

// Ensure one of @ref CPL_FAST or @ref CPL_SAFE are defined, and configure the
// @ref CPL_VARIANT accordingly.

#ifdef CPL_FAST

#ifdef CPL_SAFE
#error "Both CPL_FAST and CPL_SAFE are defined"
#endif // CPL_SAFE

#define CPL_VARIANT "fast"

#else // CPL_FAST

#ifdef CPL_SAFE
#define CPL_VARIANT "safe"
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

  /// An additional parameter for unsafe raw pointer operations.
  ///
  /// This allows us to use @ref MUST_NOT_COMPILE for verifying unsafe
  /// constructions are indeed forbidden.
  enum unsafe_raw_t {};

  /// An additional parameter for unsafe static cast operations.
  ///
  /// This allows us to use @ref MUST_NOT_COMPILE for verifying unsafe
  /// constructions are indeed forbidden.
  enum unsafe_static_t {};

  /// An additional parameter for unsafe dynamic cast operations.
  ///
  /// This allows us to use @ref MUST_NOT_COMPILE for verifying unsafe
  /// constructions are indeed forbidden.
  enum unsafe_dynamic_t {};

  /// An additional parameter for unsafe const cast operations.
  ///
  /// This allows us to use @ref MUST_NOT_COMPILE for verifying unsafe
  /// constructions are indeed forbidden.
  enum unsafe_const_t {};

#ifdef DOXYGEN
  namespace fast {
#endif // DOXYGEN

#if defined(DOXYGEN) || defined(CPL_FAST)
    /// A fast (unsafe) reference for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ref {
      /// The raw pointer to the value.
      T* m_raw_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      ref(T* raw_ptr, unsafe_raw_t) : m_raw_ptr(raw_ptr) {
      }

      /// Unsafe construction from a different type of reference.
      template <typename U> ref(ref<U> other, unsafe_raw_t) : m_raw_ptr(reinterpret_cast<T*>(other.get())) {
      }

      /// Unsafe construction from a different type of reference.
      template <typename U> ref(ref<U> other, unsafe_static_t) : m_raw_ptr(static_cast<T*>(other.get())) {
      }

      /// Unsafe construction from a different type of reference.
      template <typename U> ref(ref<U> other, unsafe_dynamic_t) : m_raw_ptr(dynamic_cast<T*>(other.get())) {
      }

      /// Unsafe construction from a different type of reference.
      template <typename U> ref(ref<U> other, unsafe_const_t) : m_raw_ptr(const_cast<T*>(other.get())) {
      }

      /// Copy a reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      ref(ref<U> other)
        : m_raw_ptr(other.get()) {
      }

      /// Access the raw pointer.
      T* get() const {
        return m_raw_ptr;
      }
    };

    /// A fast (unsafe) pointer for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ptr {
      /// The raw pointer to the value.
      T* m_raw_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      ptr(T* raw_ptr, unsafe_raw_t) : m_raw_ptr(raw_ptr) {
      }

      /// Unsafe construction from a different type of pointer.
      template <typename U> ptr(ptr<U> other, unsafe_raw_t) : m_raw_ptr(reinterpret_cast<T*>(other.get())) {
      }

      /// Unsafe construction from a different type of pointer.
      template <typename U> ptr(ptr<U> other, unsafe_static_t) : m_raw_ptr(static_cast<T*>(other.get())) {
      }

      /// Unsafe construction from a different type of pointer.
      template <typename U> ptr(ptr<U> other, unsafe_dynamic_t) : m_raw_ptr(dynamic_cast<T*>(other.get())) {
      }

      /// Unsafe construction from a different type of pointer.
      template <typename U> ptr(ptr<U> other, unsafe_const_t) : m_raw_ptr(const_cast<T*>(other.get())) {
      }

      /// Deterministic null default constructor.
      ///
      /// This is slower from the `T*` default constructor, for sanity.
      ptr() : m_raw_ptr(nullptr) {
      }

      /// Explicit null constructor.
      ptr(nullptr_t) : ptr() {
      }

      /// Copy a pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      ptr(ptr<U> other)
        : m_raw_ptr(other.get()) {
      }

      /// Access the raw pointer.
      T* get() const {
        return m_raw_ptr;
      }
    };
#endif // DOXYGEN || CPL_FAST

#ifdef DOXYGEN
  }

  namespace safe {
#endif // DOXYGEN

#if defined(DOXYGEN) || defined(CPL_SAFE)
    /// A `Deleter` that doesn't delete the object.
    ///
    /// We use this for the `std::shared_ptr` we attach to local data to ensure
    /// there are no dangling pointers when it is destroyed.
    template <typename T> struct no_delete {
      /// Default constructor.
      no_delete() noexcept = default;

      /// Copy constructor.
      template <typename U> no_delete(const no_delete<U>&) noexcept {
      }

      /// (Do not) delete the given pointer.
      void operator()(T*) const noexcept {
      }
    };

    /// Cast the shared pointer of a `ptr` of a different type.
    template <typename T, typename U> static std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_raw_t) {
      typedef typename std::shared_ptr<T>::element_type E;
      E* other_raw = reinterpret_cast<E*>(other.get());
      return std::shared_ptr<T>(other, other_raw);
    }

    /// Cast the shared pointer of a `ptr` of a different type.
    template <typename T, typename U> static std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_static_t) {
      typedef typename std::shared_ptr<T>::element_type E;
      E* other_raw = static_cast<E*>(other.get());
      return std::shared_ptr<T>(other, other_raw);
    }

    /// Cast the shared pointer of a `ptr` of a different type.
    template <typename T, typename U> static std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_dynamic_t) {
      typedef typename std::shared_ptr<T>::element_type E;
      E* other_raw = dynamic_cast<E*>(other.get());
      return std::shared_ptr<T>(other, other_raw);
    }

    /// Cast the shared pointer of a `ptr` of a different type.
    template <typename T, typename U> static std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_const_t) {
      typedef typename std::shared_ptr<T>::element_type E;
      E* other_raw = const_cast<E*>(other.get());
      return std::shared_ptr<T>(other, other_raw);
    }

    /// Cast the weak pointer of a `ptr` of a different type.
    template <typename T, typename U, typename C>
    std::weak_ptr<T> cast_weak_ptr(const std::shared_ptr<T>& into_unsafe_ptr, const std::weak_ptr<U>& from_weak_ptr, C cast_type) {
      if (into_unsafe_ptr) {
        return into_unsafe_ptr;
      } else {
        return cast_shared_ptr<T>(from_weak_ptr.lock(), cast_type);
      }
    }

    /// A safe (slow) reference for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ref {
      template <typename U> friend class ref;

      /// If we hold a pointer created by `unsafe_ref`, then this will
      /// provide a lifetime to `m_weak_ptr`.
      std::shared_ptr<T> m_unsafe_ptr;

      /// A weak pointer to the track the lifetime of the data.
      std::weak_ptr<T> m_weak_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      ref(T* raw_ptr, unsafe_raw_t) : m_unsafe_ptr(raw_ptr, no_delete<T>()) {
      }

      /// Unsafe construction from a different type of reference.
      template <typename U, typename C>
      ref(const ref<U>& other, C cast_type)
        : m_unsafe_ptr(cast_shared_ptr<T, U>(other.m_unsafe_ptr, cast_type)),
          m_weak_ptr(cast_weak_ptr(m_unsafe_ptr, other.m_weak_ptr, cast_type)) {
      }

      /// Copy a reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      ref(const ref<U>& other)
        : m_unsafe_ptr(other.m_unsafe_ptr ? std::shared_ptr<T>(other.m_unsafe_ptr.get(), no_delete<T>())
                                          : std::shared_ptr<T>(nullptr)),
          m_weak_ptr(m_unsafe_ptr ? m_unsafe_ptr : other.m_weak_ptr.lock()) {
      }

      /// Access the raw pointer.
      ///
      /// This isn't as safe as we'd like it to be, since another thread may
      /// delete the value between the time we `return` and the time the caller
      /// uses the value.
      T* get() const {
        T* raw_ptr = m_weak_ptr.lock().get();
        CPL_ASSERT(raw_ptr, "accessing a null reference");
        return raw_ptr;
      }
    };

    /// A safe (slow) pointer for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ptr {
      template <typename U> friend class ptr;

      /// If we hold a pointer created by `unsafe_ptr`, then this will
      /// provide a lifetime to `m_weak_ptr`.
      std::shared_ptr<T> m_unsafe_ptr;

      /// A weak pointer to the track the lifetime of the data.
      std::weak_ptr<T> m_weak_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      ptr(T* raw_ptr, unsafe_raw_t) : m_unsafe_ptr(raw_ptr, no_delete<T>()) {
      }

      /// Unsafe construction from a different type of pointer.
      template <typename U, typename C>
      ptr(const ptr<U>& other, C cast_type)
        : m_unsafe_ptr(cast_shared_ptr<T, U>(other.m_unsafe_ptr, cast_type)),
          m_weak_ptr(cast_weak_ptr(m_unsafe_ptr, other.m_weak_ptr, cast_type)) {
      }

      /// Deterministic null default constructor.
      ptr() : m_unsafe_ptr(), m_weak_ptr() {
      }

      /// Explicit null constructor.
      ptr(nullptr_t) : ptr() {
      }

      /// Copy a pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      ptr(const ptr<U>& other)
        : m_unsafe_ptr(other.m_unsafe_ptr ? std::shared_ptr<T>(other.m_unsafe_ptr.get(), no_delete<T>())
                                          : std::shared_ptr<T>(nullptr)),
          m_weak_ptr(m_unsafe_ptr ? m_unsafe_ptr : other.m_weak_ptr.lock()) {
      }

      /// Access the raw pointer.
      ///
      /// This isn't as safe as we'd like it to be, since another thread may
      /// delete the value between the time we `return` and the time the caller
      /// uses the value.
      T* get() const {
        return m_weak_ptr.lock().get();
      }
    };
#endif // DOXYGEN || CPL_SAFE

#ifdef DOXYGEN
  }
#endif // DOXYGEN

  /// @file
  /// Implement the unsafe creation of pointers and references.inters and
  /// references.

  /// Create an unsafe reference to raw data.
  ///
  /// This is playing with fire. It is OK if the data is static, but there's
  /// no way to ask the compiler to ensure that.
  template <typename T> ref<T> unsafe_ref(T& data) {
    return ref<T>{ &data, unsafe_raw_t(0) };
  }

  /// Create an unsafe pointer to raw data.
  ///
  /// This is playing with fire. It is OK if the data is static, but there's
  /// no way to ask the compiler to ensure that.
  template <typename T> ptr<T> unsafe_ptr(T& data) {
    return ptr<T>{ &data, unsafe_raw_t(0) };
  }

#ifdef DOXYGEN
  namespace fast {
#endif // DOXYGEN

#if defined(DOXYGEN) || defined(CPL_FAST)
    /// A static cast between reference types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    ref<T> cast_static(ref<U> from_ref) {
      return ref<T>{ from_ref, unsafe_static_t(0) };
    }

    /// A static cast between pointer types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    ptr<T> cast_static(ptr<U> from_ptr) {
      return ptr<T>{ from_ptr, unsafe_static_t(0) };
    }
#endif // DOXYGEN || CPL_FAST

#ifdef DOXYGEN
  }

  namespace safe {
#endif // DOXYGEN

#if defined(DOXYGEN) || defined(CPL_SAFE)
    /// Static cast between related reference types.
    ///
    /// This verifies that the raw pointer value did not change, which will
    /// always be true unless you use virtual base classes.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    ref<T> cast_static(ref<U> from_ref) {
      U* from_raw = from_ref.get();
      T* to_dynamic = dynamic_cast<T*>(from_raw);
      T* to_raw = static_cast<T*>(from_raw);
      CPL_ASSERT(to_dynamic == to_raw, "static cast gave the wrong result");
      return ref<T>{ from_ref, unsafe_raw_t(0) };
    }

    /// Static cast between related pointer types.
    ///
    /// This verifies that the pointer value did not change, which will
    /// always be true unless you use virtual base classes.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    ptr<T> cast_static(ptr<U> from_ptr) {
      U* from_raw = from_ptr.get();
      T* to_dynamic = dynamic_cast<T*>(from_raw);
      T* to_raw = static_cast<T*>(from_raw);
      CPL_ASSERT(to_dynamic == to_raw, "static cast gave the wrong result");
      return ptr<T>{ from_ptr, unsafe_raw_t(0) };
    }
#endif // DOXYGEN || CPL_SAFE

#ifdef DOXYGEN
  }
#endif // DOXYGEN

  /// A reinterpret cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  ref<T> cast_reinterpret(ref<U> from_ref) {
    return ref<T>{ from_ref, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  ptr<T> cast_reinterpret(ptr<U> from_ptr) {
    return ptr<T>{ from_ptr, unsafe_raw_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  ref<T> cast_dynamic(ref<U> from_ref) {
    return ref<T>{ from_ref, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  ptr<T> cast_dynamic(ptr<U> from_ptr) {
    return ptr<T>{ from_ptr, unsafe_dynamic_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  ref<T> cast_const(ref<U> from_ref) {
    return ref<T>{ from_ref, unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  ptr<T> cast_const(ptr<U> from_ptr) {
    return ptr<T>{ from_ptr, unsafe_const_t(0) };
  }

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
