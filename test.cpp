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

  TEST_CASE("constructing a uref") {
    REQUIRE(Foo::live_objects == 0);
    THEN("we can make unique data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> made_ptr = cpl::make_uref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("we can move it") {
        cpl::uref<Bar> moved_ptr = std::move(made_ptr);
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::uref<Foo> s_foo_uref = cpl::make_uref<Foo>();

#ifdef MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS
  /// Ensure there's no copying `cpl::uref`.
  MUST_NOT_COMPILE(Foo, cpl::uref<Foo>{ s_foo_uref }, "copying unique references");
#endif // MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS

  /// Ensure there's no assignment between `cpl::uref`.
  MUST_NOT_COMPILE(Foo, s_foo_uref = s_foo_uref, "assigning unique references");

  TEST_CASE("casting a uref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a uref") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_uref = cpl::make_uref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("if we move it to a reference to a base class") {
        cpl::uref<Foo> foo_ref{ std::move(bar_uref) };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can static_cast it back to a reference to the derived class") {
          cpl::uref<Bar> cast_bar_ref = cpl::cast_static<Bar>(std::move(foo_ref));
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can dynamic_cast it back to a reference to the derived class") {
          cpl::uref<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(std::move(foo_ref));
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can reinterpret_cast it back to a reference to the derived class") {
          cpl::uref<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(std::move(foo_ref));
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can move it to a reference to a const") {
        cpl::uref<const Bar> const_bar_ref{ std::move(bar_uref) };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can const_cast it back to a reference mutable") {
          cpl::uref<Bar> cast_bar_ref = cpl::cast_const<Bar>(std::move(const_bar_ref));
          REQUIRE(Foo::live_objects == 1);
        }
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing a uptr") {
    REQUIRE(Foo::live_objects == 0);
    THEN("we have a default constructor") {
      cpl::uptr<Foo> default_ptr;
      THEN("we can move it") {
        default_ptr = std::move(default_ptr);
      }
    }
    THEN("we can explicitly construct a nullptr") {
      cpl::uptr<Foo> null_ptr(nullptr);
      THEN("we can assign a nullptr to it") {
        null_ptr = nullptr;
      }
    }
    THEN("we can make unique data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> made_ptr = cpl::make_uptr<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("we can move it") {
        cpl::uptr<Bar> moved_ptr = std::move(made_ptr);
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::uptr<Foo> s_foo_uptr;

#ifdef MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS
  /// Ensure there's no copying `cpl::uptr`.
  MUST_NOT_COMPILE(Foo, cpl::uptr<Foo>{ s_foo_uptr }, "copying unique pointers");
#endif // MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS

  /// Ensure there's no assignment between `cpl::uptr`.
  MUST_NOT_COMPILE(Foo, s_foo_uptr = s_foo_uptr, "assigning unique pointers");

  TEST_CASE("casting a uptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a unique pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_uptr = cpl::make_uptr<Bar>(foo, bar);
      REQUIRE(Foo::live_objects == 1);
      THEN("if we move it to a pointer to a base class") {
        cpl::uptr<Foo> foo_uptr{ std::move(bar_uptr) };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can static_cast it back to a pointer to the derived class") {
          cpl::uptr<Bar> cast_bar_uptr = cpl::cast_static<Bar>(std::move(foo_uptr));
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can dynamic_cast it back to a pointer to the derived class") {
          cpl::uptr<Bar> cast_bar_uptr = cpl::cast_dynamic<Bar>(std::move(foo_uptr));
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can reinterpret_cast it back to a pointer to the derived class") {
          cpl::uptr<Bar> cast_bar_uptr = cpl::cast_dynamic<Bar>(std::move(foo_uptr));
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can move it to a pointer to a const") {
        cpl::uptr<const Bar> const_bar_uptr{ std::move(bar_uptr) };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can const_cast it back to a pointer mutable") {
          cpl::uptr<Bar> cast_bar_uptr = cpl::cast_const<Bar>(std::move(const_bar_uptr));
          REQUIRE(Foo::live_objects == 1);
        }
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting a uref to a uptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a unique reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_uref{ cpl::make_uref<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects == 1);
      THEN("we can move it to a pointer") {
        cpl::uptr<Bar> bar_uptr{ std::move(bar_uref) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a pointer") {
        cpl::uptr<Bar> bar_uptr;
        bar_uptr = std::move(bar_uref);
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can move it to a pointer to const") {
        cpl::uptr<const Bar> const_bar_uptr{ std::move(bar_uref) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a pointer to const") {
        cpl::uptr<const Bar> const_bar_uptr;
        const_bar_uptr = std::move(bar_uref);
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can move it to a pointer to the base class") {
        cpl::uptr<Foo> foo_uptr{ std::move(bar_uref) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a pointer to the base class") {
        cpl::uptr<Foo> foo_uptr;
        foo_uptr = std::move(bar_uref);
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can move it to a pointer to a const base class") {
        cpl::uptr<const Foo> const_foo_uptr{ std::move(bar_uref) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can assign it to a pointer to a const base class") {
        cpl::uptr<const Foo> const_foo_uptr;
        const_foo_uptr = std::move(bar_uref);
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("converting a uptr to a uref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a unique pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_uptr{ cpl::make_uptr<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects == 1);
      THEN("we can move it to a reference") {
        cpl::uref<Bar> bar_uref{ std::move(bar_uptr) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can move it to a reference to const") {
        cpl::uref<const Bar> const_bar_uref{ std::move(bar_uptr) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can move it to a reference to the base class") {
        cpl::uref<Foo> foo_uref{ std::move(bar_uptr) };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can move it to a reference to a const base class") {
        cpl::uref<const Foo> const_foo_uref{ std::move(bar_uptr) };
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Ensure there's no implicit conversion from pointer to reference.
  MUST_NOT_COMPILE(Foo, s_foo_uref = cpl::uptr<T>(), "implicit conversion of pointer to a reference");

  TEST_CASE("constructing a ref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      REQUIRE(Foo::live_objects == 1);
      THEN("we can construct an unsafe reference to it") {
        cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can copy it to another reference") {
          cpl::ref<Bar> bar_ref_copy{ bar_ref };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to another reference") {
            bar_ref_copy = bar_ref;
            REQUIRE(Foo::live_objects == 1);
          }
        }
        THEN("we can copy it to a reference to const") {
          cpl::ref<const Bar> const_bar_ref_copy{ bar_ref };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to a reference to const") {
            const_bar_ref_copy = bar_ref;
            REQUIRE(Foo::live_objects == 1);
          }
        }
        THEN("we can copy it to a reference to a base class") {
          cpl::ref<Foo> foo_ref{ bar_ref };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to a reference to a base class") {
            foo_ref = bar_ref;
            REQUIRE(Foo::live_objects == 1);
          }
        }
        THEN("we can copy it to a reference to a const base class") {
          cpl::ref<const Foo> const_foo_ref{ bar_ref };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to a reference to a const base class") {
            const_foo_ref = bar_ref;
            REQUIRE(Foo::live_objects == 1);
          }
        }
      }
    }
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a null pointer") {
      cpl::ptr<Foo> null_foo_ptr;
      THEN("using to construct a reference will be " CPL_VARIANT) {
        REQUIRE_CPL_THROWS(cpl::ref<Foo>{ null_foo_ptr });
      }
    }
  }

  /// Ensure it is not possible to default construct a `cpl::ref`.
  MUST_NOT_COMPILE(Foo, cpl::ref<T>(), "default reference construction");

  /// Ensure it is not possible to explicitly construct a null `cpl::ref`.
  MUST_NOT_COMPILE(Foo, cpl::ref<T>(nullptr), "explicit null reference construction");

  /// Ensure it is not possible to construct a `cpl::ref` from a raw reference.
  MUST_NOT_COMPILE(Foo, cpl::ref<T>{(T*)nullptr }, "unsafe reference construction");

  /// Ensure it is not possible to construct a `cpl::ref` to an unrelated type.
  MUST_NOT_COMPILE(Foo, cpl::ref<T>{ cpl::unsafe_ref<int>(*(int*)nullptr) }, "unrelated reference construction");

  /// Ensure it is not possible to construct a `cpl::ref` to a sub-class.
  MUST_NOT_COMPILE(Bar, cpl::ref<T>{ cpl::unsafe_ref<Foo>(*(Foo*)nullptr) }, "sub-class reference construction");

  /// Ensure it is not possible to construct a `cpl::ref` to violate `const`-ness.
  MUST_NOT_COMPILE(Foo, cpl::ref<T>{ cpl::unsafe_ref<const Foo>(*(const Foo*)nullptr) }, "const violation reference construction");

  TEST_CASE("casting a ref") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a borrowed reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects == 1);
      THEN("if we copy it to a reference to a base class") {
        cpl::ref<Foo> foo_ref{ bar_ref };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can static_cast it back to a reference to the derived class") {
          cpl::ref<Bar> cast_bar_ref = cpl::cast_static<Bar>(foo_ref);
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can dynamic_cast it back to a reference to the derived class") {
          cpl::ref<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(foo_ref);
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can reinterpret_cast it back to a reference to the derived class") {
          cpl::ref<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(foo_ref);
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can copy it to copy a reference to a const") {
        cpl::ref<const Bar> const_bar_ref{ bar_ref };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can const_cast it back to a reference mutable") {
          cpl::ref<Bar> cast_bar_ref = cpl::cast_const<Bar>(const_bar_ref);
          REQUIRE(Foo::live_objects == 1);
        }
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  TEST_CASE("constructing a ptr") {
    REQUIRE(Foo::live_objects == 0);
    THEN("we have a default constructor") {
      cpl::ptr<Foo> default_ptr;
      THEN("we can assign it") {
        default_ptr = default_ptr;
      }
    }
    THEN("we can explicitly construct a nullptr") {
      cpl::ptr<Foo> null_ptr{ nullptr };
      THEN("we can assign a nullptr to it") {
        null_ptr = nullptr;
      }
    }
    REQUIRE(Foo::live_objects == 0);
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      REQUIRE(Foo::live_objects == 1);
      THEN("we can construct an unsafe pointer to it") {
        cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can copy it to another pointer") {
          cpl::ptr<Bar> bar_ptr_copy{ bar_ptr };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to another pointer") {
            bar_ptr_copy = bar_ptr;
            REQUIRE(Foo::live_objects == 1);
          }
        }
        THEN("we can copy it to a pointer to const") {
          cpl::ptr<const Bar> const_bar_ptr_copy{ bar_ptr };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to a pointer to const") {
            const_bar_ptr_copy = bar_ptr;
            REQUIRE(Foo::live_objects == 1);
          }
        }
        THEN("we can copy it to a pointer to a base class") {
          cpl::ptr<Foo> foo_ptr{ bar_ptr };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to a pointer to a base class") {
            foo_ptr = bar_ptr;
            REQUIRE(Foo::live_objects == 1);
          }
        }
        THEN("we can copy it to a pointer to a const base class") {
          cpl::ptr<const Foo> const_foo_ptr{ bar_ptr };
          REQUIRE(Foo::live_objects == 1);
          THEN("we can assign it to a pointer to a const base class") {
            const_foo_ptr = bar_ptr;
            REQUIRE(Foo::live_objects == 1);
          }
        }
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Ensure it is not possible to construct a `cpl::ptr` from a raw pointer.
  MUST_NOT_COMPILE(Foo, cpl::ptr<T>{(T*)nullptr }, "unsafe pointer construction");

  /// Ensure it is not possible to construct a `cpl::ptr` to an unrelated type.
  MUST_NOT_COMPILE(Foo, cpl::ptr<T>{ cpl::ptr<int>() }, "unrelated pointer construction");

  /// Ensure it is not possible to construct a `cpl::ptr` to a sub-class.
  MUST_NOT_COMPILE(Bar, cpl::ptr<T>{ cpl::ptr<Foo>() }, "sub-class pointer construction");

  /// Ensure it is not possible to construct a `cpl::ptr` to violate `const`-ness.
  MUST_NOT_COMPILE(Foo, cpl::ptr<T>{ cpl::ptr<const Foo>() }, "const violation pointer construction");

  TEST_CASE("casting a ptr") {
    REQUIRE(Foo::live_objects == 0);
    GIVEN("a borrowed pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects == 1);
      THEN("if we copy it to a pointer to a base class") {
        cpl::ptr<Foo> foo_ptr{ bar_ptr };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can static_cast it back to a pointer to the derived class") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_static<Bar>(foo_ptr);
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can dynamic_cast it back to a pointer to the derived class") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(foo_ptr);
          REQUIRE(Foo::live_objects == 1);
        }
        THEN("we can reinterpret_cast it back to a pointer to the derived class") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(foo_ptr);
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can copy it to a pointer to a const") {
        cpl::ptr<const Bar> const_bar_ptr{ bar_ptr };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can const_cast it back to a pointer mutable") {
          cpl::ptr<Bar> cast_bar_ptr = cpl::cast_const<Bar>(const_bar_ptr);
          REQUIRE(Foo::live_objects == 1);
        }
      }
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
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a pointer") {
        cpl::ptr<Bar> bar_ptr{ bar_ref };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can assign it to a pointer") {
          bar_ptr = bar_ref;
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can copy it to a pointer to const") {
        cpl::ptr<const Bar> const_bar_ptr{ bar_ref };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can assign it to a pointer to const") {
          const_bar_ptr = bar_ref;
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can copy it to a pointer to the base class") {
        cpl::ptr<Foo> foo_ptr{ bar_ref };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can assign it to a pointer to the base class") {
          foo_ptr = bar_ref;
          REQUIRE(Foo::live_objects == 1);
        }
      }
      THEN("we can copy it to a pointer to a const base class") {
        cpl::ptr<const Foo> const_foo_ptr{ bar_ref };
        REQUIRE(Foo::live_objects == 1);
        THEN("we can assign it to a pointer to a const base class") {
          const_foo_ptr = bar_ref;
          REQUIRE(Foo::live_objects == 1);
        }
      }
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
      REQUIRE(Foo::live_objects == 1);
      THEN("we can copy it to a reference") {
        cpl::ref<Bar> bar_ref{ bar_ptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a reference to const") {
        cpl::ref<const Bar> const_bar_ref{ bar_ptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a reference to the base class") {
        cpl::ref<Foo> foo_ref{ bar_ptr };
        REQUIRE(Foo::live_objects == 1);
      }
      THEN("we can copy it to a reference to a const base class") {
        cpl::ref<const Foo> const_foo_ref{ bar_ptr };
        REQUIRE(Foo::live_objects == 1);
      }
    }
    REQUIRE(Foo::live_objects == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static Foo s_raw_foo;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ref<Foo> s_foo_ref{ cpl::unsafe_ref<Foo>(s_raw_foo) };

  /// Ensure there's no implicit conversion from pointer to reference.
  MUST_NOT_COMPILE(Foo, s_foo_ref = cpl::ptr<T>(), "implicit conversion of pointer to a reference");

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

  /// Ensure there's no implicit conversion from unique pointer to reference.
  MUST_NOT_COMPILE(Foo, s_foo_ref = cpl::uptr<T>(), "implicit conversion of unique pointer to a reference");

  /// Ensure there's no implicit conversion from derived unique pointer to reference.
  MUST_NOT_COMPILE(Bar, s_foo_ref = cpl::uptr<T>(), "implicit conversion of unique pointer to a reference");
}
