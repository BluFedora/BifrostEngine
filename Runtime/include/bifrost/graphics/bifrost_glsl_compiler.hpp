/******************************************************************************/
/*!
* @file   bifrost_glsl_compiler.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-26
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_GLSL_COMPILER_HPP
#define BIFROST_GLSL_COMPILER_HPP

#include "bf/bf_non_copy_move.hpp"                         // bfNonCopyMoveable
#include "bifrost/data_structures/bifrost_array.hpp"       // Array<T>
#include "bifrost/data_structures/bifrost_hash_table.hpp"  // HashTable<K, V>
#include "bifrost/data_structures/bifrost_string.hpp"      // String
#include "bifrost/graphics/bifrost_gfx_api.h"              // BifrostShaderType

namespace bf
{
  //
  // Only create one of these per process.
  //
  class GLSLCompiler : private NonCopyMoveable<GLSLCompiler>
  {
   private:
    HashTable<String, String> m_LoadedFiles;
    Array<const String*>      m_CurrentlyCompiling;

   public:
    explicit GLSLCompiler(IMemoryManager& memory);
    ~GLSLCompiler();

    const String&        load(const String& filename);
    Array<std::uint32_t> toSPIRV(const String& source, BifrostShaderType type) const;
    bfShaderModuleHandle createModule(bfGfxDeviceHandle device, const String& filename, BifrostShaderType type);
    bfShaderModuleHandle createModule(bfGfxDeviceHandle device, const String& filename);
  };
}  // namespace bf

#endif /* BIFROST_GLSL_COMPILER_HPP */
