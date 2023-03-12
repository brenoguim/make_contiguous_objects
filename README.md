# make_contiguous_objects
Implementation for std::make_contiguous_objects proposal


# Introduction

This proposal intends to add standard library support to create objects adjacent in memory.
For example, if you want to create a `char` next to an `int`, next to a `long`, you can do:
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
auto s = std::make_contiguous_objects<char, int, long>(numC, numI, numL);
```

# Motivation and Scope

Data structures that optimize for performance often use "contiguous objects" as a way to avoid multiple allocations and improve the memory locality.

#### std::shared_ptr<T[]>
One of the simplest examples is `std::make_shared<T[]>` which creates a `std::shared_ptr<T[]>` where the control block adjacent to the `T` array.
This simple operation involves a large amount of non-trivial code to:
1. Calculate the exact amount of memory to allocate to hold the control block and `T`s
2. Find the correctly-aligned memory addresses where the control block and  `T`s will live
3. Construct them while being aware that should exception be thrown, all objects must be destroyed in the reverse order of construction.

If a developer attempts to write such code for the first time, it will be either a multi-day task or many details will be overlooked.
Generally, even experienced developers will avoid going into this realm due to the complexity of writing and maintaining this code.
For more details, see the [LLVM implementation](https://github.com/llvm/llvm-project/blob/2f887c9a760dfdffa584ce84361912fe122ad79f/libcxx/include/__memory/shared_ptr.h#L1139)


