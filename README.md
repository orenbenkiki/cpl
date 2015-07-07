CPL {#mainpage}
===

<em>"Clever people solve problems wise people avoid".</em>

The Clever Protection Library (CPL) tries to provide additional safety for C++
projects. For details, see the description of the
<code>[cpl](http://orenbenkiki.github.io/cpl/namespacecpl.html#details)</code>
namespace in the Doxygen-generated
<code>[documentation](http://orenbenkiki.github.io/cpl)</code>.

## Using

To use CPL, do the following:

- Include `cpl.hpp` in your C++ project.

- Use the @ref cpl types instead of the `std` types.

- Add one of `-DCPL_FAST` or `-DCPL_SAFE` to your compilation flags.
  - The `CPL_FAST` variant will compile to code that has no run-time
    assertions, providing maximal performance.
  - The `CPL_SAFE` variant will compile to code that has many run-time
    assertions, providing maximal safety.

By convention, a `.fast` or `.safe` suffix is attached to the name of generated
libraries and/or binaries to clarify which variant is used.

## Caveats

Ideally the `cpl` types would be 100% compatible, drop-in replacements to the
`std` types. In practice this is impossible due to the need to protect built-in
standard types like `T*` and `T&`. Therefore migration to using CPL requires
manually modifying the code.

The fast variant is very unsafe. The safe variant is very slow. Achieving
simultaneous safety and speed is extremely difficult, and probably impossible
in C++. CPL is an engineering compromise. Look into
[Rust](http://www.rust-lang.org/) for a new language trying to provide
simultaneous safety and speed.

The fast variant is as fast as possible. The safe variant is as safe as
possible. Obviously it is also possible to define intermediate points on this
spectrum, but currently CPL only provides the two extremes.

The implementation is naive in many ways. Writing an STL-level library in
modern C++ is a daunting task: issues such as `constexpr`, `noexcept` and
perfect forwarding are fiendishly difficult to get right. Even simple
`const`-ness is tricky due to the existence of both `const opt<T>` and
`opt<const T>`.

The unit tests are a great help in ensuring everything behaves as it should,
however they aren't 100% complete so some edge cases might be wrong. It is also
difficult to verify the library forbids all sort of invalid code (even though
@ref MUST_NOT_COMPILE helps a bit). In general CPL is relaxed about this
because the safe version will detect any foul play at run-time.

As always, YMMV, no warranty is implied, may contain nuts, etc.

## Building and Installing

CPL is provided as a single header file so there's no building or installation
required - just point the include path of the C++ compiler to the CPL
directory. However, CPL does come with a `Makefile` providing the following
goals:

- `make all` - the default goal updates the source files, compiles and runs the
  tests, and generates the html documentation.

- `make src` - updates the source files to embed a version string derived from
  running `git`, and to enforce a format using `clang-format`. This will not
  modify the source files timestamp if no changes are required.

- `make test` - compiles and runs the tests based on the updated sources.

- `make html` - generates HTML documentation using Doxygen based on the updated
  sources.

- `make clean` - removes all the generated files.

## Dependencies

There are no dependencies for using CPL, other than, of course, the standard
C++ libraries. It was developed using `g++` version 4.9.2 using C++14
constructs, and using `clang-format` for formatting the source code. CPL does
not currently compile under `clang`, though porting it would make sense.

For building and running its unit tests, CPL uses the delightful
[Catch](https://github.com/philsquared/Catch) unit tests framework for its
tests. Catch is also available as a simple set of header files, the location of
which is a configuration parameter in the `Makefile`.

## License

CPL is provided under the [MIT license](http://opensource.org/licenses/MIT).
