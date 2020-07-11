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
#include "bifrost/graphics/bifrost_glsl_compiler.hpp"

#include "bifrost/asset_io/bifrost_file.hpp"
#include "bifrost/debug/bifrost_dbg_logger.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <regex>

namespace bifrost
{
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
    const static std::regex           k_IncludeRegex(R"(^(#include)(?:.|)+("|'|<)(\S+)(>|"|');?((.|\n|\r)*)?)",
                                           std::regex::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);
    const static std::sregex_iterator k_IncludeRegexSentinel;

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

  Array<std::uint32_t> GLSLCompiler::toSPIRV(const String& source, BifrostShaderType type) const
  {
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
     }};

    EShLanguage shader_type = EShLangVertex;

    switch (type)
    {
      case BIFROST_SHADER_TYPE_VERTEX:
        shader_type = EShLangVertex;
        break;
      case BIFROST_SHADER_TYPE_TESSELLATION_CONTROL:
        shader_type = EShLangTessControl;
        break;
      case BIFROST_SHADER_TYPE_TESSELLATION_EVALUATION:
        shader_type = EShLangTessEvaluation;
        break;
      case BIFROST_SHADER_TYPE_GEOMETRY:
        shader_type = EShLangGeometry;
        break;
      case BIFROST_SHADER_TYPE_FRAGMENT:
        shader_type = EShLangFragment;
        break;
      case BIFROST_SHADER_TYPE_COMPUTE:
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
      bfLogPrint("%s", source.c_str());
      bfLogPush("Shader Link Error:");
      bfLogError("Info Log       : %s", shader.getInfoLog());
      bfLogError("Info Debug Log : %s", shader.getInfoDebugLog());
      bfLogPop();

      throw "Shader Link Error";
    }

    GlslangToSpv(*program.getIntermediate(shader_type), spir_v, &logger, &spvOptions);

    Array<std::uint32_t> result{m_CurrentlyCompiling.memory()};

    result.reserve(spir_v.size());

    for (unsigned int code : spir_v)
    {
      result.push(code);
    }

    return result;
  }

  bfShaderModuleHandle GLSLCompiler::createModule(bfGfxDeviceHandle device, const String& filename, BifrostShaderType type)
  {
    const bfShaderModuleHandle module = bfGfxDevice_newShaderModule(device, type);

    const String& source = load(filename);

#if BIFROST_PLATFORM_USE_VULKAN
    Array<std::uint32_t> spirv_code = toSPIRV(source, type);
#else
    const String& spirv_code = source;
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
    BifrostShaderType type;

    if (file::pathEndsIn(path.begin(), k_VertexShaderExt, k_VertexShaderExtLength, (int)path.length()))
    {
      type = BIFROST_SHADER_TYPE_VERTEX;
    }
    else if (file::pathEndsIn(path.begin(), k_FragmentShaderExt, k_FragmentShaderExtLength, (int)path.length()))
    {
      type = BIFROST_SHADER_TYPE_FRAGMENT;
    }
    else
    {
      return nullptr;
    }

    return createModule(device, filename, type);
  }
}  // namespace bifrost
