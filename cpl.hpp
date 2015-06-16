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
#define CPL_VERSION "0.0.11"

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
/// CPL provides `cpl::cast_static`, `cpl::cast_dynamic`,
/// `cpl::cast_reinterpret` and `cpl::cast_const` which work on all the CPL
/// types (except for `cpl::wptr`). The safe `cpl::cast_static` verifies that
/// it gives the same results as `cpl::cast_dynamic`. This will only fail if
/// there are virtual base classes involved, in which case you should use the
/// more costly `cpl::cast_dynamic`. Other than this, CPL lets you freely shoot
/// yourself in the foot using the casts.
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

  /// Cast a unique pointer to a different type.
  template <typename T, typename U> inline std::unique_ptr<T> cast_unique_ptr(std::unique_ptr<U>&& other, unsafe_raw_t) {
    return std::unique_ptr<T>(reinterpret_cast<T*>(other.release()));
  }

  /// Cast a unique pointer to a different type.
  template <typename T, typename U> inline std::unique_ptr<T> cast_unique_ptr(std::unique_ptr<U>&& other, unsafe_static_t) {
    return std::unique_ptr<T>(static_cast<T*>(other.release()));
  }

  /// Cast a unique pointer to a different type.
  template <typename T, typename U> inline std::unique_ptr<T> cast_unique_ptr(std::unique_ptr<U>&& other, unsafe_dynamic_t) {
    return std::unique_ptr<T>(dynamic_cast<T*>(other.release()));
  }

  /// Cast a unique pointer to a different type.
  template <typename T, typename U> inline std::unique_ptr<T> cast_unique_ptr(std::unique_ptr<U>&& other, unsafe_const_t) {
    return std::unique_ptr<T>(const_cast<T*>(other.release()));
  }

  /// Cast a shared pointer to a different type.
  template <typename T, typename U> inline std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_raw_t) {
    return std::shared_ptr<T>(other, reinterpret_cast<T*>(other.get()));
  }

  /// Cast a shared pointer to a different type.
  template <typename T, typename U> inline std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_static_t) {
    return std::shared_ptr<T>(other, static_cast<T*>(other.get()));
  }

  /// Cast a shared pointer to a different type.
  template <typename T, typename U> inline std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_dynamic_t) {
    return std::shared_ptr<T>(other, dynamic_cast<T*>(other.get()));
  }

  /// Cast a shared pointer to a different type.
  template <typename T, typename U> inline std::shared_ptr<T> cast_shared_ptr(const std::shared_ptr<U>& other, unsafe_const_t) {
    return std::shared_ptr<T>(other, const_cast<T*>(other.get()));
  }

  /// A fast (unsafe) shared indirection that uses reference counting.
  template <typename T> class shared : public std::shared_ptr<T> {
    template <typename U> friend class shared;

  public:
    /// Unsafe construction from a raw pointer.
    inline shared(T* raw_ptr, unsafe_raw_t) : std::shared_ptr<T>(raw_ptr) {
    }

    /// Cast construction from a different type of shared indirection.
    template <typename U, typename C>
    inline shared(const shared<U>& other, C cast_type)
      : std::shared_ptr<T>(cast_shared_ptr<T, U>(other, cast_type)) {
    }

    /// Copy construction from a compatible type of shared indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline shared(const shared<U>& other)
      : std::shared_ptr<T>(other) {
    }
  };

  /// A fast (unsafe) pointer that uses reference counting.
  template <typename T> class sptr : public shared<T> {
    using shared<T>::shared;

  public:
    /// Null default constructor.
    inline sptr() : shared<T>(nullptr, unsafe_raw_t(0)) {
    }

    /// Explicit null constructor.
    inline sptr(nullptr_t) : sptr() {
    }
  };

  /// A fast (unsafe) reference that uses reference counting.
  template <typename T> class sref : public shared<T> {
  public:
    /// Unsafe construction from a raw pointer.
    inline sref(T* raw_ptr, unsafe_raw_t) : shared<T>(raw_ptr, unsafe_raw_t(0)) {
    }

    /// Cast construction from a different type of shared indirection.
    template <typename U, typename C> inline sref(const shared<U>& other, C cast_type) : shared<T>(other, cast_type) {
    }

    /// Copy a reference.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sref(const sref<U>& other)
      : shared<T>(other) {
    }

    /// Copy a pointer.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    explicit inline sref(const sptr<U>& other)
      : shared<T>(other) {
    }

    /// Access the raw pointer.
    inline const T* get() const {
      return shared<T>::get();
    }

    /// Access the raw pointer.
    inline T* get() {
      return shared<T>::get();
    }
  };

#ifdef DOXYGEN
  namespace fast {
#endif // DOXYGEN

#if defined(DOXYGEN) || defined(CPL_FAST)
    /// A fast (unsafe) indirection that deletes the data when it is deleted.
    template <typename T> class unique : public std::unique_ptr<T> {
      template <typename U> friend class unique;

    public:
      /// Unsafe construction from a raw pointer.
      inline unique(T* raw_ptr, unsafe_raw_t) : std::unique_ptr<T>(raw_ptr) {
      }

      /// Cast construction from a different type of unique indirection.
      template <typename U, typename C>
      inline unique(unique<U>&& other, C cast_type)
        : std::unique_ptr<T>(cast_unique_ptr<T, U>(std::move(other), cast_type)) {
      }

      /// Forbid copy construction.
      inline unique(const unique<T>&) = delete;

      /// Take ownership from another unique indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline unique(unique<U>&& other)
        : std::unique_ptr<T>(std::move(other)) {
      }

      /// Take ownership from another unique indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline const unique<T>& operator=(unique<U>&& other) {
        std::unique_ptr<T>::operator=(std::move(other));
      }
    };

    /// A fast (unsafe) pointer that deletes the data when it is deleted.
    template <typename T> class uptr : public unique<T> {
      template <typename U> friend class ptr;
      using unique<T>::unique;

    public:
      /// Null default constructor.
      inline uptr() : unique<T>(nullptr, unsafe_raw_t(0)) {
      }

      /// Explicit null constructor.
      inline uptr(nullptr_t) : uptr() {
      }
    };

    /// A fast (unsafe) reference that deletes the data when it is deleted.
    ///
    /// Due to the need to be able to return these, we must make them movable,
    /// so this means we can't statically completely prevent the code from
    /// creating null references. The safe version ensures these are never
    /// used.
    template <typename T> class uref : public unique<T> {
    public:
      /// Unsafe construction from a raw pointer.
      inline uref(T* raw_ptr, unsafe_raw_t) : unique<T>(raw_ptr, unsafe_raw_t(0)) {
      }

      /// Cast construction from a different type of unique indirection.
      template <typename U, typename C> inline uref(unique<U>&& other, C cast_type) : unique<T>(std::move(other), cast_type) {
      }

      /// Copy a reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline uref(uref<U>&& other)
        : unique<T>(std::move(other)) {
      }

      /// Copy a pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline uref(uptr<U>&& other)
        : unique<T>(std::move(other)) {
      }

      /// Access the raw pointer.
      inline const T* get() const {
        return unique<T>::get();
      }

      /// Access the raw pointer.
      inline T* get() {
        return unique<T>::get();
      }
    };

    /// A fast (unsafe) indirection for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class borrow {
      template <typename U> friend class borrow;

    protected:
      /// The raw pointer to the value.
      T* m_raw_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      inline borrow(T* raw_ptr, unsafe_raw_t) : m_raw_ptr(raw_ptr) {
      }

      /// Cast construction from a different type of borrow.
      template <typename U> inline borrow(borrow<U> other, unsafe_raw_t) : m_raw_ptr(reinterpret_cast<T*>(other.m_raw_ptr)) {
      }

      /// Cast construction from a different type of borrow.
      template <typename U> inline borrow(borrow<U> other, unsafe_static_t) : m_raw_ptr(static_cast<T*>(other.m_raw_ptr)) {
      }

      /// Cast construction from a different type of borrow.
      template <typename U> inline borrow(borrow<U> other, unsafe_dynamic_t) : m_raw_ptr(dynamic_cast<T*>(other.m_raw_ptr)) {
      }

      /// Cast construction from a different type of borrow.
      template <typename U> inline borrow(borrow<U> other, unsafe_const_t) : m_raw_ptr(const_cast<T*>(other.m_raw_ptr)) {
      }

      /// Copy a borrow.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline borrow(borrow<U> other)
        : m_raw_ptr(other.m_raw_ptr) {
      }

      /// Construction from a shared indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline borrow(const shared<U>& other)
        : m_raw_ptr(other.get()) {
      }

      /// Construction from a unique indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline borrow(const unique<U>& other)
        : m_raw_ptr(other.get()) {
      }
    };

    /// A fast (unsafe) pointer for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ptr : public borrow<T> {
      using borrow<T>::borrow;

    public:
      /// Deterministic null default constructor.
      ///
      /// This is slower from the `T*` default constructor, for sanity.
      inline ptr() : borrow<T>(nullptr, unsafe_raw_t(0)) {
      }

      /// Explicit null constructor.
      inline ptr(nullptr_t) : ptr() {
      }

      /// Access the raw pointer.
      inline T* get() const {
        return borrow<T>::m_raw_ptr;
      }
    };

    /// A fast (unsafe) reference for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ref : public borrow<T> {
    public:
      /// Unsafe construction from a raw pointer.
      inline ref(T* raw_ptr, unsafe_raw_t) : borrow<T>(raw_ptr, unsafe_raw_t(0)) {
      }

      /// Cast construction from a different type of borrow.
      template <typename U, typename C> inline ref(const borrow<U>& other, C cast_type) : borrow<T>(other, cast_type) {
      }

      /// Copy a reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline ref(const ref<U>& other)
        : borrow<T>(other) {
      }

      /// Copy a pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline ref(const ptr<U>& other)
        : borrow<T>(other) {
      }

      /// Copy a shared reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline ref(const sref<U>& other)
        : borrow<T>(other) {
      }

      /// Copy a unique pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline ref(const sptr<U>& other)
        : borrow<T>(other) {
      }

      /// Copy a unique reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline ref(const uref<U>& other)
        : borrow<T>(other) {
      }

      /// Copy a unique pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline ref(const uptr<U>& other)
        : borrow<T>(other) {
      }

      /// Access the raw pointer.
      inline const T* get() const {
        return borrow<T>::m_raw_ptr;
      }

      /// Access the raw pointer.
      inline T* get() {
        return borrow<T>::m_raw_ptr;
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
      inline no_delete() = default;

      /// Copy constructor.
      template <typename U> inline no_delete(const no_delete<U>&) {
      }

      /// (Do not) delete the given pointer.
      inline void operator()(T*) const {
      }
    };

    /// Cast the weak pointer of a `ptr` of a different type.
    template <typename T, typename U, typename C>
    inline std::weak_ptr<T>
    cast_weak_ptr(const std::shared_ptr<T>& into_unsafe_ptr, const std::weak_ptr<U>& from_weak_ptr, C cast_type) {
      if (into_unsafe_ptr) {
        return into_unsafe_ptr;
      } else {
        return cast_shared_ptr<T>(from_weak_ptr.lock(), cast_type);
      }
    }

    /// A safe (slow) indirection that deletes the data when it is deleted.
    template <typename T> class unique : public std::unique_ptr<T> {
      template <typename U> friend class unique;
      template <typename U> friend class borrow;

    protected:
      /// Track the lifetime of the data.
      std::shared_ptr<T> m_shared_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      inline unique(T* raw_ptr, unsafe_raw_t) : std::unique_ptr<T>(raw_ptr), m_shared_ptr(raw_ptr, no_delete<T>()) {
      }

      /// Cast construction from a different type of unique indirection.
      template <typename U, typename C>
      inline unique(unique<U>&& other, C cast_type)
        : std::unique_ptr<T>(cast_unique_ptr<T, U>(std::move(other), cast_type)),
          m_shared_ptr(cast_shared_ptr<T, U>(std::move(other.m_shared_ptr), cast_type)) {
      }

      /// Forbid copy construction.
      inline unique(const unique<T>&) = delete;

      /// Take ownership from another unique indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline unique(unique<U>&& other)
        : std::unique_ptr<T>(std::move(other)), m_shared_ptr(std::move(other.m_shared_ptr)) {
      }

      /// Take ownership from another unique indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline const unique<T>& operator=(unique<U>&& other) {
        std::unique_ptr<T>::operator=(std::move(other));
        m_shared_ptr = std::move(other.m_shared_ptr);
      }
    };

    /// A safe (slow) pointer that deletes the data when it is deleted.
    template <typename T> class uptr : public unique<T> {
      using unique<T>::unique;

    public:
      /// Null default constructor.
      inline uptr() : unique<T>(nullptr, unsafe_raw_t(0)) {
      }

      /// Explicit null constructor.
      inline uptr(nullptr_t) : uptr() {
      }
    };

    /// A safe (slow) reference that deletes the data when it is deleted.
    ///
    /// Due to the need to be able to return these, we must make them movable,
    /// so this means we can't statically completely prevent the code from
    /// creating null references. The safe version ensures these are never
    /// used.
    template <typename T> class uref : public unique<T> {
    public:
      /// Unsafe construction from a raw pointer.
      inline uref(T* raw_ptr, unsafe_raw_t) : unique<T>(raw_ptr, unsafe_raw_t(0)) {
        CPL_ASSERT(unique<T>::get(), "constructing a null reference");
      }

      /// Cast construction from a different type of unique reference.
      template <typename U, typename C> inline uref(unique<U>&& other, C cast_type) : unique<T>(std::move(other), cast_type) {
        CPL_ASSERT(unique<T>::get(), "constructing a null reference");
      }

      /// Take ownership from another unique reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline uref(uref<U>&& other)
        : unique<T>(std::move(other)) {
        CPL_ASSERT(unique<T>::get(), "constructing a null reference");
      }

      /// Take ownership from another unique pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline uref(uptr<U>&& other)
        : unique<T>(std::move(other)) {
        CPL_ASSERT(unique<T>::get(), "constructing a null reference");
      }

      /// Access the raw pointer.
      inline const T* get() const {
        const T* raw_ptr = unique<T>::get();
        CPL_ASSERT(raw_ptr, "accessing a null reference");
        return raw_ptr;
      }

      /// Access the raw pointer.
      inline T* get() {
        T* raw_ptr = unique<T>::get();
        CPL_ASSERT(raw_ptr, "accessing a null reference");
        return raw_ptr;
      }
    };

    /// A safe (slow) indirection for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class borrow {
      template <typename U> friend class borrow;

    protected:
      /// If we hold a pointer created by `unsafe_ref` or `unsafe_ptr`, then
      /// this will provide a lifetime to `m_weak_ptr`.
      std::shared_ptr<T> m_unsafe_ptr;

      /// A weak pointer to the track the lifetime of the data.
      std::weak_ptr<T> m_weak_ptr;

    public:
      /// Unsafe construction from a raw pointer.
      inline borrow(T* raw_ptr, unsafe_raw_t) : m_unsafe_ptr(raw_ptr, no_delete<T>()), m_weak_ptr(m_unsafe_ptr) {
      }

      /// Cast construction from a different type of borrow.
      template <typename U, typename C>
      inline borrow(const borrow<U>& other, C cast_type)
        : m_unsafe_ptr(cast_shared_ptr<T, U>(other.m_unsafe_ptr, cast_type)),
          m_weak_ptr(cast_weak_ptr(m_unsafe_ptr, other.m_weak_ptr, cast_type)) {
      }

      /// Copy a borrow.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline borrow(const borrow<U>& other)
        : m_unsafe_ptr(other.m_unsafe_ptr ? std::shared_ptr<T>(other.m_unsafe_ptr.get(), no_delete<T>())
                                          : std::shared_ptr<T>(nullptr)),
          m_weak_ptr(m_unsafe_ptr ? m_unsafe_ptr : other.m_weak_ptr.lock()) {
      }

      /// Construction from a unique indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline borrow(const unique<U>& other)
        : m_unsafe_ptr(), m_weak_ptr(other.m_shared_ptr) {
      }

      /// Construction from a shared indirection.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline borrow(const shared<U>& other)
        : m_unsafe_ptr(), m_weak_ptr(other) {
      }
    };

    /// A safe (slow) pointer for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ptr : public borrow<T> {
      using borrow<T>::borrow;

    public:
      /// Deterministic null default constructor.
      inline ptr() : borrow<T>(nullptr, unsafe_raw_t(0)) {
      }

      /// Explicit null constructor.
      inline ptr(nullptr_t) : ptr() {
      }

      /// Access the raw pointer.
      ///
      /// This isn't as safe as we'd like it to be, since another thread may
      /// delete the value between the time we `return` and the time the caller
      /// uses the value.
      inline T* get() const {
        return borrow<T>::m_weak_ptr.lock().get();
      }
    };

    /// A safe (slow) reference for data whose lifetime is determined
    /// elsewhere.
    template <typename T> class ref : public borrow<T> {
    public:
      /// Unsafe construction from a raw pointer.
      inline ref(T* raw_ptr, unsafe_raw_t) : borrow<T>(raw_ptr, unsafe_raw_t(0)) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Cast construction from a different type of borrow.
      template <typename U, typename C> inline ref(const borrow<U>& other, C cast_type) : borrow<T>(other, cast_type) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Copy a reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline ref(const ref<U>& other)
        : borrow<T>(other) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Copy a pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline ref(const ptr<U>& other)
        : borrow<T>(other) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Copy a shared reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline ref(const sref<U>& other)
        : borrow<T>(other) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Copy a shared pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline ref(const sptr<U>& other)
        : borrow<T>(other) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Copy a unique reference.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      inline ref(const uref<U>& other)
        : borrow<T>(other) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Copy a unique pointer.
      template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
      explicit inline ref(const uptr<U>& other)
        : borrow<T>(other) {
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
      }

      /// Ensure no assignment of a null reference.
      inline const ref& operator=(const ref<T>& other) {
        borrow<T>::operator=(other);
        CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "assigning a null reference");
        return *this;
      }

      /// Access the raw pointer.
      ///
      /// This isn't as safe as we'd like it to be, since another thread may
      /// delete the value between the time we `return` and the time the caller
      /// uses the value.
      inline const T* get() const {
        const T* raw_ptr = borrow<T>::m_weak_ptr.lock().get();
        CPL_ASSERT(raw_ptr, "accessing a null reference");
        return raw_ptr;
      }

      /// Access the raw pointer.
      ///
      /// This isn't as safe as we'd like it to be, since another thread may
      /// delete the value between the time we `return` and the time the caller
      /// uses the value.
      inline T* get() {
        T* raw_ptr = borrow<T>::m_weak_ptr.lock().get();
        CPL_ASSERT(raw_ptr, "accessing a null reference");
        return raw_ptr;
      }
    };

#endif // DOXYGEN || CPL_SAFE

#ifdef DOXYGEN
  }
#endif // DOXYGEN

  /// @file
  /// Implement the unsafe creation of pointers and references.inters and
  /// references.

  /// Create some value owned by a shared reference.
  template <typename T, typename... Args> inline sref<T> make_sref(Args&&... args) {
    return sref<T>{ new T(std::forward<Args>(args)...), unsafe_raw_t(0) };
  }

  /// Create some value owned by a shared pointer.
  template <typename T, typename... Args> inline sptr<T> make_sptr(Args&&... args) {
    return sptr<T>{ new T(std::forward<Args>(args)...), unsafe_raw_t(0) };
  }

  /// Create some value owned by a unique reference.
  template <typename T, typename... Args> inline uref<T> make_uref(Args&&... args) {
    return uref<T>{ new T(std::forward<Args>(args)...), unsafe_raw_t(0) };
  }

  /// Create some value owned by a unique pointer.
  template <typename T, typename... Args> inline uptr<T> make_uptr(Args&&... args) {
    return uptr<T>{ new T(std::forward<Args>(args)...), unsafe_raw_t(0) };
  }

  /// Create an unsafe reference to raw data.
  ///
  /// This is playing with fire. It is OK if the data is static, but there's
  /// no way to ask the compiler to ensure that.
  template <typename T> inline ref<T> unsafe_ref(T& data) {
    return ref<T>{ &data, unsafe_raw_t(0) };
  }

  /// Create an unsafe pointer to raw data.
  ///
  /// This is playing with fire. It is OK if the data is static, but there's
  /// no way to ask the compiler to ensure that.
  template <typename T> inline ptr<T> unsafe_ptr(T& data) {
    return ptr<T>{ &data, unsafe_raw_t(0) };
  }

#ifdef DOXYGEN
  namespace fast {
#endif // DOXYGEN

#if defined(DOXYGEN) || defined(CPL_FAST)
    /// A static cast between reference types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline sref<T> cast_static(const sref<U>& from_ref) {
      return sref<T>{ from_ref, unsafe_static_t(0) };
    }

    /// A static cast between pointer types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline sptr<T> cast_static(const sptr<U>& from_ptr) {
      return sptr<T>{ from_ptr, unsafe_static_t(0) };
    }

    /// A static cast between reference types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline uref<T> cast_static(uref<U>&& from_ref) {
      return uref<T>{ std::move(from_ref), unsafe_static_t(0) };
    }

    /// A static cast between pointer types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline uptr<T> cast_static(uptr<U>&& from_ptr) {
      return uptr<T>{ std::move(from_ptr), unsafe_static_t(0) };
    }

    /// A static cast between reference types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline ref<T> cast_static(const ref<U>& from_ref) {
      return ref<T>{ from_ref, unsafe_static_t(0) };
    }

    /// A static cast between pointer types.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline ptr<T> cast_static(const ptr<U>& from_ptr) {
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
    inline sref<T> cast_static(const sref<U>& from_ref) {
      U* from_raw = const_cast<U*>(from_ref.get());
      T* to_dynamic = dynamic_cast<T*>(from_raw);
      T* to_raw = static_cast<T*>(from_raw);
      CPL_ASSERT(to_dynamic == to_raw, "static cast gave the wrong result");
      return sref<T>{ from_ref, unsafe_raw_t(0) };
    }

    /// Static cast between related pointer types.
    ///
    /// This verifies that the pointer value did not change, which will
    /// always be true unless you use virtual base classes.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline sptr<T> cast_static(const sptr<U>& from_ptr) {
      U* from_raw = from_ptr.get();
      T* to_dynamic = dynamic_cast<T*>(from_raw);
      T* to_raw = static_cast<T*>(from_raw);
      CPL_ASSERT(to_dynamic == to_raw, "static cast gave the wrong result");
      return sptr<T>{ from_ptr, unsafe_raw_t(0) };
    }

    /// Static cast between related reference types.
    ///
    /// This verifies that the raw pointer value did not change, which will
    /// always be true unless you use virtual base classes.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline uref<T> cast_static(uref<U>&& from_ref) {
      U* from_raw = from_ref.get();
      T* to_dynamic = dynamic_cast<T*>(from_raw);
      T* to_raw = static_cast<T*>(from_raw);
      CPL_ASSERT(to_dynamic == to_raw, "static cast gave the wrong result");
      return uref<T>{ std::move(from_ref), unsafe_raw_t(0) };
    }

    /// Static cast between related pointer types.
    ///
    /// This verifies that the pointer value did not change, which will
    /// always be true unless you use virtual base classes.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline uptr<T> cast_static(uptr<U>&& from_ptr) {
      U* from_raw = from_ptr.get();
      T* to_dynamic = dynamic_cast<T*>(from_raw);
      T* to_raw = static_cast<T*>(from_raw);
      CPL_ASSERT(to_dynamic == to_raw, "static cast gave the wrong result");
      return uptr<T>{ std::move(from_ptr), unsafe_raw_t(0) };
    }

    /// Static cast between related reference types.
    ///
    /// This verifies that the raw pointer value did not change, which will
    /// always be true unless you use virtual base classes.
    template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
    inline ref<T> cast_static(const ref<U>& from_ref) {
      U* from_raw = const_cast<U*>(from_ref.get());
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
    inline ptr<T> cast_static(const ptr<U>& from_ptr) {
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
  inline sref<T> cast_reinterpret(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline sptr<T> cast_reinterpret(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline uref<T> cast_reinterpret(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline uptr<T> cast_reinterpret(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_raw_t(0) };
  }

  /// A reinterpret cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline ref<T> cast_reinterpret(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline ptr<T> cast_reinterpret(const ptr<U>& from_ptr) {
    return ptr<T>{ from_ptr, unsafe_raw_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline sref<T> cast_dynamic(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline sptr<T> cast_dynamic(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline uref<T> cast_dynamic(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline uptr<T> cast_dynamic(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline ref<T> cast_dynamic(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline ptr<T> cast_dynamic(const ptr<U>& from_ptr) {
    return ptr<T>{ from_ptr, unsafe_dynamic_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline sref<T> cast_const(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline sptr<T> cast_const(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_const_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline uref<T> cast_const(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline uptr<T> cast_const(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_const_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline ref<T> cast_const(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U, typename = typename std::enable_if<std::is_convertible<T*, U*>::value>::type>
  inline ptr<T> cast_const(const ptr<U>& from_ptr) {
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
