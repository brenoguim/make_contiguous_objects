# make_contiguous_objects
Implementation for std::make_contiguous_objects proposal


Introduction

This proposal intends to add standard library support to create objects adjacent in memory.
For example, if you want to create an `char` next to an `int`, next to a `long`, you can do:
```
struct Struct { char c; int i; long l; }
auto* s = new Struct;
```

However, if you want to allocate an arbitrary number of `char`s next to several `int`s, next to multiple `long`s, there is no easy way to do so.
In practice, developers will resort to splitting up the layout:

```
struct Struct { std::vector<char> cs; std::vector<int> is; std::vector<long> ls; }
auto* s = new Struct{numC, numI, numL};
```

With this proposal, it would be possible to write:
```
auto* s = std::make_contiguous_objects<char, int, long>(numC, numI, numL);
```

Motivation and Scope


