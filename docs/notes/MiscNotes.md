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


---


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

# True Misc

__debugbreak()

__builtin_trap()

Coefficients for rgb are 0.2, 0.7, 0.1 


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
```cpp
--- C++ Scripting

class Script
	{
	public:
		Script() {};
		virtual void Update() = 0;
		virtual void Start() = 0;
		virtual void OnCollisionEnter() {};
		std::string ScriptPath;
	};

  #include "Script.h"
#include 

class TestScriptClass : public Script
{
	public:
		TestScriptClass(){};
		~TestScriptClass(){};
		void Update()
		{
			printf("Update");
		};
		void Start()
		{
			printf("Start");
		};
};


extern "C"
{
	__declspec(dllexport) Script *CreateScript()
	{
		return new TestScriptClass();
	}
}

typedef DI::Script* (__stdcall *ScriptPtr)()

const char* ScriptName = "TestScriptClass.dll";

HINSTANCE hGetProcIDDLL = LoadLibraryA(ScriptName);

if (hGetProcIDDLL) {
  ScriptPtr CreateScript = (ScriptPtr)GetProcAddress(hGetProcIDDLL, "CreateScript");

  if (CreateScript) {
    CreateScriptMap[ScriptName] = CreateScript;
  } else {
    // Error could find 'CreateScript' in ScriptName.
  }
} else {
  // Error Could not load scrpt dll.
}
```
/*
search for "serialization"
same thing
fwrite whole struct is fine, but that prevents you to update/patch game with changed struct contents, so usually you write some kind of header with version, and then have different code paths for different versions (if necessary to support older saves)
also depending on game you might want to validate values loaded back, to prevent buffer overflows and other memory issues when somebody messes up with bytes in file (or accidental file corruption)
also never fopen existing save to overwrite with new data. Always create new file, and then rename over old file, if you need to overwrite
this will prevent save file corruption when writing file fails for some reason (only partial amount of bytes written)
*/
