# C++ and C

## Reserved Identifiers

[Stack Overflow](https://stackoverflow.com/questions/228783/what-are-the-rules-about-using-an-underscore-in-a-c-identifier/228797#228797)

- At global scope all `_[lower-case-letters]`
- Always reserved are `__` and `_[capital-letter]`
- C++: Anything in the `std` namespace but you are allowed to make template specializations.
- Cannot use adjacent underscores "__" anywhere in the identifier.

## Move Semantics

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

## Memory

- (C++) The free store is the term for the memory where dynamic object allocations live. (new / delete) 
- (C) The heap is the term for the memory where dynamic memory allocations live. (malloc / free) 

## TODO: C++ Design Patterns

- Policy Based Design: https://en.wikipedia.org/wiki/Modern_C%2B%2B_Design#Policy-based_design
  - Works well with a 'compressed_pair' for EBO


---

# Mathematics

## Lerp / Smoothing

### Framerate Independence

[Reference](https://gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php)

A moving lerp can be used to smoothly get to a target value using `value = lerp(value, target_value, /* coefficient: usually a low number like 0.05 - 0.2 */)`.

*This is NOT framerate independent unless you use a fixed time step which isn't always desireable.*

To fix this an exponential function involving time can be used instead:
`value = lerp(target_value, value, exp[base](-rate * delta_time))`

- A `rate` of 1.0 means `value` will reach 1.0 / `base` of `target_value` in one second.
- Halving and doubling `rate` will change how fast it moves as expected.

To convert your old `coefficient` to the new rate:

`rate = -target_fps * log[base](1.0 - coefficient)`

To make a more slider friendly number to edit just have a `log_rate` variable that is edited and to get the `rate` do `exp[base](log_rate)`.

### Logarithmic Lerping

Some values are better moving on a logarithmically namely "scale / zoom".

```cpp

zoom = lerp(zoom, target_zoom, delta_time / duration);

=>

zoom = exp[base](lerp(log(zoom), log[base](target_zoom), delta_time / duration);
```

## Linear Algebra

### Quaternion

```
A quaternion represents an expression: 
q = w + i*x + j*y + k*z

i^2 = j^2 = k^2 = i*j*k = -1

A `pure` quaternion is one in which the w component is 0.0f. 



p_1 = q * p_o * q^*

Notation: q^* = q's conjugate
```

### Matrix

```
R = A^T
=> transpose(inverse(R)) =inverse(A) = transpose(inverse(transpose(A)))
```

#### Normal Mapping

> The normal of an object must be transformed by the transposed inverse of the model to world matrix.

```glsl
mat4 NormalMatrix      = transpose(inverse(ModelMatrix));
vec4 TransformedNormal = NormalMatrix * ObjectNormal;
```
> The TBN matrix can be calculated in this way:

```glsl
vertex-inputs:
  vec3 in_Tangent;
  vec3 in_Normal;

// Gram–Schmidt Orthonormalization

vec3 T   = normalize(mat3(NormalMatrix) * in_Tangent);
vec3 N   = normalize(mat3(NormalMatrix) * in_Normal);
T        = normalize(T - dot(T, N) * N);
vec3 B   = cross(N, T);
mat3 TBN = transpose(mat3(T, B, N));
```

#### Decomposition

```glsl
// Row Major Matrix Representation.

mat4 M = [a, b, c, d]
         [e, f, g, h] 
         [i, j, k, l] 
         [0, 0, 0, 1]

vec3 Translation = vec3{d h l};
vec3 Scale       = [
  length([a, e, i]),
  length([b, f, j]),
  length([c, g, k]),
];
mat4 Rotation = [a / Scale.x, b / Scale.y, c / Scale.z, 0]
                [e / Scale.x, f / Scale.y, g / Scale.z, 0]
                [i / Scale.x, j / Scale.y, k / Scale.z, 0]
                [      0,           0,           0,     1]
```


---

# Graphics

## Vulkan

### Misc Good Things To Know

- A buffer memory barrier is just a more specific version of a Memory barrier.
- The Image Layout indicates to the GPU how to compress the image.
  - If you need to change the layout often (~3+ times) then just use the GENERAL layout.
- Framebuffer Local : ONLY the same sample / pixel is used in multiple sub-passes rather than sampling.
- Semaphores are for syncing of queues.
- Renderpasses are synchronized with external subpass dependencies.
- Transitioning to a subpass with automatically transition an image to the correct layout for you.



---

# IPC (Inter-Process Communication)

## Shared Memory

> This may be the fastest solution to IPC.

> The two processes must be on the same PC.

- Win32: [https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory]
	- CreateFileMapping, MapViewOfFile, UnMapViewOfFile, CloseHandle.
- Posix: shmget, shmat, shmdt, shmctl API calls.

## UDP / TCP Sockets

> For both Win32 and Posix the API is basically the same.

> This is networking so works across computer.

- Win32: WinSock
- Posix: Berkely API

### Ports

- They are 16bit so the 'full' range is 0 - 65535
- Use Port 0 for the OS to choose.
- Port #s 0 - 1000 are reserved for services.
  - Ex: HTTP: 80, HTTPS: 443, SSH: 22
- Ports we can use (1,000 to 6,535)

## Named Pipes

> Pipes work on the same PC or across the network.

> There are also anon pipes, they are one way (parent -> child).

- Win32:
	- Pipe name format server: "\\.\pipe\PipeName", client: "\\ComputerName\pipe\PipeName" (remeber to correctly escape backslashes)
	- CreateNamedPipe, ConnectNamedPipe, etc..
- Posix: mkfifo, mknod, normal file IO functions.


## WM_COPY_DATA / SendMessage (Win32)

> Handle the 'WM_COPY_DATA' message in the WinProc function.

> Server calls 'SendMessage' with some data.

> Must two processes be on the same PC.

## Mailslot (Win32)

> Works only for really small data sizes.

> One way, client -> server.

> Works across network.

---

# UI

## Animation Speeds

- Animation speeds should span 200 - 500ms, sub 100ms animations feel instantaneous to the human brain.


| Very Fast        | Fast  | Normal              | Slow  | Very Slow |
|------------------|-------|---------------------|-------|-----------|
| 100ms            | 200ms | 300ms               | 400ms | 500ms     |

* 100ms - 200ms should be used for hover fx and fades.
* 300ms - 500ms for larger, more complex transitions.

- While phones should have effects in the 200 - 300ms range you can increase or decrease that amount by ~30% depending on the screen size and platform.
  - Users are accustomed to websites opening quickly
  - Tablets have larger screens so increase the time it takes.
  - Smart watches have such a small screen so the time should be very short. 

> Large objects should move slower than smaller ones.

- When displaying a list of items each item should only take 20-25ms to appear.
- List items should appear from top to bottom while a grid should start in the top-left and go to the bottom right (diagonal movement)

- When you have to animate a lot of items try to have a central object to have be the focus and have the rest of the items be subordinate to it.

- When moving an object that changes size as well then if the size change is disproportionate (square => rectangle) then the movement of the object should go along an arc according to _Material Design_.

- Objects at the same z-level should never intersect each other while animating.

## DPI (Dots Per Inch) & Scaling

- To correctly handle HiDPI displays you should handle it per window, per monitor.

> Default DPI Per Platform

- Windows : 96
-   macOS : 72

> Windows

Each window is allowed to have a 'DPI Awareness' level declaring how it handles DPI.
This can be set either though a manifest file or at runtime programmatically.

*If you set DPI awareness programmatically you must set it before any windows (`HWND`s) are created*

- `SetProcessDpiAwarenessContext` (Windows 10)
  - `SetProcessDpiAwareness` (Windows 8.1)
  - `SetProcessDpiAware` (Windows Vista)
- `GetDpiForWindow`

- DPI_AWARENESS_UNAWARE
  - The default value indicating Windows will lie to you and will scale up your app automatically.
  - This leads to blurry text and graphics since this is just a dumb bilinear scaling.
- DPI_AWARENESS_SYSTEM_AWARE
  - The window is correct for the main monitor but doesn't handle changing to a monitor with a different DPI.
- DPI_AWARENESS_PER_MONITOR_AWARE
  - The window is a rockstar and handles dpi scaling changes through the WM_DPICHANGE event.

> Apple

Can only be set through the app manifest.

On macOS set the `NSHighResolutionCapable` flag to `TRUE`.

---

# Compilers

> Terms / Concepts

Most are covered by the links below but I wanted to just give you as feel of what terms will be thrown at yah.

- Recursive Decent Parser: 
-- This is a technique of parsing string expressions ex: "3 * 6 / 6 - 4", make sure to handle operator precedence! Writing a simple calculator app should get you familiar with the concept (that's what I started with).

- Abstract Syntax Tree 
-- Data structure for representation of the string you are parsing.

- Bytecode: 
-- A custom set of instructions that will be run by your own virtual machine. (ex here Lua's set of bytecode instructions: http://luaforge.net/docman/83/98/ANoFrillsIntroToLua51VMInstructions.pdf)

> Websites

[http://www.craftinginterpreters.com/]
- Super great resource on being introduced to making an interpreter / compiler with really nice diagrams.

[https://github.com/wren-lang/wren]
- Good Reference of a Small Programming Language Implementation in C99. Made by the same guy that wrote "http://www.craftinginterpreters.com/"

[https://www.lua.org/doc/jucs05.pdf]
- This is an overview of how Lua was implemented from a runtime perspective if you also wanted to make a virtual machine for your language.

[http://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/]
- This topic is also covered in "http://www.craftinginterpreters.com/" but it is how to implement a basic garbage collector.

[https://ruslanspivak.com/lsbasi-part1/]
- Unfinished tutorial in Python on how to go about making an interpreter / compiler using a slightly different method. This method of using Abstract Syntax Tree (AST, researching this topic is a good idea) is more robust and allows for more flexibility in making an optimizing compiler. Using an AST is the _industry_ standard (gcc, clang/llvm, msvc) but not needed for simpler languages like Lua or Python.

[https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/]
- An alternative / addition to recursive decent parser with an elegant way of handling operator precedence. (Covered in Crafting Interpreters [http://www.craftinginterpreters.com/compiling-expressions.html])

> Books

"Language Implementation Patterns: Create Your Own Domain-Specific and General Programming Languages"
- A nice reference for seeing various techniques to handle specific compiler problems.

"Principles of Compiler Design." and
"Compilers: Principles, Techniques, and Tools"
- Considered the go to for compilers but pretty hard to read as it includes all these mathematical proofs on language design but it will teach pretty much all of compiler design.

---

# C++ Property System Sketch

```cpp
namespace bf
{
  //
  // Property Storage Policies
  //
  // * Inline / Owned - The data in held by the property itself.
  // * Virtual        - A pair of getter and setter functions.
  // * Reference      - Only holds a reference to a piece of data.
  // 

  //
  // Problem Space
  //
  // - Should provide a way to have automatically disconnecting 'Connections'
  // - While a slot is being signaled any other slots may be disconnected
  //   so safe iteration over slots is something to think about.
  // - Some libraries handle threading, I don't believe this is a real problem.
  //   - Well designed un-synchronized APIs with no global state can be externally synchronized.
  // - Lifetimes
  //   - What if the `property` outlives the `connection(s)`.  (Not a problem)
  //   - What if the `connection(s)` outlives the property(s). (Is a problem)
  //

  template<typename T>
  struct InlinePropertyStorage
  {
    T data;
  };

  template<typename T>
  struct RefPropertyStorage
  {
    T* data;
  };

  template<
    typename T, 
    typename TGetter = std::function<T ()>, 
    typename TSetter = std::function<void (T value)>
  >
  struct VirtualPropertyStorage
  {
    TGetter getter;
    TSetter setter;
  };

  template<typename T, template<typename> TStorage>
  class property
  {
    TStorage<T> storage;

  };
}
```

---

# True Misc

__debugbreak()

__builtin_trap()

Coefficients for rgb are 0.2, 0.7, 0.1 

# An Imported Model Has

- A List of Meshes
  - Each mesh has a list of bones
  - Each mesh has either one material or less.
  - List of vertices
- A List of Embedded Materials
  - Each Material has a list of properties and texture types.
  - Each texture type can have multiple textures.


# Assimp Data

- There may be more Nodes than Bones.
- Each mesh has an array of bones.
- Each mesh vertex needs some bone data (weight:float, bone_index:int)
- Each animation 'track' has which bone is affects + all the frame datas.
- A bone is associated with one aiNode, but an aiNode may not have a bone.
- Per Mesh Bone Data
  - Mat4x4 OffsetMatrix;
  - Node*  NodeToChange;
  - Per Vertex Data
    - Uint8 BoneIndex
    - Float BoneWeight

- Model Specific Data
  - Mat4x4       GlobalInverseTransform = scene->mRootNode->mTransformation.Inverse()
  - Node[]       Nodes;
  - BoneInput[]  BonesOffset

- Model Instance Data
  - BoneOutput[] BonesFinalTransformation

- Node
  - Mat4x4 Transform;
  - Int    FirstChild;
  - Int    NumChildren;
  - Int!   BoneIndex;

- Anim Specific Data
  - String      Name
  - AnimTrack[] Tracks
  
- AnimTrack
  - String NodeName;
  - Vec3[] TranslationKeys;
  - Quat[] RotationKeys;
  - Vec3[] ScaleKeys;

- BoneInput
  - Mat4x4 MeshToBoneSpace;

- BoneOutput
  - Mat4x4 FinalTransformation;

- <Model, Anim> Specific Data
  - AnimTrack*[] BoneToTrack


# Rand Notes On Rendering Metrics

- Aim for ~thousands of draw calls or less (1000 - 2300 max).


# Text Notes

  - [Making a Win32 Text Editor](http://www.catch22.net/tuts/neatpad/neatpad-overview#)
  - [http://utf8everywhere.org/](http://utf8everywhere.org/)

# (Un/Re)do

Ryan Renderer API
```c
R_CommandBuffer command_buffer = {0};
R_BeginCommandBuffer(&command_buffer);
{
  R_PushRect(...);
  R_PushFoo(...);
  R_PushBlah(...);
}
R_EndCommandBuffer();

// Push the commands to the main command buffer, in a different order with a deferred transform
R_PushTransform(...);
R_PushCommandBuffer(command_buffer);
R_PopTransform();
```

```cpp
#define TIMED_BLOCK(...) for(int _i_ = (begin_timed_block(...), 0); !_i_; (_i_ += 1), end_timed_block(...))
```

```
a <  b : a < b
a <= b : !(b < a)
a == b : !(a < b) && !(b < a)
a >= b : !(a < b)
a >  b : b < a
```

Windows Has a Default THread Pool.
