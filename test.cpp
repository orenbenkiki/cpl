/// @file
/// Test the clever protection library (CPL).

#define CATCH_CONFIG_MAIN
#include "cpl.hpp"
#include "catch.hpp"

#ifdef DOXYGEN // {
/// Require that the expression will be checked in the safe variant but not in
/// the fast variant.
#define REQUIRE_CPL_THROWS(EXPRESSION)

/// If this is defined, we compile out tests that dereference invalid pointers
/// to demonstrate they are indeed invalid.
#define AVOID_INVALID_MEMORY_ACCESS
#endif // } DOXYGEN

#ifdef CPL_FAST // {
#define REQUIRE_CPL_THROWS(EXPRESSION) REQUIRE_NOTHROW(EXPRESSION)
#endif // } CPL_FAST

#ifdef CPL_SAFE // {
#define REQUIRE_CPL_THROWS(EXPRESSION) REQUIRE_THROWS(EXPRESSION)
#endif // } CPL_SAFE

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

void wtf() {
}

/// Test the Clever Protection Library.
namespace test {

  /// Used by the @ref MUST_NOT_COMPILE macro.
  template <class T> struct sink { typedef void type; };

  /// Used by the @ref MUST_NOT_COMPILE macro.
  template <class T> using sink_t = typename sink<T>::type;

  /// A sample data type to refer to in the tests.
  struct Foo {
    /// Which live non-empty objects exist.
    static cpl::set<const Foo*> live_objects;

    /// Ensure the live object set reflect the current state of the object.
    void sync_live_objects() const {
      if (foo > 0) {
        live_objects.insert(this);
      } else {
        live_objects.erase(this);
      }
    }

    /// Hold some meaningless data.
    int foo;

    /// Make the default constructor deterministic.
    Foo() : Foo(0) {
      sync_live_objects();
    }

    /// An assignment operator for testing `is`.
    Foo& operator=(nullptr_t) {
      foo = 0;
      sync_live_objects();
    }

    /// Allow constructing different instances for the tests.
    explicit Foo(int foo) : foo(foo) {
      sync_live_objects();
    }

    /// Maintain the count of live objects.
    Foo(const Foo& other) : Foo(other.foo) {
      sync_live_objects();
    }

    /// Maintain the count of live objects.
    Foo(Foo&& other) : foo(other.foo) {
      other.foo = -1;
      sync_live_objects();
      other.sync_live_objects();
    }

    /// Maintain the count of live objects.
    Foo& operator=(const Foo& other) {
      foo = other.foo;
      sync_live_objects();
      return *this;
    }

    /// Maintain the count of live objects.
    Foo& operator=(Foo&& other) {
      foo = other.foo;
      other.foo = -1;
      sync_live_objects();
      other.sync_live_objects();
      return *this;
    }

    /// Maintain the count of live objects.
    ///
    /// TRICKY: Also change the value of the `foo` member so any dangling
    /// pointers will see the wrong value. We _hope_ such access won't fault -
    /// it will be flagged by memory corruption tools.
    virtual ~Foo() {
      foo = -1;
      sync_live_objects();
    }
  };

  cpl::set<const Foo*> Foo::live_objects;

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
#ifdef CPL_FAST // {
    GIVEN("the fast variant is compiled") {
      THEN("CPL_VARIANT will be defined to \"fast\"") {
        REQUIRE(cpl::string(CPL_VARIANT) == "fast");
      }
    }
#endif          // } CPL_FAST
#ifdef CPL_SAFE // {
    GIVEN("the safe variant is compiled") {
      THEN("CPL_VARIANT will be defined to \"safe\"") {
        REQUIRE(cpl::string(CPL_VARIANT) == "safe");
      }
    }
#endif // } CPL_SAFE
  }

/// Verify that a reference is valid.
#define VERIFY_VALID_REF(REF) \
  REQUIRE(REF->foo == foo);   \
  REQUIRE((*REF).foo == foo)

/// Verify that a pointer is valid.
#define VERIFY_VALID_PTR(PTR) \
  VERIFY_VALID_REF(PTR);      \
  REQUIRE(!!PTR)

/// Verify that a reference is invalid and detected by CPL.
#define VERIFY_INVALID_REF(REF) \
  REQUIRE_CPL_THROWS(REF->foo); \
  REQUIRE_CPL_THROWS(*REF)

/// Verify that a pointer is invalid and detected by CPL.
#define VERIFY_INVALID_PTR(PTR) \
  VERIFY_INVALID_REF(PTR);      \
  REQUIRE(!PTR)

/// verify that an indirection does not compare equal to null.
#define VERIFY_NOT_NULL(IND)       \
  REQUIRE(IND != nullptr);         \
  REQUIRE_FALSE(IND == nullptr);   \
  if (!(IND > nullptr)) {          \
    REQUIRE_FALSE(IND >= nullptr); \
  }                                \
  if (!(IND < nullptr)) {          \
    REQUIRE_FALSE(IND <= nullptr); \
  }

/// verify that an indirection does compare equal to null.
#define VERIFY_NULL(IND)         \
  REQUIRE_FALSE(IND != nullptr); \
  REQUIRE(IND == nullptr);       \
  REQUIRE(IND >= nullptr);       \
  REQUIRE(IND <= nullptr);

#ifdef DOXYGEN // {

/// Verify that the data referred to has been deleted.
#define VERIFY_EXPIRED_REF(REF)

/// Verify that the data pointed to has been deleted.
#define VERIFY_EXPIRED_PTR(PTR)

#endif // } DOXYGEN

#ifdef CPL_SAFE // {

#define VERIFY_EXPIRED_REF(REF) VERIFY_INVALID_REF(REF)
#define VERIFY_EXPIRED_PTR(PTR) VERIFY_INVALID_PTR(PTR)

#endif // } CPL_SAFE

#ifdef CPL_FAST // {

#ifdef AVOID_INVALID_MEMORY_ACCESS // {

#define VERIFY_EXPIRED_REF(REF)
#define VERIFY_EXPIRED_PTR(PTR) REQUIRE(!!PTR)

#else // } AVOID_INVALID_MEMORY_ACCESS {

#define VERIFY_EXPIRED_REF(REF) \
  REQUIRE(REF->foo != foo);     \
  REQUIRE((*REF).foo != foo)

#define VERIFY_EXPIRED_PTR(PTR) \
  REQUIRE(PTR->foo != foo);     \
  REQUIRE((*PTR).foo != foo);   \
  REQUIRE(!!PTR)

#endif // } AVOID_INVALID_MEMORY_ACCESS

#endif // } CPL_SAFE

/// Used for simple copy parameters.
#define COPY(X) X

/// Use for moving parameters.
#define MOVE(X) std::move(X)

/// Verify that a reference type copy construction behaves as expected.
#define VERIFY_VALID_COPIED_REF_CONSTRUCTION(REF)               \
  THEN("we can copy it") {                                      \
    VERIFY_VALID_REF_CONSTRUCTION(REF, COPY, VERIFY_VALID_REF); \
  }

/// Verify that a reference type move construction behaves as expected.
#define VERIFY_VALID_MOVED_REF_CONSTRUCTION(REF)                  \
  THEN("we can move-and-clear it") {                              \
    VERIFY_VALID_REF_CONSTRUCTION(REF, MOVE, VERIFY_INVALID_REF); \
  }

/// Verify that a reference type move construction behaves as expected.
#define VERIFY_VALID_UNMOVED_REF_CONSTRUCTION(REF)              \
  THEN("we can move-and-keep it") {                             \
    VERIFY_VALID_REF_CONSTRUCTION(REF, MOVE, VERIFY_VALID_REF); \
  }

/// Verify that a reference type construction behaves as expected.
#define VERIFY_VALID_REF_CONSTRUCTION(REF, PASS, VERIFY_PASSED) \
  THEN("to construct another reference") {                      \
    cpl::REF<Bar> bar_ref_copy{ PASS(bar_ref) };                \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(bar_ref_copy);                             \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to assign to another reference") {                      \
    cpl::REF<Bar> bar_ref_copy = PASS(bar_ref);                 \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(bar_ref_copy);                             \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to construct a reference to const") {                   \
    cpl::REF<const Bar> const_bar_ref_copy{ PASS(bar_ref) };    \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(const_bar_ref_copy);                       \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to assign a reference to const") {                      \
    cpl::REF<const Bar> const_bar_ref_copy = PASS(bar_ref);     \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(const_bar_ref_copy);                       \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to construct a reference to a base class") {            \
    cpl::REF<Foo> foo_ref{ PASS(bar_ref) };                     \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(foo_ref);                                  \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to assign to a reference to a base class") {            \
    cpl::REF<Foo> foo_ref = PASS(bar_ref);                      \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(foo_ref);                                  \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to construct a reference to a const base class") {      \
    cpl::REF<const Foo> const_foo_ref{ PASS(bar_ref) };         \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(const_foo_ref);                            \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("to assign to a reference to a const base class") {      \
    cpl::REF<const Foo> const_foo_ref = PASS(bar_ref);          \
    VERIFY_PASSED(bar_ref);                                     \
    VERIFY_VALID_REF(const_foo_ref);                            \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }

/// Verify a reference detects being created from a null pointer.
#define VERIFY_NULL_REF_CONSTRUCTION(REF, PTR, PASS)              \
  GIVEN("a null pointer") {                                       \
    cpl::PTR<Foo> null_foo_ptr;                                   \
    REQUIRE(Foo::live_objects.size() == 0);                       \
    VERIFY_NULL(null_foo_ptr);                                    \
    VERIFY_INVALID_PTR(null_foo_ptr);                             \
    THEN("using to construct a reference will be " CPL_VARIANT) { \
      REQUIRE_CPL_THROWS(cpl::REF<Foo>{ PASS(null_foo_ptr) });    \
    }                                                             \
  }

/// Verify that a pointer type copy construction behaves as expected.
#define VERIFY_VALID_COPIED_PTR_CONSTRUCTION(PTR)               \
  THEN("we can copy it") {                                      \
    VERIFY_VALID_PTR_CONSTRUCTION(PTR, COPY, VERIFY_VALID_PTR); \
  }

/// Verify that a pointer type move construction behaves as expected.
#define VERIFY_VALID_MOVED_PTR_CONSTRUCTION(PTR)                  \
  THEN("we can move-and-clear it") {                              \
    VERIFY_VALID_PTR_CONSTRUCTION(PTR, MOVE, VERIFY_INVALID_PTR); \
  }

/// Verify that a pointer type move construction behaves as expected.
#define VERIFY_VALID_UNMOVED_PTR_CONSTRUCTION(PTR)              \
  THEN("we can move-and-keep it") {                             \
    VERIFY_VALID_PTR_CONSTRUCTION(PTR, MOVE, VERIFY_VALID_PTR); \
  }

/// Verify that a pointer type construction behaves as expected.
#define VERIFY_VALID_PTR_CONSTRUCTION(PTR, PASS, VERIFY_PASSED)                 \
  THEN("to construct another pointer") {                                        \
    cpl::PTR<Bar> bar_ptr_copy{ PASS(bar_ptr) };                                \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(bar_ptr_copy);                                             \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to assign to another pointer") {                                        \
    cpl::PTR<Bar> bar_ptr_copy;                                                 \
    bar_ptr_copy = PASS(bar_ptr);                                               \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(bar_ptr_copy);                                             \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to construct to a pointer to const") {                                  \
    cpl::PTR<const Bar> const_bar_ptr_copy{ PASS(bar_ptr) };                    \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(const_bar_ptr_copy);                                       \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to assign to a pointer to const") {                                     \
    cpl::PTR<const Bar> const_bar_ptr_copy;                                     \
    const_bar_ptr_copy = PASS(bar_ptr);                                         \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(const_bar_ptr_copy);                                       \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to construct a pointer to a base class") {                              \
    cpl::PTR<Foo> foo_ptr{ PASS(bar_ptr) };                                     \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(foo_ptr);                                                  \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to assign to a pointer to a base class") {                              \
    cpl::PTR<Foo> foo_ptr;                                                      \
    foo_ptr = PASS(bar_ptr);                                                    \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(foo_ptr);                                                  \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to construct a pointer to a const base class") {                        \
    cpl::PTR<const Foo> const_foo_ptr{ PASS(bar_ptr) };                         \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(const_foo_ptr);                                            \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("to assign to a pointer to a const base class") {                        \
    cpl::PTR<const Foo> const_foo_ptr;                                          \
    const_foo_ptr = PASS(bar_ptr);                                              \
    VERIFY_PASSED(bar_ptr);                                                     \
    VERIFY_VALID_PTR(const_foo_ptr);                                            \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("we can assign a nullptr to it") {                                       \
    bar_ptr = nullptr;                                                          \
    VERIFY_INVALID_PTR(bar_ptr);                                                \
  }                                                                             \
  THEN("asking it for a ref-or will return the original data") {                \
    int foo_alternate = __LINE__;                                               \
    int bar_alternate = __LINE__;                                               \
    Bar raw_bar_alternate{ foo, bar };                                          \
    cpl::ref<Bar> bar_ref_alternate{ cpl::unsafe_ref<Bar>(raw_bar_alternate) }; \
    REQUIRE(Foo::live_objects.size() == 2);                                     \
    cpl::ref<Bar> bar_ref = bar_ptr.ref_or(bar_ref_alternate);                  \
    REQUIRE(Foo::live_objects.size() == 2);                                     \
    VERIFY_VALID_PTR(bar_ptr);                                                  \
    VERIFY_VALID_REF(bar_ref);                                                  \
  }

/// Verify a pointer provides null construction.
#define VERIFY_NULL_PTR_CONSTRUCTION(PTR)         \
  THEN("we have a default constructor") {         \
    cpl::PTR<Foo> default_ptr;                    \
    VERIFY_INVALID_PTR(default_ptr);              \
    REQUIRE(Foo::live_objects.size() == 0);       \
  }                                               \
  THEN("we can explicitly construct a nullptr") { \
    cpl::PTR<Foo> null_ptr{ nullptr };            \
    VERIFY_INVALID_PTR(null_ptr);                 \
    REQUIRE(Foo::live_objects.size() == 0);       \
  }

/// Ensure attempts for invalid reference construction will fail.
#define VERIFY_INVALID_REF_CONSTRUCTION(REF, PTR, MAKE_REF, MAKE_REF_CONST, PASS)                           \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, REF) = cpl::PTR<T>(), "implicit conversion of pointer to a reference"); \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(), "default reference construction");                                   \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(nullptr), "explicit null reference construction");                      \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>((T*)nullptr), "unsafe reference construction");                         \
  MUST_NOT_COMPILE(int, cpl::REF<Foo>(MAKE_REF), "unrelated reference construction");                       \
  MUST_NOT_COMPILE(int, CAT(s_foo_, REF) = (MAKE_REF), "unrelated reference assignment");                   \
  MUST_NOT_COMPILE(Foo, cpl::REF<Bar>(MAKE_REF), "sub-class reference construction");                       \
  MUST_NOT_COMPILE(Foo, CAT(s_bar_, REF) = (MAKE_REF), "sub-class reference assignment");                   \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(MAKE_REF_CONST), "const violation reference construction");             \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, REF) = (MAKE_REF_CONST), "const violation reference assignment");       \
  MUST_NOT_COMPILE(Foo, cpl::REF<T>(PASS(CAT(s_foo_const_, REF))), "const violation reference construction")

/// Ensure attempts for invalid pointer construction will fail.
#define VERIFY_INVALID_PTR_CONSTRUCTION(PTR, PASS)                                                         \
  MUST_NOT_COMPILE(Foo, cpl::PTR<T>{(T*)nullptr }, "unsafe pointer construction");                         \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, PTR) = (T*)nullptr, "unsafe pointer assignment");                      \
  MUST_NOT_COMPILE(int, cpl::PTR<Foo>{ PASS(cpl::PTR<T>()) }, "unrelated pointer construction");           \
  MUST_NOT_COMPILE(int, CAT(s_foo_, PTR) = PASS(cpl::PTR<T>()), "unrelated pointer assignment");           \
  MUST_NOT_COMPILE(Foo, cpl::PTR<Bar>{ PASS(cpl::PTR<T>()) }, "sub-class pointer construction");           \
  MUST_NOT_COMPILE(Foo, CAT(s_bar_, PTR) = PASS(cpl::PTR<T>()), "sub-class pointer construction");         \
  MUST_NOT_COMPILE(Foo, cpl::PTR<T>{ PASS(cpl::PTR<const T>()) }, "const violation pointer construction"); \
  MUST_NOT_COMPILE(Foo, CAT(s_foo_, PTR) = PASS(cpl::PTR<const T>()), "const violation pointer construction")

/// Ensure casting a reference works as expected.
#define VERIFY_REF_CASTS(REF, PASS)                                               \
  THEN("if we copy it to a reference to a base class") {                          \
    cpl::REF<Foo> foo_ref{ PASS(bar_ref) };                                       \
    REQUIRE(Foo::live_objects.size() == 1);                                       \
    THEN("we can clever_cast it back to a reference to the derived class") {      \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_clever<Bar>(PASS(foo_ref));          \
      REQUIRE(Foo::live_objects.size() == 1);                                     \
    }                                                                             \
    THEN("we can static_cast it back to a reference to the derived class") {      \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_static<Bar>(PASS(foo_ref));          \
      REQUIRE(Foo::live_objects.size() == 1);                                     \
    }                                                                             \
    THEN("we can dynamic_cast it back to a reference to the derived class") {     \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(PASS(foo_ref));         \
      REQUIRE(Foo::live_objects.size() == 1);                                     \
    }                                                                             \
    THEN("we can reinterpret_cast it back to a reference to the derived class") { \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_dynamic<Bar>(PASS(foo_ref));         \
      REQUIRE(Foo::live_objects.size() == 1);                                     \
    }                                                                             \
  }                                                                               \
  THEN("we can copy it to a reference to a const") {                              \
    cpl::REF<const Bar> const_bar_ref{ PASS(bar_ref) };                           \
    REQUIRE(Foo::live_objects.size() == 1);                                       \
    THEN("we can const_cast it back to a reference mutable") {                    \
      cpl::REF<Bar> cast_bar_ref = cpl::cast_const<Bar>(PASS(const_bar_ref));     \
      REQUIRE(Foo::live_objects.size() == 1);                                     \
    }                                                                             \
  }                                                                               \
  THEN("we can cast it to a reference") {                                         \
    Bar& ref = bar_ref;                                                           \
    const Bar& const_ref = bar_ref;                                               \
  }

/// Ensure casting a pointer works as expected.
#define VERIFY_PTR_CASTS(PTR, PASS)                                             \
  THEN("if we copy it to a pointer to a base class") {                          \
    cpl::PTR<Foo> foo_ptr{ PASS(bar_ptr) };                                     \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
    THEN("we can clever_cast it back to a pointer to the derived class") {      \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_clever<Bar>(PASS(foo_ptr));        \
      REQUIRE(Foo::live_objects.size() == 1);                                   \
    }                                                                           \
    THEN("we can static_cast it back to a pointer to the derived class") {      \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_static<Bar>(PASS(foo_ptr));        \
      REQUIRE(Foo::live_objects.size() == 1);                                   \
    }                                                                           \
    THEN("we can dynamic_cast it back to a pointer to the derived class") {     \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(PASS(foo_ptr));       \
      REQUIRE(Foo::live_objects.size() == 1);                                   \
    }                                                                           \
    THEN("we can reinterpret_cast it back to a pointer to the derived class") { \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_dynamic<Bar>(PASS(foo_ptr));       \
      REQUIRE(Foo::live_objects.size() == 1);                                   \
    }                                                                           \
  }                                                                             \
  THEN("we can copy it to a pointer to a const") {                              \
    cpl::PTR<const Bar> const_bar_ptr{ PASS(bar_ptr) };                         \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
    THEN("we can const_cast it back to a pointer mutable") {                    \
      cpl::PTR<Bar> cast_bar_ptr = cpl::cast_const<Bar>(PASS(const_bar_ptr));   \
      REQUIRE(Foo::live_objects.size() == 1);                                   \
    }                                                                           \
  }

/// Ensure converting a reference to a pointer works as expected.
#define VERIFY_CONVERTING_REF_TO_PTR(REF, PTR, PASS)            \
  THEN("we can copy it to a pointer") {                         \
    cpl::PTR<Bar> bar_ptr{ PASS(bar_ref) };                     \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can assign it to a pointer") {                       \
    cpl::PTR<Bar> bar_ptr;                                      \
    bar_ptr = PASS(bar_ref);                                    \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can copy it to a pointer to const") {                \
    cpl::ptr<const Bar> const_bar_ptr{ PASS(bar_ref) };         \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can assign it to a pointer to const") {              \
    cpl::ptr<const Bar> const_bar_ptr;                          \
    const_bar_ptr = PASS(bar_ref);                              \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can copy it to a pointer to the base class") {       \
    cpl::ptr<Foo> foo_ptr{ PASS(bar_ref) };                     \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can assign it to a pointer to the base class") {     \
    cpl::ptr<Foo> foo_ptr;                                      \
    foo_ptr = PASS(bar_ref);                                    \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can copy it to a pointer to a const base class") {   \
    cpl::ptr<const Foo> const_foo_ptr{ PASS(bar_ref) };         \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }                                                             \
  THEN("we can assign it to a pointer to a const base class") { \
    cpl::ptr<const Foo> const_foo_ptr;                          \
    const_foo_ptr = PASS(bar_ref);                              \
    REQUIRE(Foo::live_objects.size() == 1);                     \
  }

/// Ensure converting a pointer to a reference works as expected.
#define VERIFY_CONVERTING_PTR_TO_REF(REF, PTR, PASS)                            \
  THEN("we can copy it to a reference") {                                       \
    cpl::REF<Bar> bar_ref{ PASS(bar_ptr) };                                     \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("we can copy it to a reference to const") {                              \
    cpl::REF<const Bar> const_bar_ref{ PASS(bar_ptr) };                         \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("we can copy it to a reference to the base class") {                     \
    cpl::REF<Foo> foo_ref{ PASS(bar_ptr) };                                     \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("we can copy it to a reference to a const base class") {                 \
    cpl::ref<const Foo> const_foo_ref{ PASS(bar_ptr) };                         \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
  }                                                                             \
  THEN("we can ask it for a reference") {                                       \
    cpl::ref<Bar> bar_ref = bar_ptr.ref();                                      \
    REQUIRE(Foo::live_objects.size() == 1);                                     \
    VERIFY_VALID_PTR(bar_ptr);                                                  \
    VERIFY_VALID_REF(bar_ref);                                                  \
  }                                                                             \
  THEN("asking it for a ref-or will return the original data") {                \
    int foo_alternate = __LINE__;                                               \
    int bar_alternate = __LINE__;                                               \
    Bar raw_bar_alternate{ foo, bar };                                          \
    cpl::ref<Bar> bar_ref_alternate{ cpl::unsafe_ref<Bar>(raw_bar_alternate) }; \
    REQUIRE(Foo::live_objects.size() == 2);                                     \
    cpl::ref<Bar> bar_ref = bar_ptr.ref_or(bar_ref_alternate);                  \
    REQUIRE(Foo::live_objects.size() == 2);                                     \
    VERIFY_VALID_PTR(bar_ptr);                                                  \
    VERIFY_VALID_REF(bar_ref);                                                  \
  }

  TEST_CASE("swapping an optional value") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("an optional value") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_opt = cpl::opt<Bar>{ cpl::in_place, foo, bar };
      cpl::ptr<Bar> bar_ptr = bar_opt;
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_VALID_PTR(bar_opt);
      THEN("we can swap it with an empty optional value") {
        cpl::opt<Bar> empty_opt;
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_INVALID_PTR(empty_opt);
        empty_opt.swap(bar_opt);
        VERIFY_VALID_PTR(empty_opt);
        VERIFY_INVALID_PTR(bar_opt);
        VERIFY_EXPIRED_PTR(bar_ptr);
      }
      THEN("we can swap an empty optional value with it") {
        cpl::opt<Bar> empty_opt;
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_INVALID_PTR(empty_opt);
        bar_opt.swap(empty_opt);
        VERIFY_VALID_PTR(empty_opt);
        VERIFY_INVALID_PTR(bar_opt);
        VERIFY_EXPIRED_PTR(bar_ptr);
      }
      THEN("we can swap it with a second optional value") {
        int foo_second = __LINE__;
        int bar_second = __LINE__;
        auto bar_opt_second = cpl::opt<Bar>{ cpl::in_place, foo_second, bar_second };
        cpl::ptr<Bar> bar_ptr_second = bar_opt_second;
        REQUIRE(Foo::live_objects.size() == 2);
        bar_opt_second.swap(bar_opt);
        VERIFY_VALID_PTR(bar_opt_second);
        VERIFY_VALID_PTR(bar_ptr_second);
      }
      THEN("we can swap a second optional value with it") {
        int foo_second = __LINE__;
        int bar_second = __LINE__;
        auto bar_opt_second = cpl::opt<Bar>{ cpl::in_place, foo_second, bar_second };
        cpl::ptr<Bar> bar_ptr_second = bar_opt_second;
        REQUIRE(Foo::live_objects.size() == 2);
        bar_opt.swap(bar_opt_second);
        VERIFY_VALID_PTR(bar_opt_second);
        VERIFY_VALID_PTR(bar_ptr_second);
      }
    }
    GIVEN("an empty optional value") {
      cpl::opt<Bar> bar_opt;
      REQUIRE(Foo::live_objects.size() == 0);
      VERIFY_INVALID_PTR(bar_opt);
      THEN("we can emplace a value into it") {
        int foo = __LINE__;
        int bar = __LINE__;
        bar_opt.emplace(foo, bar);
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_VALID_PTR(bar_opt);
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("constructing an sref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we make shared data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_ref = cpl::make_sref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_NOT_NULL(bar_ref);
      VERIFY_VALID_COPIED_REF_CONSTRUCTION(sref);
      VERIFY_VALID_MOVED_REF_CONSTRUCTION(sref);
    }
    VERIFY_NULL_REF_CONSTRUCTION(sref, sptr, COPY);
    REQUIRE(Foo::live_objects.size() == 0);
  }

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static cpl::sref<Foo> s_foo_sref = cpl::make_sref<Foo>();

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static cpl::sref<Bar> s_bar_sref{ cpl::make_sref<Bar>() };

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static cpl::sref<const Foo> s_foo_const_sref{ cpl::make_sref<const Foo>() };

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static const cpl::sref<Foo> s_const_foo_sref{ cpl::make_sref<Foo>() };

  /// Verify `cpl::sref` is protected against invalid construction.
  VERIFY_INVALID_REF_CONSTRUCTION(sref, sptr, cpl::make_sref<T>(0), cpl::make_sref<const T>(0), COPY);

  TEST_CASE("casting an sref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("an sref") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_ref = cpl::make_sref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_REF_CASTS(sref, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("constructing an sptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we make shared data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_ptr{ cpl::make_sptr<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_NOT_NULL(bar_ptr);
      VERIFY_VALID_COPIED_PTR_CONSTRUCTION(sptr);
      VERIFY_VALID_MOVED_PTR_CONSTRUCTION(sptr);
      THEN("we can ask it for a shared reference") {
        cpl::sref<Bar> bar_ref = bar_ptr.sref();
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_VALID_PTR(bar_ptr);
        VERIFY_VALID_REF(bar_ref);
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::sptr<Foo> s_foo_sptr;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::sptr<Bar> s_bar_sptr;

  /// Verify `cpl::sptr` is protected against invalid construction.
  VERIFY_INVALID_PTR_CONSTRUCTION(sptr, COPY);

  TEST_CASE("casting an sptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a shared pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_ptr = cpl::make_sptr<Bar>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_PTR_CASTS(sptr, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("converting an sref to an sptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a shared reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sref<Bar> bar_ref{ cpl::make_sref<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_CONVERTING_REF_TO_PTR(sref, sptr, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("converting an sptr to an sref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a shared pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::sptr<Bar> bar_ptr{ cpl::make_sptr<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_CONVERTING_PTR_TO_REF(sref, sptr, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("constructing a uref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we make unique data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_ref = cpl::make_uref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_NOT_NULL(bar_ref);
      VERIFY_VALID_MOVED_REF_CONSTRUCTION(uref);
    }
    VERIFY_NULL_REF_CONSTRUCTION(uref, uptr, MOVE);
    REQUIRE(Foo::live_objects.size() == 0);
  }

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static cpl::uref<Foo> s_foo_uref = cpl::make_uref<Foo>();

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static cpl::uref<Bar> s_bar_uref{ cpl::make_uref<Bar>() };

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static cpl::uref<const Foo> s_foo_const_uref{ cpl::make_uref<const Foo>() };

  /// Used for @ref VERIFY_INVALID_REF_CONSTRUCTION.
  static const cpl::uref<Foo> s_const_foo_uref{ cpl::make_uref<Foo>() };

  /// Verify `cpl::uref` is protected against invalid construction.
  VERIFY_INVALID_REF_CONSTRUCTION(uref, uptr, cpl::make_uref<T>(0), cpl::make_uref<const T>(0), MOVE);

#ifdef MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS // {
  /// Ensure there's no copying `cpl::uref`.
  MUST_NOT_COMPILE(Foo, cpl::uref<T>{ s_foo_uref }, "copying unique references");
#endif // } MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS

  TEST_CASE("casting a uref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a uref") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_ref = cpl::make_uref<Bar>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_NOT_NULL(bar_ref);
      VERIFY_REF_CASTS(uref, MOVE);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("constructing a uptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we make unique data") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_ptr{ cpl::make_uptr<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_VALID_MOVED_PTR_CONSTRUCTION(uptr);
      THEN("we can ask it for a unique reference") {
        cpl::uref<Bar> bar_ref = bar_ptr.uref();
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_INVALID_PTR(bar_ptr);
        VERIFY_VALID_REF(bar_ref);
      }
    }
    VERIFY_NULL_PTR_CONSTRUCTION(uptr);
    REQUIRE(Foo::live_objects.size() == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::uptr<Foo> s_foo_uptr;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::uptr<Bar> s_bar_uptr;

  /// Verify `cpl::uptr` is protected against invalid construction.
  VERIFY_INVALID_PTR_CONSTRUCTION(uptr, MOVE);

#ifdef MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS // {
  /// Ensure there's no copying `cpl::uptr`.
  MUST_NOT_COMPILE(cpl::uptr<Foo>{ s_foo_uptr }, "copying unique pointers");
#endif // } MAKE_SFINAE_RESPOND_TO_DELETED_FUNCTIONS

  TEST_CASE("casting a uptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a unique pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_ptr = cpl::make_uptr<Bar>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_PTR_CASTS(uptr, MOVE);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("converting a uref to a uptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a unique reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uref<Bar> bar_ref{ cpl::make_uref<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_CONVERTING_REF_TO_PTR(uref, uptr, MOVE);
      THEN("swapping it into a null pointer will be " CPL_VARIANT) {
        cpl::uptr<Bar> bar_ptr;
        REQUIRE_CPL_THROWS(bar_ptr.swap(bar_ref));
      }
      THEN("swapping a null pointer it into it will be " CPL_VARIANT) {
        cpl::uptr<Bar> bar_ptr;
        REQUIRE_CPL_THROWS(bar_ref.swap(bar_ptr));
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("converting a uptr to a uref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a unique pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      cpl::uptr<Bar> bar_ptr{ cpl::make_uptr<Bar>(foo, bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_CONVERTING_PTR_TO_REF(uref, uptr, MOVE);
      THEN("we can swap it with a null pointer") {
        cpl::uptr<Bar> null_ptr;
        null_ptr.swap(bar_ptr);
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_VALID_PTR(null_ptr);
        VERIFY_INVALID_PTR(bar_ptr);
      }
      THEN("we can swap a null pointer with it") {
        cpl::uptr<Bar> null_ptr;
        bar_ptr.swap(null_ptr);
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_VALID_PTR(null_ptr);
        VERIFY_INVALID_PTR(bar_ptr);
      }
      THEN("we can swap it with a reference") {
        int old_foo = foo;
        int old_bar = bar;
        int foo = __LINE__;
        int bar = __LINE__;
        cpl::uref<Bar> bar_ref{ cpl::make_uref<Bar>(foo, bar) };
        cpl::ptr<Bar> bar_ptr_borrow = bar_ref;
        VERIFY_VALID_REF(bar_ref);
        VERIFY_VALID_PTR(bar_ptr_borrow);
        REQUIRE(Foo::live_objects.size() == 2);
        bar_ref.swap(bar_ptr);
        REQUIRE(Foo::live_objects.size() == 2);
        VERIFY_VALID_PTR(bar_ptr);
        VERIFY_VALID_PTR(bar_ptr_borrow);
        foo = old_foo;
        bar = old_bar;
        VERIFY_VALID_REF(bar_ref);
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("constructing a ref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      REQUIRE(bar_ref == &raw_bar);
      REQUIRE_FALSE(bar_ref != &raw_bar);
      REQUIRE(&raw_bar == bar_ref);
      REQUIRE_FALSE(&raw_bar != bar_ref);
      REQUIRE(bar_ref >= &raw_bar);
      REQUIRE_FALSE(bar_ref > &raw_bar);
      REQUIRE(bar_ref <= &raw_bar);
      REQUIRE_FALSE(bar_ref < &raw_bar);
      VERIFY_VALID_COPIED_REF_CONSTRUCTION(ref);
      VERIFY_VALID_UNMOVED_REF_CONSTRUCTION(ref);
    }
    VERIFY_NULL_REF_CONSTRUCTION(ref, ptr, COPY);
    REQUIRE(Foo::live_objects.size() == 0);
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
  VERIFY_INVALID_REF_CONSTRUCTION(
    ref, ptr, cpl::ref<T>(cpl::unsafe_ref<T>(*(T*) nullptr)), cpl::ref<const T>(cpl::unsafe_ref<T>(*(T*) nullptr)), COPY);

  TEST_CASE("casting a ref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a borrowed reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_REF_CASTS(ref, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("constructing a ptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("raw data") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr(raw_bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      REQUIRE(bar_ptr == &raw_bar);
      REQUIRE_FALSE(bar_ptr != &raw_bar);
      REQUIRE(&raw_bar == bar_ptr);
      REQUIRE_FALSE(&raw_bar != bar_ptr);
      REQUIRE(bar_ptr >= &raw_bar);
      REQUIRE_FALSE(bar_ptr > &raw_bar);
      REQUIRE(bar_ptr <= &raw_bar);
      REQUIRE_FALSE(bar_ptr < &raw_bar);
      VERIFY_VALID_COPIED_PTR_CONSTRUCTION(ptr);
      VERIFY_VALID_UNMOVED_PTR_CONSTRUCTION(ptr);
    }
    VERIFY_NULL_PTR_CONSTRUCTION(ptr);
    REQUIRE(Foo::live_objects.size() == 0);
  }

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ptr<Foo> s_foo_ptr;

  /// Used for @ref MUST_NOT_COMPILE.
  static cpl::ptr<Bar> s_bar_ptr;

  /// Verify `cpl::ptr` is protected against invalid construction.
  VERIFY_INVALID_PTR_CONSTRUCTION(ptr, COPY);

  TEST_CASE("casting a ptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a borrowed pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_PTR_CASTS(ptr, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("converting a ref to a ptr") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a borrowed reference to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ref<Bar> bar_ref{ cpl::unsafe_ref<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_CONVERTING_REF_TO_PTR(ref, ptr, COPY);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("converting a ptr to a ref") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("a borrowed pointer to a derived class") {
      int foo = __LINE__;
      int bar = __LINE__;
      Bar raw_bar{ foo, bar };
      cpl::ptr<Bar> bar_ptr{ cpl::unsafe_ptr<Bar>(raw_bar) };
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_CONVERTING_PTR_TO_REF(ref, ptr, COPY);
      THEN("we can compare it to a reference") {
        cpl::ref<Bar> bar_ref = bar_ptr.ref();
        REQUIRE(Foo::live_objects.size() == 1);
        REQUIRE(bar_ptr == bar_ref);
        REQUIRE_FALSE(bar_ptr != bar_ref);
        REQUIRE(bar_ref == bar_ptr);
        REQUIRE_FALSE(bar_ref != bar_ptr);
        REQUIRE(bar_ptr >= bar_ref);
        REQUIRE_FALSE(bar_ptr > bar_ref);
        REQUIRE(bar_ptr <= bar_ref);
        REQUIRE_FALSE(bar_ptr < bar_ref);
        VERIFY_VALID_PTR(bar_ptr);
        VERIFY_VALID_REF(bar_ref);
      }
    }
    GIVEN("a null pointer") {
      cpl::ptr<Bar> bar_ptr;
      REQUIRE(Foo::live_objects.size() == 0);
      THEN("asking it for a reference will be " CPL_VARIANT) {
        REQUIRE_CPL_THROWS(bar_ptr.ref());
      }
      THEN("asking it for a ref-or will return the alternate data") {
        int foo = __LINE__;
        int bar = __LINE__;
        Bar raw_bar_alternate{ foo, bar };
        cpl::ref<Bar> bar_ref_alternate{ cpl::unsafe_ref<Bar>(raw_bar_alternate) };
        REQUIRE(Foo::live_objects.size() == 1);
        cpl::ref<Bar> bar_ref = bar_ptr.ref_or(bar_ref_alternate);
        REQUIRE(Foo::live_objects.size() == 1);
        VERIFY_VALID_REF(bar_ref);
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

/// Use in @ref VERIFY_BORROWING when the value has an implicit cast to a @ref
/// cpl::ref.
#define IMPLICIT_ASSIGN_TO_REF(TYPE, VALUE) VALUE

/// Use in @ref VERIFY_BORROWING when the value has an explicit cast to a @ref
/// cpl::ref.
#define EXPLICIT_ASSIGN_TO_REF(TYPE, VALUE) cpl::ref<TYPE>(VALUE)

/// Verify a borrowed reference reacts to the data being deleted.
#define VERIFY_EXPIRED_BORROWED_REF()                                               \
  THEN("the borrowed pointer will be " CPL_VARIANT " if the original is deleted") { \
    bar_something.reset();                                                          \
    VERIFY_EXPIRED_REF(bar_ref);                                                    \
  }

/// Verify a borrowed pointer reacts to the data being deleted.
#define VERIFY_EXPIRED_BORROWED_PTR()                                               \
  THEN("the borrowed pointer will be " CPL_VARIANT " if the original is deleted") { \
    bar_something.reset();                                                          \
    VERIFY_EXPIRED_PTR(bar_ptr);                                                    \
  }

/// Enable testing reset of the original value.
#define WITH_RESET(X) X

/// Disable testing reset of the original value.
#define WITHOUT_RESET(X)

/// Verify a borrowed reference reacts to the data being deleted.
#define VERIFY_RESET_BORROWED_REF(RESET)                                                \
  RESET(THEN("the borrowed pointer will be " CPL_VARIANT " if the original is reset") { \
    bar_something->reset();                                                             \
    VERIFY_EXPIRED_REF(bar_ref);                                                        \
  })

/// Verify a borrowed pointer reacts to the data being deleted.
#define VERIFY_RESET_BORROWED_PTR(RESET)                                                \
  RESET(THEN("the borrowed pointer will be " CPL_VARIANT " if the original is reset") { \
    bar_something->reset();                                                             \
    VERIFY_EXPIRED_PTR(bar_ptr);                                                        \
  })

/// Verify we cab borrow something.
#define VERIFY_BORROWING(ACCESS_VALUE, ASSIGN_TO_REF, RESET)                  \
  THEN("we can copy it to a borrowed pointer") {                              \
    cpl::ptr<Bar> bar_ptr{ *bar_something };                                  \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed pointer") {                            \
    cpl::ptr<Bar> bar_ptr;                                                    \
    bar_ptr = *bar_something;                                                 \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed pointer to const") {                     \
    cpl::ptr<const Bar> bar_ptr{ *bar_something };                            \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed pointer to const") {                   \
    cpl::ptr<const Bar> bar_ptr;                                              \
    bar_ptr = *bar_something;                                                 \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed pointer to a base class") {              \
    cpl::ptr<Foo> bar_ptr{ *bar_something };                                  \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed pointer to a base class") {            \
    cpl::ptr<Foo> bar_ptr;                                                    \
    bar_ptr = *bar_something;                                                 \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed pointer to a base class to const") {     \
    cpl::ptr<const Foo> bar_ptr{ *bar_something };                            \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed pointer to a base class to const") {   \
    cpl::ptr<const Foo> bar_ptr;                                              \
    bar_ptr = *bar_something;                                                 \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_PTR();                                            \
    VERIFY_RESET_BORROWED_PTR(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed reference") {                            \
    cpl::ref<Bar> bar_ref{ *bar_something };                                  \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed reference") {                          \
    cpl::ref<Bar> bar_ref = cpl::unsafe_ref<Bar>(ACCESS_VALUE);               \
    bar_ref = ASSIGN_TO_REF(Bar, *bar_something);                             \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed reference to const") {                   \
    cpl::ref<const Bar> bar_ref{ *bar_something };                            \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed reference to const") {                 \
    cpl::ref<const Bar> bar_ref = cpl::unsafe_ref<Bar>(ACCESS_VALUE);         \
    bar_ref = ASSIGN_TO_REF(Bar, *bar_something);                             \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed reference to a base class") {            \
    cpl::ref<Foo> bar_ref{ *bar_something };                                  \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed reference to a base class") {          \
    cpl::ref<Foo> bar_ref = cpl::unsafe_ref<Foo>(ACCESS_VALUE);               \
    bar_ref = ASSIGN_TO_REF(Foo, *bar_something);                             \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can copy it to a borrowed reference to a base class to const") {   \
    cpl::ref<const Foo> bar_ref{ *bar_something };                            \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }                                                                           \
  THEN("we can assign it to a borrowed reference to a base class to const") { \
    cpl::ref<const Foo> bar_ref = cpl::unsafe_ref<Foo>(ACCESS_VALUE);         \
    bar_ref = ASSIGN_TO_REF(Foo, *bar_something);                             \
    REQUIRE(Foo::live_objects.size() == 1);                                   \
    VERIFY_EXPIRED_BORROWED_REF();                                            \
    VERIFY_RESET_BORROWED_REF(RESET);                                         \
  }

  TEST_CASE("borrowing a held value") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we have a held reference") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_something = std::make_unique<cpl::is<Bar>>(foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_BORROWING(*bar_something, IMPLICIT_ASSIGN_TO_REF, WITHOUT_RESET);
    }
    GIVEN("a const held value") {
      int foo = __LINE__;
      int bar = __LINE__;
      const cpl::is<Bar> const_is_bar{ foo, bar };
      REQUIRE(Foo::live_objects.size() == 1);
      THEN("we can assign it to a reference to a const") {
        int foo_second = __LINE__;
        int bar_second = __LINE__;
        auto bar_is_second = cpl::is<Bar>{ foo_second, bar_second };
        cpl::ref<const Bar> ref_const_bar = bar_is_second;
        ref_const_bar = const_is_bar;
        VERIFY_VALID_REF(ref_const_bar);
      }
      THEN("we can assign it to a pointer to a const") {
        cpl::ptr<const Bar> ptr_const_bar;
        ptr_const_bar = const_is_bar;
        VERIFY_VALID_PTR(ptr_const_bar);
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("borrowing an optional value") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we have an optional value") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_something = std::make_unique<cpl::opt<Bar>>(cpl::in_place, foo, bar);
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_BORROWING(bar_something->value(), EXPLICIT_ASSIGN_TO_REF, WITH_RESET);
    }
    GIVEN("a const optional value") {
      int foo = __LINE__;
      int bar = __LINE__;
      const cpl::opt<Bar> const_opt_bar{ cpl::in_place, foo, bar };
      REQUIRE(Foo::live_objects.size() == 1);
      THEN("we can copy it to a reference to a const") {
        cpl::ref<const Bar> ref_const_bar{ const_opt_bar };
        ref_const_bar = const_opt_bar;
        VERIFY_VALID_REF(ref_const_bar);
      }
      THEN("we can assign it to a pointer to a const") {
        cpl::ptr<const Bar> ref_const_bar;
        ref_const_bar = const_opt_bar;
        VERIFY_VALID_PTR(ref_const_bar);
      }
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("borrowing a shared indirection") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we have a shared reference") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_something = std::make_unique<cpl::sref<Bar>>(cpl::make_sref<Bar>(foo, bar));
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_BORROWING(**bar_something, IMPLICIT_ASSIGN_TO_REF, WITHOUT_RESET);
    }
    GIVEN("we have a non-null shared pointer") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_something = std::make_unique<cpl::sptr<Bar>>(cpl::make_sptr<Bar>(foo, bar));
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_BORROWING(**bar_something, EXPLICIT_ASSIGN_TO_REF, WITH_RESET);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }

  TEST_CASE("borrowing a unique indirection") {
    REQUIRE(Foo::live_objects.size() == 0);
    GIVEN("we have a unique reference") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_something = std::make_unique<cpl::uref<Bar>>(cpl::make_uref<Bar>(foo, bar));
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_BORROWING(**bar_something, IMPLICIT_ASSIGN_TO_REF, WITHOUT_RESET);
    }
    GIVEN("we have a non-null unique pointer") {
      int foo = __LINE__;
      int bar = __LINE__;
      auto bar_something = std::make_unique<cpl::uptr<Bar>>(cpl::make_uptr<Bar>(foo, bar));
      REQUIRE(Foo::live_objects.size() == 1);
      VERIFY_BORROWING(**bar_something, EXPLICIT_ASSIGN_TO_REF, WITH_RESET);
    }
    REQUIRE(Foo::live_objects.size() == 0);
  }
}
