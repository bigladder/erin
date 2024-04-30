# Style Guide

TODO: make consistent with Athenium

## Numbers

When writing a double floating point number always show the decimal and the zero:

```
double good = 10.0;

double bad = 10.;
```

Rationale: The single decimal without the zero can sometimes be easy to miss.


## Casing and Capitalization

Variables should be in snake case `like_this`.

Functions should be in snake case `like_this`.

An exception is for method-like functions.
A method-like function is a normal C++ function.
However, it is closely associated with its' first parameter.
The first parameter is usually (but not always) a struct.
For a method-like function, the casing would be `TypeName_method_name`.
A collection of method-like functions is a light-weight alternative to yielding some of the organizational benefits of a class without the overhead.
They are especially useful during early development and for library-internals.

User-defined types (structs, classes, enums, etc.) should be in Pascal case `LikeThis`.

## Function and Method Signatures

We always place the return type on the line above the function and method signatures:

```cpp
ReturnType
foo_bar_baz(int a, int b, double c);
```

Any non-trivial type should be passed into a function as `<type> const&` UNLESS it is to be modified (in which case, drop `const`).
A non-trivial type is a type other than basic numeric types like char, int, long, float, and double.

## Attributes on Variables

Please use the following conventions:

- `long double var0 = 0.0;`
- `unsigned int var1 = 0;`

However, for `const` qualification, that should go afterwards:
- `std::vector<unsigned int> const& vec_of_uint`

Reasoning: `unsigned` (in particular) and `long` are part of the type.
However, `const` is a qualifier and we don't want to obfusticate what the type is.

