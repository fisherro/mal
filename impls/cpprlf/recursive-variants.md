# Recursive variants

There are two issues with recursive variants.

## Infinite size

Firstly, needing to know the size.

    using object = std::variant<int, std::pair<object, object>>;

The size of `object` depends on the size of `std::pair<object, object>`, which
depends on the size of `object`... It's an infinite loop.

## Can't forward declar a variant

    using object = std::variant<int, struct foo*, object*>;

Including a pointer to an incomplete type in a variant is fine. The size of
the type itself isn't needed to determine the size of the variant. Only the
size of a pointer.

But the type still needs to be declared. For `foo`, qualifying it with `struct`
serves as a foward declaration. But there's no way to forward declare
`object` itself.

This problem also affects types that might be parametarized on object.

    using object = std::variant<
        int, std::unique_ptr<object>, std::vector<object>>

Although the sizes of `unique_ptr<object>` and `vector<object>` don't depend on the size of `object`, `object` is still not declared at this point, so it can't be used as a template parameter.

## Solution 1

One option is to use wrapper structs/classes.

    using object = std::variant<
        int,
        std::pair<struct foo, struct foo>,
        std::vector<struct bar>>;
    
    struct foo { std::unique_ptr<object> my_object; };
    struct bar { object my_object; };

In a case like `pair` where the size depends on the size of the objects
contained, the wrapper (`foo`) has to dynamically allocate the object. Note that something
like the proposed `std::polymorphic_value` (P0201) or `std::indirect_value` (P1950) would be
a better option than `std::unique_ptr` in order to (mostly) transparently
preserve value semantics.

P3019 proposed both std::polymorphic and std::indirect. This made it into C++26.

In a case like `vector`, the contained objects will already be dynamically
allocated, so the wrapper (`bar`) can contain object directly.

The downside is that the wrapper makes accessing the objects more tedious.

    std::get<std::vector<bar>>(an_object).my_object.at(0);

## Solution 2

Another option is type erasure, and `std::any` is one way to use it.

    using object = std::variant<
        int,
        std::pair<std::any, std::any>,
        std::vector<std::any>>;

The problem here is that everytime you want to access the objects hidden
behind a `std::any`, you have to use `any_cast`.

    std::any_cast<object>(std::get<std::vector<std::any>>(an_object).at(0))

...which is even more tedious.

## References

Foonathan has a good blog post on the topic with an implementation of a wrapper type:

https://www.foonathan.net/2022/05/recursive-variant-box/#content

\[TODO: Add links to the papers mentioned above.]

`Std::polymorphic_value` supports polymorphism while `std::indirect_value` does not. But `std::indirect_value` supports a custom deleter and custom copier, which `std::polymorphic_value` does not.

A stack overflow answer offers a `value_ptr` wrapper class.

https://stackoverflow.com/questions/39454347/using-stdvariant-with-recursion-without-using-boostrecursive-wrapper

## \[TODO: Integrate this better] Boost recursive wrapper

The `boost::recursive_wrapper` class dynamically allocates its contents.
`Boost::variant` has special support for it.

`Std::variant` has no such support.

`Boost::recursive_wrapper` might work with `std::variant`? Although if you're
using boost, might as well use `boost::variant`, I suppose.

