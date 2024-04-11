# Style Guide

TODO: make consistent with Athenium

## Casing and Capitalization

Variables should be in snake case `like_this`. 

Functions should be in camel case `like_this`.

An exception is for method-like functions.
A method-like function is a normal C++ function.
However, it is closely associated with its' first parameter.
The first parameter is usually (but not always) a struct.
For a method-like function, the casing would be `TypeName_method_name`.
A collection of method-like functions is a light-weight alternative to a C++ class.
They are especially useful during early development and for library-internals.

User-defined types (structs, classes, enums, etc.) should be in Pascal case `LikeThis`.

## Function and Method Signatures

We always place the return type on the line above the function and method signatures:

```cpp
ReturnType
foo_bar_baz(int a, int b, double c);
```



