# make_contiguous_objects
Implementation for std::make_contiguous_objects proposal


# Introduction

This proposal intends to add support to allocate adjacent objects of different types.

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
Or, if performance is a requirement, developers will write by hand the code to allocate a blob of memory and place each object in the desired place.

With this proposal, it would be possible to write:
```
auto s = std::make_contiguous_objects<char, int, long>(numC, numI, numL);
```

# Motivation and Scope

Data structures that optimize for performance often use "contiguous objects" as a way to avoid multiple allocations and improve the memory locality.

#### std::shared_ptr<T[]>
One of the simplest examples is `std::make_shared<T[]>` which creates a `std::shared_ptr<T[]>` where the control block is adjacent to the `T` array.
This simple operation involves a large amount of non-trivial code to:
1. Calculate the exact amount of memory to allocate to hold the control block and `T`s
2. Find the correctly-aligned memory addresses where the control block and  `T`s will live
3. Construct them while being aware that should exception be thrown, all objects must be destroyed in the reverse order of construction.

If a developer attempts to write such code for the first time, it will be either a multi-day task or many details will be overlooked.
Generally, even experienced developers will avoid going into this realm due to the complexity of writing and maintaining this code.

For more details, see the [libc++ implementation](https://github.com/llvm/llvm-project/blob/2f887c9a760dfdffa584ce84361912fe122ad79f/libcxx/include/__memory/shared_ptr.h#L1139)

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
- Decide and document a facility to get one array from another (discussed in Limitations of the solution)
- Allocator support

# Applying the proposed API in real code

[Simplification of libc++ machinery for std::shared_ptr<T[]>](https://github.com/llvm/llvm-project/compare/main...brenoguim:llvm-project:breno.mco?diff=split#diff-19c001df6058f7f3e4c8d1cd2856da344c1bfc52a06b8c144540b0d4cc99ff1d)


[Simple shared ptr initial implementation](https://github.com/brenoguim/make_contiguous_objects/blob/main/tests/unit/shared_array.test.cpp)

# Discussion on common feedback

# Why not RAII?

This API returns a tuple of spans and the caller is responsible for calling `std::destroy_contiguous_objects` passing that same tuple to destroy all objects and release the storage.
It's obvious there is an opportunity for returning an object that manages the lifetime of the whole structure.

The current paper does not propose that because:
1. It can easily be built on top of the raw API
2. The author does not expect users of the API to store the whole result very frequently.

In most use-cases, the return value (`tuple<span<Args>...>`) will contain redundant information. For example, in a case where the arrays have the same size:
```
auto t = std::make_contiguous_objects<Metadata, Element>(numElements, numElements);
```
Users will generally store only the first pointer and derive the other arrays from that and `numElements`.
See the application of this library on libc++ `shared_ptr<T[]>` in the section "Applying the proposed API in real code".

# Require U for single objects, U[] for dynamic arrays and U[N] for static arrays to match std::shared_ptr

The main reason for this distinction (as far as I can tell) in shared_ptr API is the lack of a type dispatching mechanism to choose which constructor to use.
So `make_shared<T>` allows passing multiple arguments that will be used for the construction of `T` inplace.
`make_shared<T[]>` allows either passing nothing (`new T()`) or a single argument (`new T(arg)`).
And finally `make_shared_for_overwrite<T[]>` gives the `new T` constructor, that leaves trivial types uninitialized.

This proposal intends to use the `std::arg` mechanism with type dispatching to choose any of those options, and others like input iterator constructor.

# std::arg might be too "out there"

It's something we don't have in any other function in the standard and can be a very "open" API. Might bring a lot of controversy.
Other options are:
1. Chaining 1:
```
auto r = std::make_contiguous_objects<string, int, float>()
    .arg(std::ctor, 15, a, b,c)
    .arg(std::input_iterator, 10, it)
    .arg(std::uninit, 12);
```
2. Chaining 2:
```
auto r = std::make_contiguous_objects()
    .arg<string>(std::ctor, 15, a, b,c)
    .arg<int>(std::input_iterator, 10, it)
    .arg<float>(std::uninit, 12);
```
3. Limiting to always call the default constructor: `std::make_contiguous_objects<int, float>(10, 20);`

Either 1 or 2 should be viable options, while 3 could be too limited.


# Bikeshed

- `std::make_objects`
- `std::new_objects`
- `std::allocate_objects`
- All of the above with `adjacent_objects` instead of `objects`
- All of the above with `arrays` instead of `objects`



