# Style Guide

TODO: make consistent with Athenium

## Casing and Capitalization

Variables should be in snake case `like_this`. 

Functions should be in camel case `likeThis`.
An exception is for method-like functions.
A method-like function is a normal C++ function.
However, it is closely associated with its' first parameter.
For a method-like function, the casing would be `TypeName_methodName`.

User-defined types (structs, classes, enums, etc.) should be in Pascal case `LikeThis`.

## Function and Method Signatures

We always place the return type on the line above the function and method signatures:

```cpp
ReturnType
fooBarBaz(int a, int b, double c);
```



