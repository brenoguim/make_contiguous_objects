# make_contiguous_objects
Implementation for std::make_contiguous_objects proposal


# Introduction

This proposal intends to add standard library support to allocate adjacent objects of different types.

If you want to create a `char` next to an `int`, next to a `long`, you can do:
```
struct Struct { char c; int i; long l; };
auto* s = new Struct;
```

However, if you want to allocate an arbitrary number of `char`s next to several `int`s, next to multiple `long`s, there is no easy way to do so.
In practice, developers will resort to splitting up the layout:

```
struct Struct { std::vector<char> cs; std::vector<int> is; std::vector<long> ls; };
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

Containers often need to store metadata adjacent to a group of elements.

- [Abseil flat_hash_set](https://github.com/abseil/abseil-cpp/blob/d8933b836b1e1aac982b1dd42cc6ac1343a878d5/absl/container/internal/raw_hash_set.h#L1342)
- [std::hive](https://github.com/mattreecebentley/plf_hive/blob/8c2bf6d9606df1d76900751ffffc472e994b529b/plf_hive.h#L174)

#### Others

- [LLVM User class](https://github.com/llvm/llvm-project/blob/1597e5e6932b944c2c382a138e76b757da56b200/llvm/include/llvm/IR/User.h#L63)
- [C Flexible Array Members](https://en.wikipedia.org/wiki/Flexible_array_member)
- `new T[]` for non trivially destructible types, in some compilers, also store the size of the array together with the data.

# Proposed facilities

```
template<class... Args, class... Initializers>
auto make_contiguous_objects(Initializers... args) -> std::tuple<std::span<Args>...>
```
Where `Initializers` can be:

1. `size_t`: The number of elements in the array of that type
2. `std::arg(std::uninit_t, size_t count)`: Number of elements to be left uninitialized if trivially destructible.
3. `std::arg(std::ctor_t, size_t count, Args...)`: Number of elements and initialization parameters.
4. `std::arg(std::aggregate_t, size_t count, Args...)`: Number of elements and initialization parameters for aggregate init `{}`.
5. `std::arg(std::input_iterator_t, size_t count, InputIterator)`: Number of elements and an input iterator to provide values for the array
6. `std::arg(std::functor_t, size_t, Functor)`: Number of elements and a functor to provide values for the array

[See an example of each being used](https://github.com/brenoguim/make_contiguous_objects/blob/6bbd8ca8f6f4fb5e5c21fb3d1b5442d1dd2a8978/tests/unit/basic.test.cpp#L91)

The return type is a tuple of `std::span<T>` pointing to each array.

TBD
- Document `destroy_contiguous_objects`
- Decide and document a facility to get one array from another (discussed in Shortcomings)
- Allocator support

# Shortcomings

### Unsolved problems
#### Implicit memory layout
In most use-cases, the return value (`tuple<span<Args>...>`) won't be stored as it contains redundant information.
For example, in a case where the arrays have the same size:
```
auto t = std::make_contiguous_objects<Metadata, Element>(numElements, numElements);
```

One can generally just store the pointer to the first `Metadata` object and the number of elements.
From that alone it's possible to find the end of the `Metadata` array:
```
auto mdEnd = mdBegin + numElements;
```

And the begining of the `Element` array:
```
auto elBegin =  nextAligned<Element*>(mdEnd);
```
For this reason, it's important to also provide a functionality similar to `nextAligned` that takes a point from an object, and finds the next aligned position.

But even with that, one of the largest shortcomings is that the layout is not explicit. It needs to be described through comments or by code interpretation.
Moreover, there is no way to assign names to the fields.

#### Delayed object initialization

Some applications might not want to initialize all objects immediately. For example, containers want to reserve memory for a chunk of objects, but only initialize such objects when they are inserted.

For this, the developer would still need an aligned storage type and use that with `make_contiguous_objects`. Moreover, the developer would have to manually destroy.
```
struct MetaData { unsigned numElements = 0; };
auto l  = std::make_contiguous_objects<Metadata, ElementStorage>(1, 16);

// During insert
auto& numElements = std::get<0>(l)[0].numElements;
auto* nextElement = &std::get<1>(l)[numElements];
new (nextElement) Element;
numElements++;

// During destruction
// Create a layout that uses "Element" instead of "ElementStorage" and std::destroy_contiguous_objects will destroy them and the original buffer.
std::tuple<span<Metadata>, span<Element>> lToDestroy {std::get<0>(l), {begin(), end()}};
std::destroy_contiguous_objects(lToDestroy);
```

TBD: Add a test to show this with better names and code organization.


# Demonstration

TBD: Apply the facilities above in the motivating examples to show that it can simplify the code.

So far, just an [initial implementation](https://github.com/brenoguim/make_contiguous_objects/blob/main/tests/unit/shared_array.test.cpp) of shared_ptr<T[]>.

# Bikeshed

- `std::make_adjacent_objects`
- `std::make_objects`
- `std::make_adjacent`

