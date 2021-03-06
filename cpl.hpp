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
#include <memory>

#ifndef CPL_WITHOUT_COLLECTIONS // {

#ifdef CPL_FAST // {
#include <bitset>
#include <map>
#include <set>
#include <string>
#include <vector>
#endif // } CPL_FAST

#ifdef CPL_SAFE // {
#include <debug/bitset>
#include <debug/map>
#include <debug/set>
#include <debug/string>
#include <debug/vector>
#endif // } CPL_SAFE

#endif // } CPL_WITHOUT_COLLECTIONS

/// The Git-derived version number.
///
/// To update this, run `make version`. This should be done before every
/// commit. It should arguably be managed by git hooks, but it really isn't
/// that much of a hassle.
#define CPL_VERSION "0.2.4"

#ifdef DOXYGEN // {

/// The name of the CPL variant we are compiling
#define CPL_VARIANT

/// If this is defined, the fast (unsafe) variant will be compiled.
#define CPL_FAST

/// If this is defined, the safe (slow) variant will be compiled.
#define CPL_SAFE

/// If this is defined, do not provide the @ref cpl version of the standard
/// collections, and do not even include their header files.
#define CPL_WITHOUT_COLLECTIONS

#else // } DOXYGEN {

// Ensure one of @ref CPL_FAST or @ref CPL_SAFE are defined, and configure the
// @ref CPL_VARIANT accordingly.

#ifdef CPL_FAST // {

#ifdef CPL_SAFE // {
#error "Both CPL_FAST and CPL_SAFE are defined"
#endif // } CPL_SAFE

#define CPL_VARIANT "fast"

#else // } CPL_FAST {

#ifdef CPL_SAFE // {
#define CPL_VARIANT "safe"
#else // } CPL_SAFE {
#error "No CPL variant chosen - neither CPL_FAST nor CPL_SAFE are defined"
#endif // } CPL_SAFE

#endif // } CPL_FAST

#endif // } DOXYGEN

#ifndef CPL_ASSERT // {
/// Perform a run-time verification.
///
/// By default, if the condition is false, this throws a generic
/// `std::logic_error` with the specified message. Making this a macro allows
/// user code to override it with a custom mechanism.
#define CPL_ASSERT(CONDITION, MESSAGE) \
  if (CONDITION) {                     \
  } else                               \
  throw(std::logic_error(MESSAGE))
#endif // } CPL_ASSERT

#ifdef CPL_FAST // {
#undef CPL_ASSERT
#define CPL_ASSERT(CONDITION, MESSAGE)
#endif // } CPL_FAST

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
/// To use CPL, simply include `cpl.hpp` and use the types defined in it.
///
/// ## Types
///
/// CPL provides the following set of types:
///
/// | Type           | May be null? | Data lifetime is as long as        | Fast implementation is based on  |
/// | -------------- | ------------ | ---------------------------------- | -------------------------------  |
/// | @ref cpl::is   | No           | The `is` exists                    | `T`                              |
/// | @ref cpl::opt  | Yes          | The `opt` exists and is not reset  | `std::experimental::optional<T>` |
/// | @ref cpl::uref | No           | The `uref` exists                  | `std::unique_ptr<T>`             |
/// | @ref cpl::uptr | Yes          | The `uptr` exists and is not reset | `std::unique_ptr<T>`             |
/// | @ref cpl::sref | No           | The `sref` exists                  | `std::shared_ptr<T>`             |
/// | @ref cpl::sptr | Yes          | The `sptr` exists                  | `std::shared_ptr<T>`             |
/// | @ref cpl::wptr | Yes          | Some `sptr` exists                 | `std::weak_ptr<T>`               |
/// | @ref cpl::ref  | No           | One of the above holds the data    | `std::reference_wrapper`         |
/// | @ref cpl::ptr  | Yes          | One of the above holds the data    | `T*`                             |
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
/// The interface of the CPL types is as close as possible to the interface of
/// the types they are based on. Note that the base types have somewhat
/// inconsistent interface; for example, only `opt` has `value_or`, and
/// only the `const`-ness of `is` and `opt` is reflected in the accessed value
/// (that is, a `const ref<T>` is not the same as `ref<const T>`).
///
/// All the pointer-like types provide the usual `operator*`, `operator->` and
/// `operator bool` operators, and sometimes also well as `get`, `value`, and
/// `value_or` (if these are provided by the relevant standard type, and also
/// a `reset()` for @ref cpl::opt because it just makes sense).
///
/// In addition, `cpl::ptr` provide `ref` and `ref_or` methods. These are more
/// efficient than `value` and `value_or` since they just return a reference;
/// using CPL types we avoid the reference-to-deleted-temporary-object problem
/// that prevented `value_or` from returning a reference.
///
/// The reference types provide the same interface, minus `operator
/// bool`, and with the addition of `operator T&`.
///
/// It would have been nice to have a consistent interface for the different
/// types, but it was deemed more important to stay as close as possible to
/// the standard types.
///
/// For a semblance of consistency, @ref cpl::ptr and @ref cpl::ref have a
/// similar interface to @ref cpl::sref and @ref cpl::uref, which are in turn
/// similar to @ref cpl::sptr and @ref cpl::uptr - for example, they also
/// provides a `get`. This means that all the "reference" types, @ref cpl::ref
/// included, are more like a guaranteed-non-null-`T*` than a `T&`. This is
/// also reflected in the fact that `const`-ness of the CPL type does not mean
/// `const`-ness of the accessed value. It is very difficult to create a "smart
/// reference" type in C++ as it is at the time of writing this (C++14). At
/// least there's some comfort in knowing that `->` will always work to access
/// the data members.
///
/// ## Casting
///
/// CPL provides @ref cpl::cast_static, @ref cpl::cast_dynamic, @ref
/// cpl::cast_reinterpret and @ref cpl::cast_const which work on all the CPL
/// types. The fast @ref cpl::cast_clever is the same as `cpl::cast_static`.
/// The safe `cpl::cast_clever` verifies that `cpl::cast_static` gives the same
/// results as `cpl::cast_dynamic`. This will only fail if there are virtual
/// base classes involved, in which case you should use the more costly
/// `cpl::cast_dynamic`. Other than this, CPL lets you freely shoot yourself in
/// the foot using the casts.
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
/// static" constraint in C++, so it is impossible for CPL to distinguish the
/// safe and unsafe cases.
///
/// ## Collections
///
/// Unless @ref CPL_WITHOUT_COLLECTIONS is defined, then CPL will provide the
/// @ref cpl::bitset, @ref cpl::map, @ref cpl::set, @ref cpl::string and @ref
/// cpl::vector types. These will compile to the standard versions in fast mode
/// and to the (G++ specific) debug versions in safe mode.
///
/// Using these types instead of the `std` types will provide additional checks
/// in safe mode, detecting out-of-bounds and similar errors, while having zero
/// impact on the fast mode.
namespace cpl {
  template <typename T> class sref;
  template <typename T> class uref;
  template <typename T> class ref;

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

  /// A holder of some value.
  ///
  /// This allows creation of @ref cpl::ptr and @ref cpl::ref to the value. It
  /// is annoying we need to wrap the value in a class for this. Deriving from
  /// `T` makes it a bit easier to suffer, but also means that `T` can't be a
  /// primitive type.
  template <typename T> class is : public T {
#ifdef CPL_SAFE // {
    template <typename U> friend class borrow;

    /// Track the lifetime of the data.
    std::shared_ptr<T> m_shared_ptr;
#endif // } CPL_SAFE

  public:
    /// Reuse the held value constructors.
    template <typename... Args>
    inline is(Args&&... args)
      : T(std::forward<Args>(args)...)
#ifdef CPL_SAFE // {
        ,
        m_shared_ptr((T*)this, no_delete<T>())
#endif // } CPL_SAFE
    {
    }

    /// Ensure we don't copy the shared pointer value.
    inline is(const is<T>& other)
      : T(other)
#ifdef CPL_SAFE // {
        ,
        m_shared_ptr((T*)this, no_delete<T>())
#endif // } CPL_SAFE
    {
    }

    /// Ensure we don't copy the shared pointer value.
    inline const is& operator=(const is<T>& other) {
      T::operator=(other);
      return *this;
    }

    /// Allow assigning a raw value.
    inline const is& operator=(const T& other) {
      T::operator=(other);
      return *this;
    }
  };

  /// Allow convenient access to `std::experimental::in_place_t`.
  typedef std::experimental::in_place_t in_place_t;

  /// Allow convenient access to `std::experimental::in_place`.
  constexpr std::experimental::in_place_t in_place{};

  /// A holder of some optional value.
  template <typename T> class opt : public std::experimental::optional<T> {
#ifdef CPL_SAFE // {
    template <typename U> friend class borrow;

    /// Track the lifetime of the data.
    std::shared_ptr<T> m_shared_ptr;

  public:
    /// Reuse the optional value constructors.
    template <typename... Args>
    inline opt(Args&&... args)
      : std::experimental::optional<T>(std::forward<Args>(args)...),
        m_shared_ptr(!*this ? (T*)nullptr : std::experimental::optional<T>::operator->(), no_delete<T>()) {
    }

    /// Reuse the optional value assignment.
    template <typename... Args> inline opt& operator=(Args&&... args) {
      bool was_invalid = !*this;
      std::experimental::optional<T>::operator=(std::forward<Args>(args)...);
      if (!*this != was_invalid) {
        if (!*this) {
          m_shared_ptr.reset();
        } else {
          m_shared_ptr.reset(std::experimental::optional<T>::operator->(), no_delete<T>());
        }
      }
      return *this;
    }
#else  // } CPL_SAFE {
    using std::experimental::optional<T>::optional;

  public:
#endif // } CPL_SAFE

#ifdef CPL_SAFE // {
    /// Access the value.
    inline T& operator*() {
      CPL_ASSERT(!!*this, "accessing an empty optional value");
      return std::experimental::optional<T>::operator*();
    }

    /// Access the value.
    inline const T& operator*() const {
      CPL_ASSERT(!!*this, "accessing an empty optional value");
      return std::experimental::optional<T>::operator*();
    }

    /// Access a data member.
    inline T* operator->() {
      CPL_ASSERT(!!*this, "accessing an empty optional value");
      return std::experimental::optional<T>::operator->();
    }

    /// Access a data member.
    inline const T* operator->() const {
      CPL_ASSERT(!!*this, "accessing an empty optional value");
      return std::experimental::optional<T>::operator->();
    }
#endif // } CPL_SAFE

    /// Make the optional value empty.
    void reset() {
      std::experimental::optional<T>::operator=(std::experimental::nullopt);
#ifdef CPL_SAFE // {
      m_shared_ptr.reset();
#endif // } CPL_SAFE
    };

#ifdef CPL_SAFE // {
    /// Track swap of the value.
    inline void swap(opt<T>& other) {
      std::experimental::optional<T>::swap(other);
      if (!*this || !other) {
        if (!*this) {
          m_shared_ptr.reset();
        } else {
          m_shared_ptr.reset(std::experimental::optional<T>::operator->(), no_delete<T>());
        }
        if (!other) {
          other.m_shared_ptr.reset();
        } else {
          other.m_shared_ptr.reset(other.std::experimental::optional<T>::operator->(), no_delete<T>());
        }
      }
    }

    /// Construct a value in-place.
    template <typename... Args> inline void emplace(Args&&... args) {
      if (!*this) {
        m_shared_ptr.reset(std::experimental::optional<T>::operator->(), no_delete<T>());
      }
      std::experimental::optional<T>::emplace(std::forward<Args>(args)...);
    }
#endif // } CPL_SAFE
  };

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

  /// Cast a raw pointer to a different type.
  template <typename T, typename U> inline T* cast_raw_ptr(U* other, unsafe_raw_t) {
    return reinterpret_cast<T*>(other);
  }

  /// Cast a raw pointer to a different type.
  template <typename T, typename U> inline T* cast_raw_ptr(U* other, unsafe_static_t) {
    return static_cast<T*>(other);
  }

  /// Cast a raw pointer to a different type.
  template <typename T, typename U> inline T* cast_raw_ptr(U* other, unsafe_dynamic_t) {
    return dynamic_cast<T*>(other);
  }

  /// Cast a raw pointer to a different type.
  template <typename T, typename U> inline T* cast_raw_ptr(U* other, unsafe_const_t) {
    return const_cast<T*>(other);
  }

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

  /// An indirection that uses reference counting.
  template <typename T> class shared : public std::shared_ptr<T> {
    template <typename U> friend class shared;

  public:
    /// Construction from a standard shared pointer.
    inline shared(const std::shared_ptr<T>& other) : std::shared_ptr<T>(other) {
    }

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

    /// Move construction from a compatible type of shared indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline shared(shared<U>&& other)
      : std::shared_ptr<T>(std::move(other)) {
    }

#ifdef CPL_SAFE // {
    /// Access the value.
    inline T& operator*() const {
      T* raw_ptr = std::shared_ptr<T>::get();
      CPL_ASSERT(raw_ptr, "dereferencing a null pointer");
      return *raw_ptr;
    }

    /// Access a data member.
    inline T* operator->() const {
      T* raw_ptr = std::shared_ptr<T>::get();
      CPL_ASSERT(raw_ptr, "dereferencing a null pointer");
      return raw_ptr;
    }
#endif // } CPL_SAFE
  };

  /// A pointer that uses reference counting.
  template <typename T> class sptr : public shared<T> {
    template <typename U> friend class wptr;
    using shared<T>::shared;

  public:
    /// Null default constructor.
    inline sptr() : shared<T>(nullptr, unsafe_raw_t(0)) {
    }

    /// Explicit null constructor.
    inline sptr(std::nullptr_t) : sptr() {
    }

    /// Share with another shared indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sptr(shared<U>&& other)
      : shared<T>(std::move(other)) {
    }

    /// Share with another shared indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sptr<T>& operator=(shared<U>&& other) {
      shared<T>::operator=(std::move(other));
      return *this;
    }

    /// Provide a reference to the value (which must exist).
    inline ::cpl::ref<T> ref() const {
      return ::cpl::ref<T>(*this);
    }

    /// Access the current value or, if empty, a default value.
    inline ::cpl::ref<T> ref_or(const ::cpl::ref<T>& if_empty) const {
      return *this ? ref() : if_empty;
    }

    /// Convert the pointer to a reference.
    inline cpl::sref<T> sref() const {
      return cpl::sref<T>(std::move(*this));
    }
  };

  /// A reference that uses reference counting.
  template <typename T> class sref : public shared<T> {
  public:
    /// Unsafe construction from a raw pointer.
    inline sref(T* raw_ptr, unsafe_raw_t) : shared<T>(raw_ptr, unsafe_raw_t(0)) {
      CPL_ASSERT(shared<T>::get(), "constructing a null reference");
    }

    /// Cast construction from a different type of shared indirection.
    template <typename U, typename C> inline sref(const shared<U>& other, C cast_type) : shared<T>(other, cast_type) {
      CPL_ASSERT(shared<T>::get(), "constructing a null reference");
    }

    /// Copy a reference.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sref(const sref<U>& other)
      : shared<T>(other) {
      CPL_ASSERT(shared<T>::get(), "constructing a null reference");
    }

    /// Copy a pointer.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    explicit inline sref(const sptr<U>& other)
      : shared<T>(other) {
      CPL_ASSERT(shared<T>::get(), "constructing a null reference");
    }

    /// Share with another shared pointer.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    explicit inline sref(sptr<U>&& other)
      : shared<T>(std::move(other)) {
    }

    /// Share with another shared reference.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sref<T>& operator=(sref<U>&& other) {
      sref<T>::operator=(std::move(other));
      return *this;
    }

    /// Share with another shared reference.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sref(sref<U>&& other)
      : shared<T>(std::move(other)) {
    }

    /// Forbid testing for null.
    explicit operator bool() const = delete;

    /// Forbid clearing the reference.
    void reset() = delete;

    /// Forbid clearing the reference.
    template <typename U> void reset(U* raw_ptr) {
      shared<T>::reset(raw_ptr);
      CPL_ASSERT(shared<T>::get(), "resetting a null reference");
    }

    /// Forbid clearing the reference.
    template <typename U, typename D> void reset(U* raw_ptr, D deleter) {
      shared<T>::reset(raw_ptr, deleter);
      CPL_ASSERT(shared<T>::get(), "resetting a null reference");
    }

    /// Forbid clearing the reference.
    template <typename U, typename D, typename A> void reset(U* raw_ptr, D deleter, A allocator) {
      shared<T>::reset(raw_ptr, deleter, allocator);
      CPL_ASSERT(shared<T>::get(), "resetting a null reference");
    }

    /// Access the value.
    inline operator T&() const {
      return *shared<T>::get();
    }
  };

  /// A weak way to obtain a shared pointer.
  template <typename T> struct wptr : public std::weak_ptr<T> {
    using std::weak_ptr<T>::weak_ptr;

  public:
    /// Obtain a shared pointer, unless the data was already deleted.
    sptr<T> lock() const {
      return sptr<T>(std::weak_ptr<T>::lock());
    }
  };

  /// An indirection that deletes the data when it is deleted.
  template <typename T> class unique : public std::unique_ptr<T> {
    template <typename U> friend class unique;
#ifdef CPL_SAFE // {

#ifndef DOXYGEN // {
    // Doxygen 1.8.5 gets terribly confused by this statement.
    template <typename U> friend class borrow;
#endif // } DOXYGEN

  protected:
    /// Track the lifetime of the data.
    std::shared_ptr<T> m_shared_ptr;
#endif // } CPL_SAFE

  public:
    /// Unsafe construction from a raw pointer.
    inline unique(T* raw_ptr, unsafe_raw_t)
      : std::unique_ptr<T>(raw_ptr)
#ifdef CPL_SAFE // {
        ,
        m_shared_ptr(raw_ptr, no_delete<T>())
#endif // } CPL_SAFE
    {
    }

    /// Cast construction from a different type of unique indirection.
    template <typename U, typename C>
    inline unique(unique<U>&& other, C cast_type)
      : std::unique_ptr<T>(cast_unique_ptr<T, U>(std::move(other), cast_type))
#ifdef CPL_SAFE // {
        ,
        m_shared_ptr(cast_shared_ptr<T, U>(std::move(other.m_shared_ptr), cast_type))
#endif // } CPL_SAFE
    {
    }

    /// Forbid copy construction.
    inline unique(const unique<T>&) = delete;

    /// Take ownership from another unique indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline unique(unique<U>&& other)
      : std::unique_ptr<T>(std::move(other))
#ifdef CPL_SAFE // {
        ,
        m_shared_ptr(std::move(other.m_shared_ptr))
#endif // } CPL_SAFE
    {
    }

    /// Take ownership from another unique indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline unique<T>& operator=(unique<U>&& other) {
      std::unique_ptr<T>::operator=(std::move(other));
#ifdef CPL_SAFE // {
      m_shared_ptr = std::move(other.m_shared_ptr);
#endif // } CPL_SAFE
      return *this;
    }

#ifdef CPL_SAFE // {
    /// Track reset of the indirection.
    void reset(T* raw_ptr = nullptr) {
      std::unique_ptr<T>::reset(raw_ptr);
      m_shared_ptr.reset(raw_ptr);
    }
#endif // } CPL_SAFE

    /// Track swap of the indirection.
    inline void swap(unique<T>& other) {
      std::unique_ptr<T>::swap(other);
#ifdef CPL_SAFE // {
      m_shared_ptr.swap(other.m_shared_ptr);
#endif // } CPL_SAFE
    }

#ifdef CPL_SAFE // {
    /// Access the value.
    inline T& operator*() const {
      T* raw_ptr = std::unique_ptr<T>::get();
      CPL_ASSERT(raw_ptr, "dereferencing a null pointer");
      return *raw_ptr;
    }

    /// Access a data member.
    inline T* operator->() const {
      T* raw_ptr = std::unique_ptr<T>::get();
      CPL_ASSERT(raw_ptr, "dereferencing a null pointer");
      return raw_ptr;
    }
#endif // } CPL_SAFE
  };

  // Forward declare for `swap`.
  template <typename T> class uref;

  /// A pointer that deletes the data when it is deleted.
  template <typename T> class uptr : public unique<T> {
    template <typename U> friend class ptr;
    using unique<T>::unique;

  public:
    /// Null default constructor.
    inline uptr() : unique<T>(nullptr, unsafe_raw_t(0)) {
    }

    /// Explicit null constructor.
    inline uptr(std::nullptr_t) : uptr() {
    }

#ifdef CPL_SAFE // {
    /// Allow @ref cpl::uref to catch swaps.
    inline void swap(cpl::uref<T>& other) {
      other.swap(*this);
    }

    /// Allow swapping with @ref cpl::uptr as well.
    inline void swap(uptr<T>& other) {
      unique<T>::swap(other);
    }
#endif // } CPL_SAFE

    /// Provide a reference to the value (which must exist).
    inline ::cpl::ref<T> ref() const {
      return ::cpl::ref<T>(*this);
    }

    /// Access the current value or, if empty, a default value.
    inline ::cpl::ref<T> ref_or(const ::cpl::ref<T>& if_empty) const {
      return *this ? ref() : if_empty;
    }

    /// Convert the pointer to a reference (clearing it).
    inline cpl::uref<T> uref() {
      return cpl::uref<T>(std::move(*this));
    }
  };

  /// A reference that deletes the data when it is deleted.
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

    /// Cast construction from a different type of unique indirection.
    template <typename U, typename C> inline uref(unique<U>&& other, C cast_type) : unique<T>(std::move(other), cast_type) {
      CPL_ASSERT(unique<T>::get(), "constructing a null reference");
    }

    /// Copy a reference.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline uref(uref<U>&& other)
      : unique<T>(std::move(other)) {
      CPL_ASSERT(unique<T>::get(), "constructing a null reference");
    }

    /// Assign a reference.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline uref& operator=(uref<U>&& other) {
      unique<T>::operator=(std::move(other));
      CPL_ASSERT(unique<T>::get(), "assigning a null reference");
      return *this;
    }

    /// Copy a pointer.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    explicit inline uref(uptr<U>&& other)
      : unique<T>(std::move(other)) {
      CPL_ASSERT(unique<T>::get(), "constructing a null reference");
    }

    /// Forbid testing for null.
    operator bool() const = delete;

    /// Forbid clearing the reference.
    void reset() = delete;

    /// Forbid clearing the reference.
    void reset(T* raw_ptr) {
      unique<T>::reset(raw_ptr);
      CPL_ASSERT(unique<T>::get(), "constructing a null reference");
    }

    /// Forbid clearing the reference.
    inline void swap(unique<T>& other) {
      unique<T>::swap(other);
      CPL_ASSERT(unique<T>::get(), "swapping a null reference");
    }

    /// Access the value.
    inline operator T&() const {
      return *unique<T>::get();
    }
  };

  /// An indirection for data whose lifetime is determined elsewhere.
  template <typename T> class borrow {
    template <typename U> friend class borrow;

  protected:
#ifdef CPL_FAST // {
    /// The raw pointer to the value.
    T* m_raw_ptr;
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
    /// If we hold a pointer created by `unsafe_ref` or `unsafe_ptr`, then
    /// this will provide a lifetime to `m_weak_ptr`.
    std::shared_ptr<T> m_unsafe_ptr;

    /// A weak pointer to the track the lifetime of the data.
    std::weak_ptr<T> m_weak_ptr;
#endif // } CPL_SAFE

  public:
    /// Provide convenient access to the type of the data.
    typedef T element_type;

    /// Unsafe construction from a raw pointer.
    inline borrow(T* raw_ptr, unsafe_raw_t)
      :
#ifdef CPL_FAST // {
        m_raw_ptr(raw_ptr)
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(raw_ptr, no_delete<T>()),
        m_weak_ptr(m_unsafe_ptr)
#endif // } CPL_SAFE
    {
    }

    /// Cast construction from a different type of borrow.
    template <typename U, typename C>
    inline borrow(borrow<U> other, C cast_type)
      :
#ifdef CPL_FAST // {
        m_raw_ptr(cast_raw_ptr<T>(other.m_raw_ptr, cast_type))
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(cast_shared_ptr<T, U>(other.m_unsafe_ptr, cast_type)),
        m_weak_ptr(cast_weak_ptr(m_unsafe_ptr, other.m_weak_ptr, cast_type))
#endif // } CPL_SAFE
    {
    }

    /// Copy a borrow.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline borrow(const borrow<U>& other)
      :
#ifdef CPL_FAST // {
        m_raw_ptr(other.m_raw_ptr)
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(other.m_unsafe_ptr ? std::shared_ptr<T>(other.m_unsafe_ptr.get(), no_delete<T>())
                                        : std::shared_ptr<T>(nullptr)),
        m_weak_ptr(m_unsafe_ptr ? m_unsafe_ptr : other.m_weak_ptr.lock())
#endif // } CPL_SAFE
    {
    }

    /// Move a borrow.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline borrow& operator=(borrow<U>&& other) {
#ifdef CPL_FAST // {
      m_raw_ptr = other.m_raw_ptr;
      other.m_raw_ptr = nullptr;
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
      m_unsafe_ptr = std::move(other.m_unsafe_ptr);
      m_weak_ptr = std::move(other.m_weak_ptr);
#endif // } CPL_SAFE
      return *this;
    }

    /// Construction from a held value.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline borrow(is<U>& other)
      :
#ifdef CPL_FAST // {
        m_raw_ptr((T*)&other)
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(),
        m_weak_ptr(other.m_shared_ptr)
#endif // } CPL_SAFE
    {
    }

    /// Construction from a held value.
    template <typename U, typename = typename std::enable_if<std::is_const<T>::value && std::is_convertible<U*, T*>::value>::type>
    inline borrow(const is<U>& other)
      : borrow(*const_cast<is<U>*>(&other)) {
    }

    /// Construction from an optional value.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline borrow(opt<U>& other)
      :
#ifdef CPL_FAST // {
        m_raw_ptr((T*)&other)
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(),
        m_weak_ptr(other.m_shared_ptr)
#endif // } CPL_SAFE
    {
    }

    /// Construction from a held value.
    template <typename U, typename = typename std::enable_if<std::is_const<T>::value && std::is_convertible<U*, T*>::value>::type>
    inline borrow(const opt<U>& other)
      : borrow(*const_cast<opt<U>*>(&other)) {
    }

    /// Construction from a shared indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline borrow(const shared<U>& other)
      :
#ifdef CPL_FAST // {
        m_raw_ptr(other.get())
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(),
        m_weak_ptr(other)
#endif // } CPL_SAFE
    {
    }

    /// Construction from a unique indirection.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline borrow(const unique<U>& other)
      :
#ifdef CPL_FAST // {
        m_raw_ptr(other.get())
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
        m_unsafe_ptr(),
        m_weak_ptr(other.m_shared_ptr)
#endif // } CPL_SAFE
    {
    }

    /// Access the raw pointer.
    ///
    /// In safe mode, this isn't as safe as we'd like it to be, since another
    /// thread may delete the value between the time we `return` and the time
    /// the caller uses the value.
    inline T* get() const {
#ifdef CPL_FAST // {
      return m_raw_ptr;
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
      return m_weak_ptr.lock().get();
#endif // } CPL_SAFE
    }

    /// Access the value.
    inline T& operator*() const {
#ifdef CPL_FAST // {
      return *get();
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
      T* raw_ptr = get();
      CPL_ASSERT(raw_ptr, "dereferencing a null borrow");
      return *raw_ptr;
#endif // } CPL_SAFE
    }

    /// Access a data member.
    inline T* operator->() const {
#ifdef CPL_FAST // {
      return get();
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
      T* raw_ptr = get();
      CPL_ASSERT(raw_ptr, "dereferencing a null borrow");
      return raw_ptr;
#endif // } CPL_SAFE
    }
  };

/// Compare borrowed indirections.
#define CPL_COMPARE_BORROW(OPERATOR)                                                                                     \
  template <typename T, typename U> inline bool operator OPERATOR(const borrow<T>& lhs, const borrow<U>& rhs) noexcept { \
    return lhs.get() OPERATOR rhs.get();                                                                                 \
  }                                                                                                                      \
  template <typename T, typename U> inline bool operator OPERATOR(const borrow<T>& lhs, U* rhs) noexcept {               \
    return lhs.get() OPERATOR rhs;                                                                                       \
  }                                                                                                                      \
  template <typename T, typename U> inline bool operator OPERATOR(T* lhs, const borrow<U>& rhs) noexcept {               \
    return lhs OPERATOR rhs.get();                                                                                       \
  }                                                                                                                      \
  template <typename T> inline bool operator OPERATOR(const borrow<T>& lhs, std::nullptr_t) noexcept {                   \
    return lhs.get() OPERATOR nullptr;                                                                                   \
  }                                                                                                                      \
  template <typename T> inline bool operator OPERATOR(std::nullptr_t, const borrow<T>& rhs) noexcept {                   \
    return nullptr OPERATOR rhs.get();                                                                                   \
  }

  CPL_COMPARE_BORROW(> )
  CPL_COMPARE_BORROW(< )
  CPL_COMPARE_BORROW(>= )
  CPL_COMPARE_BORROW(== )
  CPL_COMPARE_BORROW(!= )
  CPL_COMPARE_BORROW(<= )

  // Forward declare for the `ref` method.
  template <typename T> class ref;

  /// A pointer for data whose lifetime is determined elsewhere.
  template <typename T> class ptr : public borrow<T> {
    using borrow<T>::borrow;

  public:
    /// Deterministic null default constructor.
    ///
    /// This is slower from the `T*` default constructor, for sanity.
    inline ptr() : borrow<T>(nullptr, unsafe_raw_t(0)) {
    }

    /// Explicit null constructor.
    inline ptr(std::nullptr_t) : ptr() {
    }

    /// Test whether the pointer is not null.
    inline operator bool() const {
      return !!borrow<T>::get();
    }

    /// Provide a reference to the value (which must exist).
    inline ::cpl::ref<T> ref() const {
      return ::cpl::ref<T>(*this);
    }

    /// Access the current value or, if empty, a default value.
    inline ::cpl::ref<T> ref_or(const ::cpl::ref<T>& if_empty) const {
      return *this ? ref() : if_empty;
    }
  };

  /// A reference for data whose lifetime is determined elsewhere.
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

    /// Construct from a held value.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline ref(is<U>& other)
      : borrow<T>(other) {
    }

    /// Construct from a held value.
    template <typename U, typename = typename std::enable_if<std::is_const<T>::value && std::is_convertible<U*, T*>::value>::type>
    inline ref(const is<U>& other)
      : borrow<T>(*const_cast<is<U>*>(&other)) {
    }

    /// Construct from an optional value.
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    explicit inline ref(opt<U>& other)
      : borrow<T>(other) {
      CPL_ASSERT(borrow<T>::m_weak_ptr.lock().get(), "constructing a null reference");
    }

    /// Construct from an optional value.
    template <typename U, typename = typename std::enable_if<std::is_const<T>::value && std::is_convertible<U*, T*>::value>::type>
    inline ref(const opt<U>& other)
      : borrow<T>(*const_cast<opt<U>*>(&other)) {
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

    /// Access the value.
    inline operator T&() const {
      return *borrow<T>::get();
    }
  };

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

  /// A clever cast between reference types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline sref<T> cast_clever(const sref<U>& from_ref) {
#ifdef CPL_SAFE // {
    U* from_raw = const_cast<U*>(from_ref.get());
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return sref<T>{ from_ref, unsafe_static_t(0) };
  }

  /// A clever cast between pointer types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline sptr<T> cast_clever(const sptr<U>& from_ptr) {
#ifdef CPL_SAFE // {
    U* from_raw = from_ptr.get();
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return sptr<T>{ from_ptr, unsafe_static_t(0) };
  }

  /// A clever cast between pointer types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline wptr<T> cast_clever(const wptr<U>& from_ptr) {
#ifdef CPL_SAFE // {
    U* from_raw = from_ptr.lock().get();
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return wptr<T>{ from_ptr, unsafe_static_t(0) };
  }

  /// A clever cast between reference types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline uref<T> cast_clever(uref<U>&& from_ref) {
#ifdef CPL_SAFE // {
    U* from_raw = from_ref.get();
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return uref<T>{ std::move(from_ref), unsafe_static_t(0) };
  }

  /// A clever cast between pointer types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline uptr<T> cast_clever(uptr<U>&& from_ptr) {
#ifdef CPL_SAFE // {
    U* from_raw = from_ptr.get();
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return uptr<T>{ std::move(from_ptr), unsafe_static_t(0) };
  }

  /// A clever cast between reference types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline ref<T> cast_clever(const ref<U>& from_ref) {
#ifdef CPL_SAFE // {
    U* from_raw = const_cast<U*>(from_ref.get());
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return ref<T>{ from_ref, unsafe_static_t(0) };
  }

  /// A clever cast between pointer types.
  ///
  /// In safe mode, this verifies that the raw pointer value did not change,
  /// which will always be true unless you use virtual base classes.
  template <typename T, typename U> inline ptr<T> cast_clever(const ptr<U>& from_ptr) {
#ifdef CPL_SAFE // {
    U* from_raw = from_ptr.get();
    T* to_dynamic = dynamic_cast<T*>(from_raw);
    T* to_raw = static_cast<T*>(from_raw);
    CPL_ASSERT(to_dynamic == to_raw, "clever cast gave the wrong result");
#endif // } CPL_SAFE
    return ptr<T>{ from_ptr, unsafe_static_t(0) };
  }

  /// A reinterpret cast between reference types.
  template <typename T, typename U> inline sref<T> cast_reinterpret(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U> inline sptr<T> cast_reinterpret(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U> inline wptr<T> cast_reinterpret(const wptr<U>& from_ptr) {
    return wptr<T>{ from_ptr, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between reference types.
  template <typename T, typename U> inline uref<T> cast_reinterpret(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U> inline uptr<T> cast_reinterpret(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_raw_t(0) };
  }

  /// A reinterpret cast between reference types.
  template <typename T, typename U> inline ref<T> cast_reinterpret(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_raw_t(0) };
  }

  /// A reinterpret cast between pointer types.
  template <typename T, typename U> inline ptr<T> cast_reinterpret(const ptr<U>& from_ptr) {
    return ptr<T>{ from_ptr, unsafe_raw_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U> inline sref<T> cast_dynamic(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U> inline sptr<T> cast_dynamic(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U> inline wptr<T> cast_dynamic(const wptr<U>& from_ptr) {
    return wptr<T>{ from_ptr, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U> inline uref<T> cast_dynamic(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U> inline uptr<T> cast_dynamic(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between reference types.
  template <typename T, typename U> inline ref<T> cast_dynamic(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_dynamic_t(0) };
  }

  /// A dynamic cast between pointer types.
  template <typename T, typename U> inline ptr<T> cast_dynamic(const ptr<U>& from_ptr) {
    return ptr<T>{ from_ptr, unsafe_dynamic_t(0) };
  }

  /// A static cast between reference types.
  template <typename T, typename U> inline sref<T> cast_static(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_static_t(0) };
  }

  /// A static cast between pointer types.
  template <typename T, typename U> inline sptr<T> cast_static(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_static_t(0) };
  }

  /// A static cast between pointer types.
  template <typename T, typename U> inline wptr<T> cast_static(const wptr<U>& from_ptr) {
    return wptr<T>{ from_ptr, unsafe_static_t(0) };
  }

  /// A static cast between reference types.
  template <typename T, typename U> inline uref<T> cast_static(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_static_t(0) };
  }

  /// A static cast between pointer types.
  template <typename T, typename U> inline uptr<T> cast_static(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_static_t(0) };
  }

  /// A static cast between reference types.
  template <typename T, typename U> inline ref<T> cast_static(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_static_t(0) };
  }

  /// A static cast between pointer types.
  template <typename T, typename U> inline ptr<T> cast_static(const ptr<U>& from_ptr) {
    return ptr<T>{ from_ptr, unsafe_static_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U> inline sref<T> cast_const(const sref<U>& from_ref) {
    return sref<T>{ from_ref, unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U> inline sptr<T> cast_const(const sptr<U>& from_ptr) {
    return sptr<T>{ from_ptr, unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U> inline wptr<T> cast_const(const wptr<U>& from_ptr) {
    return wptr<T>{ from_ptr, unsafe_const_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U> inline uref<T> cast_const(uref<U>&& from_ref) {
    return uref<T>{ std::move(from_ref), unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U> inline uptr<T> cast_const(uptr<U>&& from_ptr) {
    return uptr<T>{ std::move(from_ptr), unsafe_const_t(0) };
  }

  /// A const cast between reference types.
  template <typename T, typename U> inline ref<T> cast_const(const ref<U>& from_ref) {
    return ref<T>{ from_ref, unsafe_const_t(0) };
  }

  /// A const cast between pointer types.
  template <typename T, typename U> inline ptr<T> cast_const(const ptr<U>& from_ptr) {
    return ptr<T>{ from_ptr, unsafe_const_t(0) };
  }

#ifdef DOXYGEN // {
  /// A fixed-size vector of bits.
  ///
  /// This is compiled to either `std::bitset` or `__gnu_debug::bitset`
  /// depending on the compilation mode.
  template <size_t N> class bitset {};

  /// A mapping from keys to values.
  ///
  /// This is compiled to either `std::map` or `__gnu_debug::map` depending on
  /// the compilation mode.
  template <typename K, typename T, typename C = std::less<K>, typename A = std::allocator<std::pair<const K, T>>> class map {};

  /// A set of values.
  ///
  /// This is compiled to either `std::set` or `__gnu_debug::set` depending on
  /// the compilation mode.
  template <typename T, typename C = std::less<T>, typename A = std::allocator<T>> class set {};

  /// Just a string.
  ///
  /// This is compiled to either `std::string` or `__gnu_debug::string`
  /// depending on the compilation mode.
  class string {};

  /// A dynamic vector of values.
  ///
  /// This is compiled to either `std::vector` or `__gnu_debug::vector`
  /// depending on the compilation mode.
  template <typename T, typename A = std::allocator<T>> class vector {};
#endif // } DOXYGEN

#ifndef CPL_WITHOUT_COLLECTIONS // {

#ifdef CPL_FAST // {
  // Compiles to the standard version of a bitset.
  template <size_t N> using bitset = std::bitset<N>;

  // Compiles to the standard version of a map.
  //
  // @todo Should we convert `at` to a faster, unchecked operation?
  template <typename K, typename T, typename C = std::less<K>, typename A = std::allocator<std::pair<const K, T>>>
  using map = std::map<K, T, C, A>;

  // Compiles to the standard version of a set.
  template <typename T, typename C = std::less<T>, typename A = std::allocator<T>> using set = std::set<T, C, A>;

  // Compiles to the standard version of a string.
  using string = std::string;

  // Compiles to the standard version of a vector.
  //
  // @todo Should we convert `at` to a faster, unchecked operation?
  template <typename T, typename A = std::allocator<T>> using vector = std::vector<T, A>;
#endif // } CPL_FAST

#ifdef CPL_SAFE // {
  // Compiles to the debug version of a bitset.
  template <size_t N> using bitset = __gnu_debug::bitset<N>;

  // Compiles to the debug version of a map.
  template <typename K, typename T, typename C = std::less<K>, typename A = std::allocator<std::pair<const K, T>>>
  using map = __gnu_debug::map<K, T, C, A>;

  // Compiles to the debug version of a set.
  template <typename T, typename C = std::less<T>, typename A = std::allocator<T>> using set = __gnu_debug::set<T, C, A>;

  // Compiles to the debug version of a string.
  using string = __gnu_debug::string;

  // Compiles to the debug version of a vector.
  template <typename T, typename A = std::allocator<T>> using vector = __gnu_debug::vector<T, A>;
#endif // } CPL_SAFE

#endif // } CPL_WITHOUT_COLLECTIONS
}
