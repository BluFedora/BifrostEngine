/******************************************************************************/
/*!
* @file   bifrost_glsl_compiler.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-26
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bf/graphics/bifrost_glsl_compiler.hpp"

#include "bf/asset_io/bifrost_file.hpp"
#include "bf/debug/bifrost_dbg_logger.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <regex>

namespace bf
{
  const static std::regex           k_IncludeRegex(R"(^(#include)(?:.|)+("|'|<)(\S+)(>|"|');?((.|\n|\r)*)?)",
                                         std::regex::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);
  const static std::sregex_iterator k_IncludeRegexSentinel;

  GLSLCompiler::GLSLCompiler(IMemoryManager& memory) :
    m_LoadedFiles{},
    m_CurrentlyCompiling{memory}
  {
    glslang::InitializeProcess();

    // glslang::OutputSpvHex(SpirV, "Output.cpp, "varName"); // Generates an array with the spv code with variable name being varName ;), Might Be useful later.
  }

  GLSLCompiler::~GLSLCompiler()
  {
    glslang::FinalizeProcess();
  }

  const String& GLSLCompiler::load(const String& filename)
  {
    for (const String* const file : m_CurrentlyCompiling)
    {
      if (*file == filename)
      {
        throw "Circular Dependency / Includes.";
      }
    }

    const auto it = m_LoadedFiles.find(filename);

    if (it != m_LoadedFiles.end())
    {
      return it->value();
    }

    std::ifstream file(filename.c_str());

    if (file)
    {
      String      processed_file;
      std::string line;
      std::smatch match;

      m_CurrentlyCompiling.push(&filename);

      while (std::getline(file, line))
      {
        const bool is_potential_include = line.length() > 0 && line[0] == '#';

        if (is_potential_include && std::regex_match(line, k_IncludeRegex))
        {
          for (std::sregex_iterator next(line.begin(), line.end(), k_IncludeRegex); next != k_IncludeRegexSentinel; ++next)
          {
            match = (*next);

            const auto include_match = match[3];

            std::string file_name2 = include_match.str();

            const String  included_file = {file_name2.c_str(), file_name2.length()};
            const String& included_src  = load(included_file);

            processed_file.append(included_src);
          }
        }
        else
        {
          processed_file.append(line.c_str(), line.length());
          processed_file.append('\n');
        }
      }
      file.close();
      m_CurrentlyCompiling.pop();

      m_LoadedFiles.emplace(filename, processed_file);
#if 0
      // TODO : THis should not be harded like this.
      // Write out the compiled file ;)
      // const StringRange filename_path = file::directoryOfFile(filename);
      const StringRange filename_name = file::fileNameOfPath(filename);

      File file_out{"../assets/shaders/standard/processed/" + filename_name, file::FILE_MODE_WRITE};

      if (file_out)
      {
        file_out.write(processed_file);

        file_out.close();
      }
#endif
    }
    else
    {
      throw "Failed to load file";
    }

    return *m_LoadedFiles.at(filename);
  }

  static TBuiltInResource InitResources()
  {
    TBuiltInResource Resources;

    Resources.maxLights                                 = 32;
    Resources.maxClipPlanes                             = 6;
    Resources.maxTextureUnits                           = 32;
    Resources.maxTextureCoords                          = 32;
    Resources.maxVertexAttribs                          = 64;
    Resources.maxVertexUniformComponents                = 4096;
    Resources.maxVaryingFloats                          = 64;
    Resources.maxVertexTextureImageUnits                = 32;
    Resources.maxCombinedTextureImageUnits              = 80;
    Resources.maxTextureImageUnits                      = 32;
    Resources.maxFragmentUniformComponents              = 4096;
    Resources.maxDrawBuffers                            = 32;
    Resources.maxVertexUniformVectors                   = 128;
    Resources.maxVaryingVectors                         = 8;
    Resources.maxFragmentUniformVectors                 = 16;
    Resources.maxVertexOutputVectors                    = 16;
    Resources.maxFragmentInputVectors                   = 15;
    Resources.minProgramTexelOffset                     = -8;
    Resources.maxProgramTexelOffset                     = 7;
    Resources.maxClipDistances                          = 8;
    Resources.maxComputeWorkGroupCountX                 = 65535;
    Resources.maxComputeWorkGroupCountY                 = 65535;
    Resources.maxComputeWorkGroupCountZ                 = 65535;
    Resources.maxComputeWorkGroupSizeX                  = 1024;
    Resources.maxComputeWorkGroupSizeY                  = 1024;
    Resources.maxComputeWorkGroupSizeZ                  = 64;
    Resources.maxComputeUniformComponents               = 1024;
    Resources.maxComputeTextureImageUnits               = 16;
    Resources.maxComputeImageUniforms                   = 8;
    Resources.maxComputeAtomicCounters                  = 8;
    Resources.maxComputeAtomicCounterBuffers            = 1;
    Resources.maxVaryingComponents                      = 60;
    Resources.maxVertexOutputComponents                 = 64;
    Resources.maxGeometryInputComponents                = 64;
    Resources.maxGeometryOutputComponents               = 128;
    Resources.maxFragmentInputComponents                = 128;
    Resources.maxImageUnits                             = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    Resources.maxCombinedShaderOutputResources          = 8;
    Resources.maxImageSamples                           = 0;
    Resources.maxVertexImageUniforms                    = 0;
    Resources.maxTessControlImageUniforms               = 0;
    Resources.maxTessEvaluationImageUniforms            = 0;
    Resources.maxGeometryImageUniforms                  = 0;
    Resources.maxFragmentImageUniforms                  = 8;
    Resources.maxCombinedImageUniforms                  = 8;
    Resources.maxGeometryTextureImageUnits              = 16;
    Resources.maxGeometryOutputVertices                 = 256;
    Resources.maxGeometryTotalOutputComponents          = 1024;
    Resources.maxGeometryUniformComponents              = 1024;
    Resources.maxGeometryVaryingComponents              = 64;
    Resources.maxTessControlInputComponents             = 128;
    Resources.maxTessControlOutputComponents            = 128;
    Resources.maxTessControlTextureImageUnits           = 16;
    Resources.maxTessControlUniformComponents           = 1024;
    Resources.maxTessControlTotalOutputComponents       = 4096;
    Resources.maxTessEvaluationInputComponents          = 128;
    Resources.maxTessEvaluationOutputComponents         = 128;
    Resources.maxTessEvaluationTextureImageUnits        = 16;
    Resources.maxTessEvaluationUniformComponents        = 1024;
    Resources.maxTessPatchComponents                    = 120;
    Resources.maxPatchVertices                          = 32;
    Resources.maxTessGenLevel                           = 64;
    Resources.maxViewports                              = 16;
    Resources.maxVertexAtomicCounters                   = 0;
    Resources.maxTessControlAtomicCounters              = 0;
    Resources.maxTessEvaluationAtomicCounters           = 0;
    Resources.maxGeometryAtomicCounters                 = 0;
    Resources.maxFragmentAtomicCounters                 = 8;
    Resources.maxCombinedAtomicCounters                 = 8;
    Resources.maxAtomicCounterBindings                  = 1;
    Resources.maxVertexAtomicCounterBuffers             = 0;
    Resources.maxTessControlAtomicCounterBuffers        = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers     = 0;
    Resources.maxGeometryAtomicCounterBuffers           = 0;
    Resources.maxFragmentAtomicCounterBuffers           = 1;
    Resources.maxCombinedAtomicCounterBuffers           = 1;
    Resources.maxAtomicCounterBufferSize                = 16384;
    Resources.maxTransformFeedbackBuffers               = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances                          = 8;
    Resources.maxCombinedClipAndCullDistances           = 8;
    Resources.maxSamples                                = 4;
    Resources.maxMeshOutputVerticesNV                   = 256;
    Resources.maxMeshOutputPrimitivesNV                 = 512;
    Resources.maxMeshWorkGroupSizeX_NV                  = 32;
    Resources.maxMeshWorkGroupSizeY_NV                  = 1;
    Resources.maxMeshWorkGroupSizeZ_NV                  = 1;
    Resources.maxTaskWorkGroupSizeX_NV                  = 32;
    Resources.maxTaskWorkGroupSizeY_NV                  = 1;
    Resources.maxTaskWorkGroupSizeZ_NV                  = 1;
    Resources.maxMeshViewCountNV                        = 4;

    Resources.limits.nonInductiveForLoops                 = 1;
    Resources.limits.whileLoops                           = 1;
    Resources.limits.doWhileLoops                         = 1;
    Resources.limits.generalUniformIndexing               = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing               = 1;
    Resources.limits.generalSamplerIndexing               = 1;
    Resources.limits.generalVariableIndexing              = 1;
    Resources.limits.generalConstantMatrixVectorIndexing  = 1;

    return Resources;
  }

  Array<std::uint32_t> GLSLCompiler::toSPIRV(const String& source, bfShaderType type) const
  {
#if 0
    const TBuiltInResource DefaultTBuiltInResource = {
     /* .MaxLights = */ 32,
     /* .MaxClipPlanes = */ 6,
     /* .MaxTextureUnits = */ 32,
     /* .MaxTextureCoords = */ 32,
     /* .MaxVertexAttribs = */ 64,
     /* .MaxVertexUniformComponents = */ 4096,
     /* .MaxVaryingFloats = */ 64,
     /* .MaxVertexTextureImageUnits = */ 32,
     /* .MaxCombinedTextureImageUnits = */ 80,
     /* .MaxTextureImageUnits = */ 32,
     /* .MaxFragmentUniformComponents = */ 4096,
     /* .MaxDrawBuffers = */ 32,
     /* .MaxVertexUniformVectors = */ 128,
     /* .MaxVaryingVectors = */ 8,
     /* .MaxFragmentUniformVectors = */ 16,
     /* .MaxVertexOutputVectors = */ 16,
     /* .MaxFragmentInputVectors = */ 15,
     /* .MinProgramTexelOffset = */ -8,
     /* .MaxProgramTexelOffset = */ 7,
     /* .MaxClipDistances = */ 8,
     /* .MaxComputeWorkGroupCountX = */ 65535,
     /* .MaxComputeWorkGroupCountY = */ 65535,
     /* .MaxComputeWorkGroupCountZ = */ 65535,
     /* .MaxComputeWorkGroupSizeX = */ 1024,
     /* .MaxComputeWorkGroupSizeY = */ 1024,
     /* .MaxComputeWorkGroupSizeZ = */ 64,
     /* .MaxComputeUniformComponents = */ 1024,
     /* .MaxComputeTextureImageUnits = */ 16,
     /* .MaxComputeImageUniforms = */ 8,
     /* .MaxComputeAtomicCounters = */ 8,
     /* .MaxComputeAtomicCounterBuffers = */ 1,
     /* .MaxVaryingComponents = */ 60,
     /* .MaxVertexOutputComponents = */ 64,
     /* .MaxGeometryInputComponents = */ 64,
     /* .MaxGeometryOutputComponents = */ 128,
     /* .MaxFragmentInputComponents = */ 128,
     /* .MaxImageUnits = */ 8,
     /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
     /* .MaxCombinedShaderOutputResources = */ 8,
     /* .MaxImageSamples = */ 0,
     /* .MaxVertexImageUniforms = */ 0,
     /* .MaxTessControlImageUniforms = */ 0,
     /* .MaxTessEvaluationImageUniforms = */ 0,
     /* .MaxGeometryImageUniforms = */ 0,
     /* .MaxFragmentImageUniforms = */ 8,
     /* .MaxCombinedImageUniforms = */ 8,
     /* .MaxGeometryTextureImageUnits = */ 16,
     /* .MaxGeometryOutputVertices = */ 256,
     /* .MaxGeometryTotalOutputComponents = */ 1024,
     /* .MaxGeometryUniformComponents = */ 1024,
     /* .MaxGeometryVaryingComponents = */ 64,
     /* .MaxTessControlInputComponents = */ 128,
     /* .MaxTessControlOutputComponents = */ 128,
     /* .MaxTessControlTextureImageUnits = */ 16,
     /* .MaxTessControlUniformComponents = */ 1024,
     /* .MaxTessControlTotalOutputComponents = */ 4096,
     /* .MaxTessEvaluationInputComponents = */ 128,
     /* .MaxTessEvaluationOutputComponents = */ 128,
     /* .MaxTessEvaluationTextureImageUnits = */ 16,
     /* .MaxTessEvaluationUniformComponents = */ 1024,
     /* .MaxTessPatchComponents = */ 120,
     /* .MaxPatchVertices = */ 32,
     /* .MaxTessGenLevel = */ 64,
     /* .MaxViewports = */ 16,
     /* .MaxVertexAtomicCounters = */ 0,
     /* .MaxTessControlAtomicCounters = */ 0,
     /* .MaxTessEvaluationAtomicCounters = */ 0,
     /* .MaxGeometryAtomicCounters = */ 0,
     /* .MaxFragmentAtomicCounters = */ 8,
     /* .MaxCombinedAtomicCounters = */ 8,
     /* .MaxAtomicCounterBindings = */ 1,
     /* .MaxVertexAtomicCounterBuffers = */ 0,
     /* .MaxTessControlAtomicCounterBuffers = */ 0,
     /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
     /* .MaxGeometryAtomicCounterBuffers = */ 0,
     /* .MaxFragmentAtomicCounterBuffers = */ 1,
     /* .MaxCombinedAtomicCounterBuffers = */ 1,
     /* .MaxAtomicCounterBufferSize = */ 16384,
     /* .MaxTransformFeedbackBuffers = */ 4,
     /* .MaxTransformFeedbackInterleavedComponents = */ 64,
     /* .MaxCullDistances = */ 8,
     /* .MaxCombinedClipAndCullDistances = */ 8,
     /* .MaxSamples = */ 4,
     /* .maxMeshOutputVerticesNV = */ 256,
     /* .maxMeshOutputPrimitivesNV = */ 512,
     /* .maxMeshWorkGroupSizeX_NV = */ 32,
     /* .maxMeshWorkGroupSizeY_NV = */ 1,
     /* .maxMeshWorkGroupSizeZ_NV = */ 1,
     /* .maxTaskWorkGroupSizeX_NV = */ 32,
     /* .maxTaskWorkGroupSizeY_NV = */ 1,
     /* .maxTaskWorkGroupSizeZ_NV = */ 1,
     /* .maxMeshViewCountNV = */ 4,
     /* .limits = */ {
      /* .nonInductiveForLoops = */ true,
      /* .whileLoops = */ true,
      /* .doWhileLoops = */ true,
      /* .generalUniformIndexing = */ true,
      /* .generalAttributeMatrixVectorIndexing = */ true,
      /* .generalVaryingIndexing = */ true,
      /* .generalSamplerIndexing = */ true,
      /* .generalVariableIndexing = */ true,
      /* .generalConstantMatrixVectorIndexing = */ true,
     },
    };
#else
    const TBuiltInResource DefaultTBuiltInResource = InitResources();
#endif

    EShLanguage shader_type = EShLangVertex;

    switch (type)
    {
      case BF_SHADER_TYPE_VERTEX:
        shader_type = EShLangVertex;
        break;
      case BF_SHADER_TYPE_TESSELLATION_CONTROL:
        shader_type = EShLangTessControl;
        break;
      case BF_SHADER_TYPE_TESSELLATION_EVALUATION:
        shader_type = EShLangTessEvaluation;
        break;
      case BF_SHADER_TYPE_GEOMETRY:
        shader_type = EShLangGeometry;
        break;
      case BF_SHADER_TYPE_FRAGMENT:
        shader_type = EShLangFragment;
        break;
      case BF_SHADER_TYPE_COMPUTE:
        shader_type = EShLangCompute;
        break;
      default:
        throw "Invalid shader type";
        break;
    }

    glslang::TShader          shader(shader_type);
    const char* const         source_raw = source.c_str();
    glslang::TProgram         program;
    std::vector<unsigned int> spir_v;
    spv::SpvBuildLogger       logger;
    glslang::SpvOptions       spvOptions;

    shader.setStrings(&source_raw, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, shader_type, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

#if 0
      std::string PreprocessedGLSL;

      if (!Shader.preprocess(&DefaultTBuiltInResource, 100, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer))
      {
        std::cout << "GLSL Preprocessing Failed for: " << filename << std::endl;
        std::cout << Shader.getInfoLog() << std::endl;
        std::cout << Shader.getInfoDebugLog() << std::endl;
      }

      const char* PreprocessedCStr = PreprocessedGLSL.c_str();
      Shader.setStrings(&PreprocessedCStr, 1);
#endif

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(&DefaultTBuiltInResource, 100, false, messages))
    {
      bfLogPrint("%s", source.c_str());
      bfLogPush("Shader Parse Error:");
      bfLogError("Info Log       : %s", shader.getInfoLog());
      bfLogError("Info Debug Log : %s", shader.getInfoDebugLog());
      bfLogPop();

      throw "Shader Parse Error";
    }

    program.addShader(&shader);

    if (!program.link(messages))
    {
      bfLogPrint("\n\n%s\n\n", source.c_str());
      bfLogPush("Shader Link Error:");
      bfLogError("Info Log       : %s", shader.getInfoLog());
      bfLogError("Info Debug Log : %s", shader.getInfoDebugLog());
      bfLogPop();

      throw "Shader Link Error";
    }

    GlslangToSpv(*program.getIntermediate(shader_type), spir_v, &logger, &spvOptions);

    Array<std::uint32_t> result{m_CurrentlyCompiling.memory()};

    result.resize(spir_v.size());

    std::copy(spir_v.begin(), spir_v.end(), result.begin());

    return result;
  }

  bfShaderModuleHandle GLSLCompiler::createModule(bfGfxDeviceHandle device, const String& filename, bfShaderType type)
  {
    const bfShaderModuleHandle module = bfGfxDevice_newShaderModule(device, type);

    const String& source = load(filename);

#if BF_PLATFORM_USE_VULKAN
    Array<std::uint32_t> spirv_code = toSPIRV(source, type);
#else
    const String&          spirv_code              = source;
#endif

    if (!bfShaderModule_loadData(module, (const char*)spirv_code.data(), spirv_code.size() * sizeof(spirv_code[0])))
    {
      throw "Bad SPIR-V";
    }

    return module;
  }

  bfShaderModuleHandle GLSLCompiler::createModule(bfGfxDeviceHandle device, const String& filename)
  {
    static constexpr char k_VertexShaderExt[]       = ".vert.glsl";
    static constexpr int  k_VertexShaderExtLength   = sizeof(k_VertexShaderExt) - 1;
    static constexpr char k_FragmentShaderExt[]     = ".frag.glsl";
    static constexpr int  k_FragmentShaderExtLength = sizeof(k_FragmentShaderExt) - 1;

    const StringRange path = filename;

    if (file::pathEndsIn(path.begin(), k_VertexShaderExt, k_VertexShaderExtLength, (int)path.length()))
    {
      return createModule(device, filename, BF_SHADER_TYPE_VERTEX);
    }

    if (file::pathEndsIn(path.begin(), k_FragmentShaderExt, k_FragmentShaderExtLength, (int)path.length()))
    {
      return createModule(device, filename, BF_SHADER_TYPE_FRAGMENT);
    }

    return nullptr;
  }
}  // namespace bf
