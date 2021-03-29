# C++ and C

## Reserved Identifiers

[Stack Overflow](https://stackoverflow.com/questions/228783/what-are-the-rules-about-using-an-underscore-in-a-c-identifier/228797#228797)

- At global scope all `_[lower-case-letters]`
- Always reserved are `__` and `_[capital-letter]`
- C++: Anything in the `std` namespace but you are allowed to make template specializations.
- Cannot use adjacent underscores "__" anywhere in the identifier.

## C++ Move Semantics

- The only valid operations that can be performed on a moved objects are being destructed, copy assigned to, or move assigned to.

## Optimizations C++ has over C

- `std::sort` > `qsort`
  - The comparator can be inlined since `std::sort` is a templated function.
- `std::copy` > `memcpy`
  - Has the potential to be faster since the compiler has more information on the types because it's templated also the call to `std::copy` may be inlined while the CRT lib has to come from a DLL.
  - Technically `std::copy` is more like `memmove` since overlapping ranges is allowed.

## Optimizations C has over C++

- pthreads / Win32 threads vs `std::thread`
  - `std::thread` jas a more expensive startup / creation expense because it allows passing of parameters (namely more than a single `void*`) to the threaded functions.

## C++ Standard Library ("STL")

- As of C++17 `std::vector`, `std::list`, `std::forward_list` can be template instantiated with an incomplete type:
- Algorithms:
  - `std::copy`          can handle overlapping ranges if you are copying to the left  (<==)
  - `std::copy_backward` can handle overlapping ranges if you are copying to the right (==>)

## Memory

- (C++) The free store is the term for the memory where dynamic object allocations live. (new / delete) 
- (C) The heap is the term for the memory where dynamic memory allocations live. (malloc / free) 

## TODO: C++ Design Patterns

- Policy Based Design: https://en.wikipedia.org/wiki/Modern_C%2B%2B_Design#Policy-based_design
  - Works well with a 'compressed_pair' for EBO
