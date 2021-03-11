/******************************************************************************/
/*!
* @file   bf_glsl_compiler.hpp
* @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
* @brief
*   Allows for compiling glsl into SPIR-V at runtime to allow for 
*   shader hot reloading.
*
* @version 0.0.1
* @date    2020-03-26
*
* @copyright Copyright (c) 2020-2021
*/
/******************************************************************************/
#ifndef BF_GLSL_COMPILER_HPP
#define BF_GLSL_COMPILER_HPP

#include "bf/bf_gfx_api.h"                            // bfShaderModuleHandle
#include "bf/bf_non_copy_move.hpp"                    // bfNonCopyMoveable
#include "bf/data_structures/bifrost_array.hpp"       // Array<T>
#include "bf/data_structures/bifrost_hash_table.hpp"  // HashTable<K, V>
#include "bf/data_structures/bifrost_string.hpp"      // String

namespace bf
{
  //
  // Only create one of these per process (This is a requirement of the "glslang" library).
  //
  class GLSLCompiler : private NonCopyMoveable<GLSLCompiler>
  {
   private:
    HashTable<String, String> m_LoadedFiles;         //!< <Path, Source>
    Array<const String*>      m_CurrentlyCompiling;  //!< Used for detecting circular includes.

   public:
    explicit GLSLCompiler(IMemoryManager& memory);
    ~GLSLCompiler();

    const String&        load(const String& filename);
    Array<std::uint32_t> toSPIRV(const String& source, bfShaderType type) const;
    bfShaderModuleHandle createModule(bfGfxDeviceHandle device, const String& filename, bfShaderType type);
    bfShaderModuleHandle createModule(bfGfxDeviceHandle device, const String& filename);
  };
}  // namespace bf

#endif /* BF_GLSL_COMPILER_HPP */
