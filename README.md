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

#### Containers

Containers often need to store metadata adjacent to a group of elements:
[Abseil flat_hash_set](https://github.com/abseil/abseil-cpp/blob/d8933b836b1e1aac982b1dd42cc6ac1343a878d5/absl/container/internal/raw_hash_set.h#L1342).
[std::hive](https://github.com/mattreecebentley/plf_hive/blob/8c2bf6d9606df1d76900751ffffc472e994b529b/plf_hive.h#L174)

#### Others

1. [LLVM User class](https://github.com/llvm/llvm-project/blob/1597e5e6932b944c2c382a138e76b757da56b200/llvm/include/llvm/IR/User.h#L63)
2. [C Flexible Array Members](https://en.wikipedia.org/wiki/Flexible_array_member)
3. `new T[]` for non trivially destructible types, in some compilers, also store the size of the array together with the data.

# Proposed facilities

TBD: Document what APIs are necessary to simplify the examples mentioned above

# Demonstration

TBD: Apply the facilities above in the motivating examples to show that it can simplify the code

