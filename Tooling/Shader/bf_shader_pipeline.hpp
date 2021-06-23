#ifndef BF_TOOLING_SHADER_PIPELINE_HPP
#define BF_TOOLING_SHADER_PIPELINE_HPP

#include "bf/bf_gfx_api.h"                            // bfShaderType
#include "bf/data_structures/bifrost_array.hpp"       // Array<T>
#include "bf/data_structures/bifrost_hash_table.hpp"  // HashTable<K, V>
#include "bf/data_structures/bifrost_string.hpp"      // String

namespace bf
{
  using SPIRVArray = Array<std::uint32_t>;

  enum class ShaderPipelineError
  {
    NONE,
    FAILED_TO_INITIALIZE,
    CIRCULAR_INCLUDE_DETECTED,
    FAILED_TO_OPEN_FILE,
    FAILED_TO_LINK_SHADER,
    FAILED_TO_PARSE_SHADER,
  };

  struct ShaderPreprocessContext
  {
    HashTable<String, String> loadedFiles;   //!< <Path, Source>.
    Array<StringRange>        compileStack;  //!< Used for detecting circular includes.
    Array<String>             includePaths;  //!< Where to look for files when .

    ShaderPreprocessContext(IMemoryManager& memory) :
      loadedFiles{},
      compileStack{memory},
      includePaths{memory}
    {
    }
  };

  //
  // These need to be called once per process.
  //
  ShaderPipelineError ShaderPipeline_startup();
  void                ShaderPipeline_shutdown();

  // This function does not clear 'result' but will only append to it even if an error occured in the middle of processing.
  ShaderPipelineError ShaderPipeline_preprocessSource(ShaderPreprocessContext& ctx, StringRange source, String& result);
  ShaderPipelineError ShaderPipeline_compileToSPRIV(StringRange source, bfShaderType type, SPIRVArray& result);

}  // namespace bf

#endif /* BF_TOOLING_SHADER_PIPELINE_HPP */