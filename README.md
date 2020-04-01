# Bifrost Engine

## Description

A toy game engine with a custom scripting language.
The main engine requires C++17 but there are submodules which are C99, C11, or C++11 compliant.

## Build System 

- CMake v3.8
- Currently only supports MSVC (VS2019) but eventually will also support Clang and GCC.

## Engine Modules

The engine is separated into modules with explicit dependencies on each other.

### Asset IO

Relies on:
- [Utility](#Utility)
- [Data Structures](#Data Structures)

### Core

Relies on:
- [Memory](#Memory)
- [Data Structures](#Data Structures)

### Data Structures

Relies on:
- [Memory](#Memory)
- [Utility](#Utility)

### Debug

Relies on:
- [Platform](#Platform)

### Event

Relies on:

Build Requirements:
- C++99

### Graphics

Relies on:
- [Data Structures](#Data Structures)

Build Requirements:
- C++11 Implementation
- C99 API

### Memory

Relies on:

Build Requirements:
- C++11 API and Implementation

### Meta System

Relies on:
- [Memory](#Memory)
- [Data Structures](#Data Structures)

Build Requirements:
- C++17 API and Implementation

### Platform

Relies on:
- [Core](#Core)
- [Event](#Event)
- [Data Structures](#Data Structures)
- [Utility](#Utility)

### Utility

Relies on:

Build Requirements:
- C++17 For Hash
- C99 for UUID
