# BluFedora Engine Source Code

This engine is separated into separate modules / libraries that can be categorized into layers dictating on how many other components it depends on.

# Modules and Libraries

## Layer 0: Standalone Modules

This layer consists of libraries which only rely on code within it's own source tree.

- Core
- Dispatch
- Math
- [Memory][Memory]
- Network
- [Platform][Platform]
- Script (optionally uses: TMPUtils)
- TMPUtils

## Layer 1

- Anim2D          (Networking, Data Structures)
- Data Structures (Core, [Memory][Memory])
  - Some of the Data Structures are standalone an can be copied as is.
- [Job][Job]      ([Memory][Memory])
- Text            ([Memory][Memory])
- Graphics        (Core, Data Structures, [Memory][Memory], [Platform][Platform], TMPUtils)

## Layer 2

- AssetIO    (Data Structures, Graphics, Math, [Memory][Memory])
- Graphics2D ([Memory][Memory], Graphics, Runtime, Math, Text, [Platform][Platform])
- Editor     (AssetIO, Runtime)
- Runtime    ([Memory][Memory], Graphics, Runtime, Math, Text, [Platform][Platform])

# Source Structure 

## Logical Organization

- All C++ code will be contained in the `bf` namespace.
  - Ex: `bf::ClassName`
- All C code API will be prefixed with the letters `bf`.
  - Ex: `bfAlignPointerUp`

## Header Physical Design:

1) Public API Headers
    * There are API headers which live in the 'bf' folder with a `PascalCase` naming convention. This is so if you do not know where a certain class is all you have to do is `#include "bf/ClassName.hpp"` for easier discovery. The public headers are what should be used by code non part of the library.

2) Private Library Headers
    * These headers follow a `snake_case` naming convention and are used by the library internally. Multiple classes can be declared in the library headers but there should be a separate 'Public API Header' for each class. These headers are considered unstable and are allowed to be renamed and changed by library maintainers.
    * Ex: `bf/memory/bf_crt_allocator.hpp`

<!-- Link Definitions -->

[Job]:      https://github.com/BluFedora/BF-Job-System (Link to the BF Job Library Documentation)
[Memory]:   https://github.com/BluFedora/BF-Memory     (Link to the BF Memory Library Documentation)
[Platform]: https://github.com/BluFedora/BF-Platform   (Link to the BF Platform Library Documentation)

<!-- Link Definitions -->
