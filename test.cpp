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
/// some macros (@ref TEST_ID, @ref CAT, and @ref CAT2), to force the C
/// pre-processor to generate a unique name for the test class we need to
/// generate for each test.
///
/// The implementation is to be blamed on
/// http://stackoverflow.com/questions/23547831/assert-that-code-does-not-compile/23549126#23549126
/// and is a prime example for what makes C++, well, C++: On the one hand, it
/// is impressive we can implement such a macro. On the other hand, *shudder*.
#define MUST_NOT_COMPILE(TYPE, EXPRESSION, MESSAGE)                                                   \
  template <typename T, typename = void> struct TEST_ID() : std::true_type {};                        \
  template <typename T> struct TEST_ID()<T, test::sink_t<decltype(EXPRESSION)>> : std::false_type {}; \
  static_assert(TEST_ID()<TYPE>::value, MESSAGE)

/// Generate a unique identifier for use in @ref MUST_NOT_COMPILE.
#define TEST_ID() CAT(test_, __LINE__)

/// Force expansion of the two operands before concatenation.
#define CAT(a, b) CAT2(a, b) // force expand

/// Actually concatenate two expanded operands.
#define CAT2(a, b) a##b

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
      if (foo >= 0) {
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
      if (foo >= 0) {
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
  MUST_NOT_COMPILE(Foo, T("string"), "Invalid constructor parameter");

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

  TEST_CASE("constructing a ptr") {
    THEN("we have a default constructor") {
      cpl::ptr<Foo> default_ptr;
    }
    THEN("we can explicitly construct a nullptr") {
      cpl::ptr<Foo> null_ptr{ nullptr };
    }
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      THEN("we can construct an unsafe pointer to it") {
        cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
        THEN("we can use it to initialize another pointer") {
          cpl::ptr<Bar> bar_ptr_copy{ bar_ptr };
        }
        THEN("we can use it to initialize a pointer to a base class") {
          cpl::ptr<Foo> foo_ptr{ bar_ptr };
        }
      }
    }
  }

  TEST_CASE("casting ptr") {
    GIVEN("a borrowed pointer to a const derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
      THEN("if we view it as a pointer to a base class") {
        cpl::ptr<Foo> foo_ptr{ bar_ptr };
        THEN("we can static_cast it back to a pointer to the derived class") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_static<Bar>(foo_ptr);
        }
        THEN("we can dynamic_cast it back to a pointer to the derived class") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(foo_ptr);
        }
        THEN("we can reinterpret_cast it back to a pointer to the derived class") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(foo_ptr);
        }
      }
      THEN("we can view it as a pointer to a const") {
        cpl::ptr<const Bar> const_bar_ptr{ bar_ptr };
        THEN("we can const_cast it back to a pointer mutable") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_const<Bar>(const_bar_ptr);
        }
      }
    }
  }

  /// Ensure it is not possible to construct a `cpl::ptr` from a raw pointer.
  MUST_NOT_COMPILE(Foo, cpl::ptr<T>{(T*)nullptr }, "unsafe pointer construction");

  /// Ensure it is not possible to construct a `cpl::ptr` to an unrelated type.
  MUST_NOT_COMPILE(Foo, cpl::ptr<T>{ cpl::ptr<int>() }, "unrelated pointer construction");

  /// Ensure it is not possible to construct a `cpl::ptr` to a sub-class.
  MUST_NOT_COMPILE(Bar, cpl::ptr<T>{ cpl::ptr<Foo>() }, "sub-class pointer construction");

  /// Ensure it is not possible to construct a `cpl::ptr` to violate `const`-ness.
  MUST_NOT_COMPILE(Foo, cpl::ptr<T>{ cpl::ptr<const Foo>() }, "const violation pointer construction");
}
