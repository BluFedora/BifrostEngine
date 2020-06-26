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
