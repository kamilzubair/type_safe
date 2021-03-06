# type_safe

[![Build Status](https://travis-ci.org/foonathan/type_safe.svg?branch=master)](https://travis-ci.org/foonathan/type_safe)
[![Build status](https://ci.appveyor.com/api/projects/status/aw1j2h2s52g4laen/branch/master?svg=true)](https://ci.appveyor.com/project/foonathan/type-safe/branch/master)

type_safe provides zero overhead abstractions that use the C++ type system to prevent bugs.

> Zero overhead abstractions here and in following mean abstractions that have no cost with optimizations enabled,
> but may lead to slightly lower runtime in debug mode,
> especially when assertions for this library are enabled.

The library features cannot really explained in the scope of this readme,
I highly suggest that you check out [the first](https://foonathan.github.io/blog/2016/10/11/type-safe.html) and [second blog post](https://foonathan.github.io/blog/2016/10/19/strong-typedefs.html) and the examples.

## Features

* `ts::integer<T>` - a zero overhead wrapper over a built-in integer type
    * no default constructor to force meaningful initialization
    * no "lossy" conversions (i.e. from a bigger type or a type with a different signedness)
    * no mixed arithmetic/comparision with floating points or integer types of a different signedness
    * over/underflow is undefined behavior in release mode - even for `unsigned` integers,
      enabling compiler optimizations
* `ts::floating_point<T>` - a zero overhead wrapper over a built-in floating point
    * no default constructor to force meaningful initialization
    * no "lossy"  conversion (i.e. from a bigger type)
    * no "lossy" comparisions
    * no mixed arithmetic/comparision with integers
* `ts::boolean` - a zero overhead wrapper over `bool`
    * no default constructor to force meaningful initialization
    * no conversion from integer values
    * no arithmetic operators
* `ts::flag` - an improved flag type, better than a regular `bool` or `ts::boolean`
* `ts::narrow_cast<T>` - to actually do narrow conversions
* aliases of `std::` integer/floating point types that either use the wrapper or the built-in types,
  depending on a macro
* `ts::basic_optional<StoragePolicy>` - a generic, improved `std::optional` that is fully monadic,
  also `ts::optional<T>` and `ts::optional_ref<T>` aliases
* `ts::constrained_type<T, Constraint, Verifier>` - a wrapper over some type that verifies that a certain constraint is always fulfilled
    * `ts::constraints::*` - predefined constraints like `non_null`, `non_empty`, ...
    * `ts::tagged_type<T, Constraint>` - constrained type without checking, useful for tagging
    * `ts::bounded_type<T>` - constrained type that ensures a value in a certain interval
    * `ts::clamped_type<T>` - constrained type that clamps a value to ensure that it is in the certain interval
* `ts::strong_typedef` - a generic facility to create strong typedefs more easily
* `ts::deferred_construction<T>` - create an object without initializing it yet
* `ts::output_parameter<T>` - an improved output parameter compared to the naive lvalue reference

## Installation

Header-only, just copy the files in your project.
You need to add `include/type_safe` to your include path as well as make [debug_assert.hpp](https://github.com/foonathan/debug_assert) available.
The repository is included as `git submodule`, simply run `git submodule update --init` and add `external/debug_assert` to the include path.
You also need to enable C++11.

Behavior can be customized with the following macros:

* `TYPE_SAFE_ENABLE_ASSERTIONS` (default is `1`): whether or not assertions are enabled in this library
* `TYPE_SAFE_ENABLE_WRAPPER` (default is `1`): whether or not the typedefs in `type_safe/types.hpp` use the wrapper classes

If you're using CMake there is the target `type_safe` available after you've called `add_subdirectory(path/to/type_safe)`.
Simply link this target to your target and it will setup everything automagically.
For convenience the macros are also mapped to CMake options of the same name.

## Documentation

Currently only inline comments available, not extracted.
I need to fix [standardese](https://github.com/foonathan/standardese) first but got sidelined writing a type safe library...
    
