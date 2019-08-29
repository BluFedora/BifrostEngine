---
header-includes:
  - \hypersetup{
      colorlinks=true,
      linkcolor=red,
      filecolor=cyan,
      allbordercolors={1 0 0},
      pdfborderstyle={/S/U/W 1}
    }
title:       "Bifrost : Engine Specification 2019-2020 (v0.0.1)"
author:
- Created By Shareef Abdoul-Raheem
date:        July 9th, 2019
geometry:    "left=3cm, right=3cm, top=3cm, bottom=3cm"
output:      pdf_document
rights:      (C) 2019 Copyright Shareef Abdoul-Raheem
language:    en-US
description: "This document is a very tentative overview of the Bifrost Game Engine."
cover-image: cover_image.png
abstract:    This document is a very tentative overview of the Bifrost Game Engine.
footer:      'Created By Shareef R'
header:      This is fancy
keywords:    [engine, architecture, bifrost, c, c++, game, api]

---

# Overview
  > This document describes the API design of the Engine as a whole not on any specifics.
  > Typically you will need to add more functionality than what is described here but this is the baseline so that all engine Modules can interact with each-other in a sane way.
  > The member variables on any class is a suggestion while the __member functions are a requirement__.
  <!-- > If you are contributing to the Engine layer of this project you must follow the [Style Guide](rising_tide_style_guide_2018.pdf) -->

---

### Dependencies

| Library                       | Explanation                                                                    |
|:----------------------------- |:------------------------------------------------------------------------------:|
| C++ STL (C++11, C++14, C++17) | Provides template containers so that we don't need to reinvent the wheel.      |
| C Runtime Library             | Provides a good set of utility functions such as 'cos', and 'sin'.             |
| glfw (w/ glad)                | Provides windowing (w/ OpenGL context creation) and input to our game.         |
| glm                           | Provides a very optimized and stable set of vector / matrix math operations.   |
| OpenGL v4.4                   | Provides a way to interface with the GPU Pipeline.                             |
| FMOD                          | Provides audio output and other sound utilities.                               |
| DEAR ImGui                    | Provides an easy way to create GUIs for in engine editing of our game.         |
| stb_image                     | Provides 2D asset loading utilities for use with OpenGL.                       |


## Engine Overview / Ideals

- Cmake 3.8 used for our build system.
  - "BifrostEngine/CmakeLists.txt"
- GitHub used for source control
  - 'BifrostEngine/*'
- Visual Studio for compiling and debugging although not strictly required by our setup.
- Clangformat is used for enforcing the styleguide.

---

> 2019-2020 &copy; All Rights Reserved Shareef Raheem
