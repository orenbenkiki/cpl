/// @file
/// Test the clever protection library (CPL).

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "cpl.hpp"

#ifdef DOXYGEN
/// Require that the expression will be checked in the safe variant but not in
/// the fast variant.
#define REQUIRE_CPL_THROWS(EXPRESSION)

/// If this is defined, we compile out tests that dereference invalid pointers
/// to demonstrate they are indeed invalid.
#define AVOID_INVALID_MEMORY_ACCESS
#endif // DOXYGEN

#ifdef CPL_FAST
#define REQUIRE_CPL_THROWS(EXPRESSION) REQUIRE_NOTHROW(EXPRESSION)
#endif
#ifdef CPL_SAFE
#define REQUIRE_CPL_THROWS(EXPRESSION) REQUIRE_THROWS(EXPRESSION)
#endif

/// Statically assert that the `EXPRESSION` does not compile.
///
/// The expression should be parameterized by a typename `T` and should *not*
/// compile if `T` is specified to be the `TYPE`.
///
/// This uses some helper types (@ref test::sink and @ref test::sink_t), and
/// some macros (@ref MUST_NOT_COMPILE_WITH_ID, @ref CAT, and @ref CAT2), to
/// force the C pre-processor to generate a unique name for the test class we
/// need to generate for each test.
///
/// The implementation is to be blamed on
/// http://stackoverflow.com/questions/23547831/assert-that-code-does-not-compile/23549126#23549126
/// and is a prime example for what makes C++, well, C++: On the one hand, it
/// is impressive we can implement such a macro. On the other hand, *shudder*.
#define MUST_NOT_COMPILE(TYPE, EXPRESSION, MESSAGE) MUST_NOT_COMPILE_WITH_ID(TYPE, EXPRESSION, MESSAGE, CAT(test, __COUNTER__))

/// Implement @ref MUST_NOT_COMPILE with a given unique identifier (which we
/// generate from the non-standard `__COUNTER__` macro).
#define MUST_NOT_COMPILE_WITH_ID(TYPE, EXPRESSION, MESSAGE, ID)                                \
  template <typename T, typename = void> struct ID : std::true_type {};                        \
  template <typename T> struct ID<T, test::sink_t<decltype(EXPRESSION)>> : std::false_type {}; \
  static_assert(ID<TYPE>::value, MESSAGE)

/// Force expansion of the two operands before concatenation.
#define CAT(a, b) CAT2(a, b) // force expand

/// Actually concatenate two expanded operands.
#define CAT2(a, b) a##b

/// Used for simple copy parameters.
#define COPY(X) X

/// Use for moving parameters.
#define MOVE(X) std::move(X)

/// Test the Clever Protection Library.
namespace test {

  /// Used by the @ref MUST_NOT_COMPILE macro.
  template <class T> struct sink { typedef void type; };

  /// Used by the @ref MUST_NOT_COMPILE macro.
  template <class T> using sink_t = typename sink<T>::type;

  /// A sample data type to refer to in the tests.
  struct Foo {
    /// How many live Foo objects exist.
    static int live_objects;

    /// Hold some meaningless data.
    int foo;

    /// Make the default constructor deterministic.
    Foo() : Foo(0) {
    }

    /// An assignment operator for testing `is`.
    Foo& operator=(nullptr_t) {
      foo = 0;
    }

    /// Allow constructing different instances for the tests.
    explicit Foo(int foo) : foo(foo) {
      if (foo > 0) {
        ++live_objects;
      }
    }

    /// Maintain the count of live objects.
    Foo(const Foo& foo) : Foo(foo.foo) {
    }

    /// Maintain the count of live objects.
    ///
    /// TRICKY: Also change the value of the `foo` member so any dangling
    /// pointers will see the wrong value. We _hope_ such access won't fault -
    /// it will be flagged by memory corruption tools.
    virtual ~Foo() {
      if (foo > 0) {
        --live_objects;
        foo = -1;
      }
    }

    /// Compare two Foo objects for equality.
    bool operator==(const Foo& rhs) const {
      return foo == rhs.foo;
    }

    /// Compare two Foo objects for equality.
    bool operator!=(const Foo& rhs) const {
      return foo != rhs.foo;
    }
  };

  int Foo::live_objects = 0;

  /// A sub-class to test conversions.
  struct Bar : Foo {
    /// Hold more meaningless data.
    int bar;

    /// Make the default constructor deterministic.
    Bar() : Bar(0, 0) {
    }

    /// Allow constructing different instances for the tests.
    explicit Bar(int foo, int bar) : Foo(foo), bar(bar) {
    }

    /// Change the value of `bar` to help detect dangling pointers.
    ~Bar() {
      bar = -1;
    }
  };

  /// Test the @ref MUST_NOT_COMPILE macro.
  MUST_NOT_COMPILE(Foo, T("string"), "invalid constructor parameter");

  /// Test the @ref CPL_VARIANT macro.
  TEST_CASE("CPL variant name") {
#ifdef CPL_FAST
    GIVEN("the fast variant is compiled") {
      THEN("CPL_VARIANT will be defined to \"fast\"") {
        REQUIRE(cpl::string(CPL_VARIANT) == "fast");
      }
    }
#endif
#ifdef CPL_SAFE
    GIVEN("the safe variant is compiled") {
      THEN("CPL_VARIANT will be defined to \"safe\"") {
        REQUIRE(cpl::string(CPL_VARIANT) == "safe");
      }
    }
#endif
  }

/// Verify that a reference type construction behaves as expected.
#define VERIFY_VALID_REFERENCE_CONSTRUCTION(REF, PTR, PASS)       \
  REQUIRE(Foo::live_objects == 1);                                \
  THEN("we can copy it to another reference") {                   \
    cpl::REF<Bar> bar_ref_copy{ PASS(bar_ref) };                  \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can assign it to another reference") {                 \
    cpl::REF<Bar> bar_ref_copy = PASS(bar_ref);                   \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can copy it to a reference to const") {                \
    cpl::REF<const Bar> const_bar_ref_copy{ PASS(bar_ref) };      \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can assign it to a reference to const") {              \
    cpl::REF<const Bar> const_bar_ref_copy = PASS(bar_ref);       \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can copy it to a reference to a base class") {         \
    cpl::REF<Foo> foo_ref{ PASS(bar_ref) };                       \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can assign it to a reference to a base class") {       \
    cpl::REF<Foo> foo_ref = PASS(bar_ref);                        \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can copy it to a reference to a const base class") {   \
    cpl::REF<const Foo> const_foo_ref{ PASS(bar_ref) };           \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  THEN("we can assign it to a reference to a const base class") { \
    cpl::REF<const Foo> const_foo_ref = PASS(bar_ref);            \
    REQUIRE(Foo::live_objects == 1);                              \
  }                                                               \
  }                                                               \
  REQUIRE(Foo::live_objects == 0);                                \
  GIVEN("a null pointer") {                                       \
    cpl::PTR<Foo> null_foo_ptr;                                   \
    THEN("using to construct a reference will be " CPL_VARIANT) { \
      REQUIRE_CPL_THROWS(cpl::REF<Foo>{ PASS(null_foo_ptr) });    \
    }

/// Verify that a reference type construction behaves as expected.
#define VERIFY_VALID_POINTER_CONSTRUCTION(REF, PTR, PASS)       \
  REQUIRE(Foo::live_objects == 1);                              \
  THEN("we can copy it to another pointer") {                   \
    cpl::PTR<Bar> bar_ptr_copy{ PASS(bar_ptr) };                \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to another pointer") {                 \
    cpl::PTR<Bar> bar_ptr_copy;                                 \
    bar_ptr_copy = PASS(bar_ptr);                               \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a pointer to const") {                \
    cpl::PTR<const Bar> const_bar_ptr_copy{ PASS(bar_ptr) };    \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer to const") {              \
    cpl::PTR<const Bar> const_bar_ptr_copy;                     \
    const_bar_ptr_copy = PASS(bar_ptr);                         \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a pointer to a base class") {         \
    cpl::PTR<Foo> foo_ptr{ PASS(bar_ptr) };                     \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer to a base class") {       \
    cpl::PTR<Foo> foo_ptr;                                      \
    foo_ptr = PASS(bar_ptr);                                    \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a pointer to a const base class") {   \
    cpl::PTR<const Foo> const_foo_ptr{ PASS(bar_ptr) };         \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer to a const base class") { \
    cpl::PTR<const Foo> const_foo_ptr;                          \
    const_foo_ptr = PASS(bar_ptr);                              \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  }                                                             \
  REQUIRE(Foo::live_objects == 0);                              \
  THEN("we have a default constructor") {                       \
    cpl::PTR<Foo> default_ptr;                                  \
    THEN("we can assign it") {                                  \
      default_ptr = PASS(default_ptr);                          \
    }                                                           \
  }                                                             \
  THEN("we can explicitly construct a nullptr") {               \
    cpl::PTR<Foo> null_ptr{ nullptr };                          \
    THEN("we can assign a nullptr to it") {                     \
      null_ptr = nullptr;                                       \
    }

/* @todo Is there a way to make this work?
THEN("we can copy it to a const reference") {
  const cpl::REF<Bar> bar_const_ref_copy{ const_bar_ref_copy };
  REQUIRE(Foo::live_objects == 1);
}
THEN("we can assign it to a const reference") {
  const cpl::REF<Bar> bar_const_ref_copy = const_bar_ref_copy;
  REQUIRE(Foo::live_objects == 1);
}
*/

/// Ensure attempts for invalid reference construction will fail.
#define VERIFY_INVALID_REFERENCE_CONSTRUCTION(REF, PTR, MAKE_REF, MAKE_REF_CONST, PASS)                       \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, REF) = cpl::PTR<T>(), "implicit conversion of pointer to a reference");   \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(), "default reference construction");                                     \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(nullptr), "explicit null reference construction");                        \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>((T*)nullptr), "unsafe reference construction");                           \
  MUST_NOT_COMPILE(int, cpl::REF<Foo>(MAKE_REF), "unrelated reference construction");                         \
  MUST_NOT_COMPILE(int, CAT(s_foo_, REF) = (MAKE_REF), "unrelated reference assignment");                     \
  MUST_NOT_COMPILE(Foo, cpl::REF<Bar>(MAKE_REF), "sub-class reference construction");                         \
  MUST_NOT_COMPILE(Foo, CAT(s_bar_, REF) = (MAKE_REF), "sub-class reference assignment");                     \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(MAKE_REF_CONST), "const violation reference construction");               \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, REF) = (MAKE_REF_CONST), "const violation reference assignment");         \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(PASS(CAT(s_foo_const_, REF))), "const violation reference construction"); \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(PASS(CAT(s_const_foo_, REF))), "const violation reference construction")

/// Ensure attempts for invalid pointer construction will fail.
#define VERIFY_INVALID_POINTER_CONSTRUCTION(PTR, PASS)                                                     \
  MUST_NOT_COMPILE(Foo, cpl::PTR<T>{(T*)nullptr }, "unsafe pointer construction");                         \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, PTR) = (T*)nullptr, "unsafe pointer assignment");                      \
  MUST_NOT_COMPILE(int, cpl::PTR<Foo>{ PASS(cpl::PTR<T>()) }, "unrelated pointer construction");           \
  MUST_NOT_COMPILE(int, CAT(s_foo_, PTR) = PASS(cpl::PTR<T>()), "unrelated pointer assignment");           \
  MUST_NOT_COMPILE(Foo, cpl::PTR<Bar>{ PASS(cpl::PTR<T>()) }, "sub-class pointer construction");           \
  MUST_NOT_COMPILE(Foo, CAT(s_bar_, PTR) = PASS(cpl::PTR<T>()), "sub-class pointer construction");         \
  MUST_NOT_COMPILE(Foo, cpl::PTR<T>{ PASS(cpl::PTR<const T>()) }, "const violation pointer construction"); \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, PTR) = PASS(cpl::PTR<const T>()), "const violation pointer construction")

/// Ensure casting a reference works as expected.
#define VERIFY_REFERENCE_CASTS(REF, PASS)                                         \
  THEN("if we copy it to a reference to a base class") {                          \
    cpl::REF<Foo> foo_ref{ PASS(bar_ref) };                                       \
    REQUIRE(Foo::live_objects == 1);                                              \
    THEN("we can static_cast it back to a reference to the derived class") {      \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_static<Bar>(PASS(foo_ref));          \
      REQUIRE(Foo::live_objects == 1);                                            \
    }                                                                             \
    THEN("we can dynamic_cast it back to a reference to the derived class") {     \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(PASS(foo_ref));         \
      REQUIRE(Foo::live_objects == 1);                                            \
    }                                                                             \
    THEN("we can reinterpret_cast it back to a reference to the derived class") { \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(PASS(foo_ref));         \
      REQUIRE(Foo::live_objects == 1);                                            \
    }                                                                             \
  }                                                                               \
  THEN("we can copy it to a reference to a const") {                              \
    cpl::REF<const Bar> const_bar_ref{ PASS(bar_ref) };                           \
    REQUIRE(Foo::live_objects == 1);                                              \
    THEN("we can const_cast it back to a reference mutable") {                    \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_const<Bar>(PASS(const_bar_ref));     \
      REQUIRE(Foo::live_objects == 1);                                            \
    }                                                                             \
  }

/// Ensure casting a pointer works as expected.
#define VERIFY_POINTER_CASTS(PTR, PASS)                                         \
  REQUIRE(Foo::live_objects == 1);                                              \
  THEN("if we copy it to a pointer to a base class") {                          \
    cpl::PTR<Foo> foo_ptr{ PASS(bar_ptr) };                                     \
    REQUIRE(Foo::live_objects == 1);                                            \
    THEN("we can static_cast it back to a pointer to the derived class") {      \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_static<Bar>(PASS(foo_ptr));        \
      REQUIRE(Foo::live_objects == 1);                                          \
    }                                                                           \
    THEN("we can dynamic_cast it back to a pointer to the derived class") {     \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(PASS(foo_ptr));       \
      REQUIRE(Foo::live_objects == 1);                                          \
    }                                                                           \
    THEN("we can reinterpret_cast it back to a pointer to the derived class") { \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(PASS(foo_ptr));       \
      REQUIRE(Foo::live_objects == 1);                                          \
    }                                                                           \
  }                                                                             \
  THEN("we can copy it to a pointer to a const") {                              \
    cpl::PTR<const Bar> const_bar_ptr{ PASS(bar_ptr) };                         \
    REQUIRE(Foo::live_objects == 1);                                            \
    THEN("we can const_cast it back to a pointer mutable") {                    \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_const<Bar>(PASS(const_bar_ptr));   \
      REQUIRE(Foo::live_objects == 1);                                          \
    }                                                                           \
  }

/// Ensure converting a reference to a pointer works as expected.
#define VERIFY_CONVERTING_REFERENCE_TO_POINTER(REF, PTR, PASS)  \
  REQUIRE(Foo::live_objects == 1);                              \
  THEN("we can copy it to a pointer") {                         \
    cpl::PTR<Bar> bar_ptr{ PASS(bar_ref) };                     \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer") {                       \
    cpl::PTR<Bar> bar_ptr;                                      \
    bar_ptr = PASS(bar_ref);                                    \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a pointer to const") {                \
    cpl::ptr<const Bar> const_bar_ptr{ PASS(bar_ref) };         \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer to const") {              \
    cpl::ptr<const Bar> const_bar_ptr;                          \
    const_bar_ptr = PASS(bar_ref);                              \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a pointer to the base class") {       \
    cpl::ptr<Foo> foo_ptr{ PASS(bar_ref) };                     \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer to the base class") {     \
    cpl::ptr<Foo> foo_ptr;                                      \
    foo_ptr = PASS(bar_ref);                                    \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a pointer to a const base class") {   \
    cpl::ptr<const Foo> const_foo_ptr{ PASS(bar_ref) };         \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can assign it to a pointer to a const base class") { \
    cpl::ptr<const Foo> const_foo_ptr;                          \
    const_foo_ptr = PASS(bar_ref);                              \
    REQUIRE(Foo::live_objects == 1);                            \
  }

/// Ensure converting a pointer to a reference works as expected.
#define VERIFY_CONVERTING_POINTER_TO_REFERENCE(REF, PTR, PASS)  \
  REQUIRE(Foo::live_objects == 1);                              \
  THEN("we can copy it to a reference") {                       \
    cpl::REF<Bar> bar_ref{ PASS(bar_ptr) };                     \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a reference to const") {              \
    cpl::REF<const Bar> const_bar_ref{ PASS(bar_ptr) };         \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a reference to the base class") {     \
    cpl::REF<Foo> foo_ref{ PASS(bar_ptr) };                     \
    REQUIRE(Foo::live_objects == 1);                            \
  }                                                             \
  THEN("we can copy it to a reference to a const base class") { \
    cpl::ref<const Foo> const_foo_ref{ PASS(bar_ptr) };         \
    REQUIRE(Foo::live_objects == 1);                            \
  }

  TEST_CASE("constructing an sref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we make shared data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_ref = cpl::make_sref<Bar>(foo, bar);
      VERIFY_VALID_REFERENCE_CONSTRUCTION(sref, sptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static cpl::sref<Foo> s_foo_sref = cpl::make_sref<Foo>();

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static cpl::sref<Bar> s_bar_sref{ cpl::make_sref<Bar>() };

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static cpl::sref<const Foo> s_foo_const_sref{ cpl::make_sref<const Foo>() };

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static const cpl::sref<Foo> s_const_foo_sref{ cpl::make_sref<Foo>() };

  /// Verify `cpl::sref` is protected against invalid construction.
  VERIFY_INVALID_REFERENCE_CONSTRUCTION(sref, sptr, cpl::make_sref<T>(0), cpl::make_sref<const T>(0), COPY);

  TEST_CASE("casting an sref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("an sref") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_ref = cpl::make_sref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      VERIFY_REFERENCE_CASTS(sref, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing an sptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we make shared data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_ptr{ cpl::make_sptr<Bar>(foo, bar) };
      VERIFY_VALID_POINTER_CONSTRUCTION(sref, sptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::sptr<Foo> s_foo_sptr;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::sptr<Bar> s_bar_sptr;

  /// Verify `cpl::sptr` is protected against invalid construction.
  VERIFY_INVALID_POINTER_CONSTRUCTION(sptr, COPY);

  TEST_CASE("casting an sptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a shared pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_ptr = cpl::make_sptr<Bar>(foo, bar);
      VERIFY_POINTER_CASTS(sptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting an sref to an sptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a shared reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_ref{ cpl::make_sref<Bar>(foo, bar) };
      VERIFY_CONVERTING_REFERENCE_TO_POINTER(sref, sptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting an sptr to an sref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a shared pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_ptr{ cpl::make_sptr<Bar>(foo, bar) };
      VERIFY_CONVERTING_POINTER_TO_REFERENCE(sref, sptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing a uref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we make unique data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_ref = cpl::make_uref<Bar>(foo, bar);
      VERIFY_VALID_REFERENCE_CONSTRUCTION(uref, uptr, MOVE);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static cpl::uref<Foo> s_foo_uref = cpl::make_uref<Foo>();

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static cpl::uref<Bar> s_bar_uref{ cpl::make_uref<Bar>() };

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static cpl::uref<const Foo> s_foo_const_uref{ cpl::make_uref<const Foo>() };

  /// Used for @ref VERIFY_INVALID_REFERENCE_CONSTRUCTION.
  static const cpl::uref<Foo> s_const_foo_uref{ cpl::make_uref<Foo>() };

  /// Verify `cpl::uref` is protected against invalid construction.
  VERIFY_INVALID_REFERENCE_CONSTRUCTION(uref, uptr, cpl::make_uref<T>(0), cpl::make_uref<const T>(0), MOVE);

#ifdef MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS
  /// Ensure there's no copying `cpl::uref`.
  MUST_NOT_COMPILE(Foo, cpl::uref<T>{ s_foo_uref }, "copying unique references");
#endif // MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS

  TEST_CASE("casting a uref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a uref") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_ref = cpl::make_uref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      VERIFY_REFERENCE_CASTS(uref, MOVE);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing a uptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we make unique data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_ptr{ cpl::make_uptr<Bar>(foo, bar) };
      VERIFY_VALID_POINTER_CONSTRUCTION(uref, uptr, MOVE);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::uptr<Foo> s_foo_uptr;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::uptr<Bar> s_bar_uptr;

  /// Verify `cpl::uptr` is protected against invalid construction.
  VERIFY_INVALID_POINTER_CONSTRUCTION(uptr, MOVE);

#ifdef MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS
  /// Ensure there's no copying `cpl::uptr`.
  MUST_NOT_COMPILE(cpl::uptr<Foo>{ s_foo_uptr }, "copying unique pointers");
#endif // MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS

  TEST_CASE("casting a uptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a unique pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_ptr = cpl::make_uptr<Bar>(foo, bar);
      VERIFY_POINTER_CASTS(uptr, MOVE);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting a uref to a uptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a unique reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_ref{ cpl::make_uref<Bar>(foo, bar) };
      VERIFY_CONVERTING_REFERENCE_TO_POINTER(uref, uptr, MOVE);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting a uptr to a uref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a unique pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_ptr{ cpl::make_uptr<Bar>(foo, bar) };
      VERIFY_CONVERTING_POINTER_TO_REFERENCE(uref, uptr, MOVE);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing a ref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      VERIFY_VALID_REFERENCE_CONSTRUCTION(ref, ptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static Bar s_raw_bar;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ref<Foo> s_foo_ref{ cpl::unsafe_ref<Foo>(s_raw_bar) };

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ref<Bar> s_bar_ref{ cpl::unsafe_ref<Bar>(s_raw_bar) };

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ref<const Foo> s_foo_const_ref{ cpl::unsafe_ref<const Foo>(s_raw_bar) };

  /// Used for @ref MUST_NOT_COMPILE.
  static const cpl::ref<Foo> s_const_foo_ref{ cpl::unsafe_ref<Foo>(s_raw_bar) };

  /// Verify `cpl::ref` is protected against invalid construction.
  VERIFY_INVALID_REFERENCE_CONSTRUCTION(
    ref, ptr, cpl::ref<T>(cpl::unsafe_ref<T>(*(T*) nullptr)), cpl::ref<const T>(cpl::unsafe_ref<T>(*(T*) nullptr)), COPY);

  TEST_CASE("casting a ref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a borrowed reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects == 1);
      VERIFY_REFERENCE_CASTS(ref, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing a ptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr(raw_bar) };
      VERIFY_VALID_POINTER_CONSTRUCTION(ref, ptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ptr<Foo> s_foo_ptr;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ptr<Bar> s_bar_ptr;

  /// Verify `cpl::ptr` is protected against invalid construction.
  VERIFY_INVALID_POINTER_CONSTRUCTION(ptr, COPY);

  TEST_CASE("casting a ptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a borrowed pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
      VERIFY_POINTER_CASTS(ptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting a ref to a ptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a borrowed reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      VERIFY_CONVERTING_REFERENCE_TO_POINTER(ref, ptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting a ptr to a ref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a borrowed pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
      VERIFY_CONVERTING_POINTER_TO_REFERENCE(ref, ptr, COPY);
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("borrowing a held value") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we have a held reference") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::is<Bar> bar_is{ foo, bar };
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr{ bar_is };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr;
        bar_ptr = bar_is;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr{ bar_is };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr;
        bar_ptr = bar_is;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference") {
        cpl::ref<Bar> bar_ref{ bar_is };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed reference") {
        cpl::ref<Bar> bar_ref = cpl::unsafe_ref<Bar>(*bar_is.get());
        bar_ref = bar_is;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref{ bar_is };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref = cpl::unsafe_ref<Foo>(*bar_is.get());
        bar_ref = bar_is;
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("borrowing a shared indirection") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we have a shared reference") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_sref = cpl::make_sref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr{ bar_sref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr;
        bar_ptr = bar_sref;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr{ bar_sref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr;
        bar_ptr = bar_sref;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference") {
        cpl::ref<Bar> bar_ref{ bar_sref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed reference") {
        cpl::ref<Bar> bar_ref = cpl::unsafe_ref<Bar>(*bar_sref.get());
        bar_ref = bar_sref;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref{ bar_sref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref = cpl::unsafe_ref<Foo>(*bar_sref.get());
        bar_ref = bar_sref;
        REQUIRE(Foo::live_objects == 1);
      }
    }
    GIVEN("we have a non-null shared pointer") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_sptr = cpl::make_sptr<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr{ bar_sptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr;
        bar_ptr = bar_sptr;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr{ bar_sptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr;
        bar_ptr = bar_sptr;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference") {
        cpl::ref<Bar> bar_ref{ bar_sptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref{ bar_sptr };
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("borrowing a unique indirection") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("we have a unique reference") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_uref = cpl::make_uref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr{ bar_uref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr;
        bar_ptr = bar_uref;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr{ bar_uref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr;
        bar_ptr = bar_uref;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference") {
        cpl::ref<Bar> bar_ref{ bar_uref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed reference") {
        cpl::ref<Bar> bar_ref = cpl::unsafe_ref<Bar>(*bar_uref.get());
        bar_ref = bar_uref;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref{ bar_uref };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref = cpl::unsafe_ref<Foo>(*bar_uref.get());
        bar_ref = bar_uref;
        REQUIRE(Foo::live_objects == 1);
      }
    }
    GIVEN("we have a non-null unique pointer") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_uptr = cpl::make_uptr<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr{ bar_uptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer") {
        cpl::ptr<Bar> bar_ptr;
        bar_ptr = bar_uptr;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr{ bar_uptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a borrowed pointer to a base class") {
        cpl::ptr<Foo> bar_ptr;
        bar_ptr = bar_uptr;
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference") {
        cpl::ref<Bar> bar_ref{ bar_uptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a borrowed reference to a base class") {
        cpl::ref<Foo> bar_ref{ bar_uptr };
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }
}
