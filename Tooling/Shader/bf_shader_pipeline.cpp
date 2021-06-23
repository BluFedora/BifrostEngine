#include "bf_shader_pipeline.hpp"

#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/bf_dbg_logger.h "

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <fstream>
#include <regex>

namespace bf
{
  static constexpr auto             k_IncludeRegexFlags = std::regex::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize;
  static const std::regex           k_IncludeRegex(R"(^(#include)(?:.|)+("|'|<)(\S+)(>|"|');?((.|\n|\r)*)?)", k_IncludeRegexFlags);
  static const std::cregex_iterator k_IncludeRegexSentinel;

  ShaderPipelineError ShaderPipeline_startup()
  {
    if (!glslang::InitializeProcess())
    {
      return ShaderPipelineError::FAILED_TO_INITIALIZE;
    }

    return ShaderPipelineError::NONE;
  }

  void ShaderPipeline_shutdown()
  {
    glslang::FinalizeProcess();
  }

  static ShaderPipelineError ShaderPipeline_preprocessSourceHelper(ShaderPreprocessContext& ctx, StringRange filename, String*& result);

  static ShaderPipelineError ShaderPipeline_preprocessSourceProcessLine(std::cmatch& match, String& processed_file, ShaderPreprocessContext& ctx, StringRange line)
  {
    const bool is_potential_include = line.length() > 0 && line[0] == '#';

    if (is_potential_include && std::regex_match(line.begin(), line.end(), k_IncludeRegex))
    {
      for (std::cregex_iterator next(line.begin(), line.end(), k_IncludeRegex); next != k_IncludeRegexSentinel; ++next)
      {
        match = (*next);

        const auto& include_match = match[3];

        std::string  file_name2    = include_match.str();
        const String included_file = {file_name2.c_str(), file_name2.length()};
        String*      included_src;

        const auto err = ShaderPipeline_preprocessSourceHelper(ctx, included_file, included_src);

        if (err != ShaderPipelineError::NONE)
        {
          return err;
        }

        processed_file.append(*included_src);
      }
    }
    else
    {
      processed_file.append(line.data(), line.length());
      processed_file.append('\n');
    }

    return ShaderPipelineError::NONE;
  }

  static ShaderPipelineError ShaderPipeline_preprocessSourceHelper(ShaderPreprocessContext& ctx, StringRange filename, String*& result)
  {
    for (StringRange file : ctx.compileStack)
    {
      if (file == filename)
      {
        return ShaderPipelineError::CIRCULAR_INCLUDE_DETECTED;
      }
    }

    const auto it = ctx.loadedFiles.find(filename);

    ShaderPipelineError error = ShaderPipelineError::NONE;

    if (it != ctx.loadedFiles.end())
    {
      result = &it->value();
    }
    else
    {
      char filename_cstr[path::k_MaxLength];
      std::memcpy(filename_cstr, filename.str_bgn, StringRange_length(filename));

      std::ifstream file(filename_cstr);

      if (file)
      {
        String      processed_file;
        std::cmatch match;

        ctx.compileStack.push(filename_cstr);

        std::string line;
        while (error == ShaderPipelineError::NONE && std::getline(file, line))
        {
          error = ShaderPipeline_preprocessSourceProcessLine(match, processed_file, ctx, {line.c_str(), line.length()});
        }

        file.close();
        ctx.compileStack.pop();

        if (error == ShaderPipelineError::NONE)
        {
          result = &ctx.loadedFiles.emplace(filename_cstr, processed_file)->value();
        }
      }
      else
      {
        return ShaderPipelineError::FAILED_TO_OPEN_FILE;
      }
    }

    return error;
  }

  ShaderPipelineError ShaderPipeline_preprocessSource(ShaderPreprocessContext& ctx, StringRange source, String& result)
  {
    ShaderPipelineError error = ShaderPipelineError::NONE;
    std::cmatch         match;

    string_utils::tokenize(source, '\n', [&](StringRange line) {
      error = ShaderPipeline_preprocessSourceProcessLine(match, result, ctx, line);
      return error == ShaderPipelineError::NONE;
    });

    return error;
  }

  ShaderPipelineError ShaderPipeline_compileToSPRIV(StringRange source, bfShaderType type, SPIRVArray& result)
  {
    TBuiltInResource DefaultTBuiltInResource;
    DefaultTBuiltInResource.maxLights                                 = 32;
    DefaultTBuiltInResource.maxClipPlanes                             = 6;
    DefaultTBuiltInResource.maxTextureUnits                           = 32;
    DefaultTBuiltInResource.maxTextureCoords                          = 32;
    DefaultTBuiltInResource.maxVertexAttribs                          = 64;
    DefaultTBuiltInResource.maxVertexUniformComponents                = 4096;
    DefaultTBuiltInResource.maxVaryingFloats                          = 64;
    DefaultTBuiltInResource.maxVertexTextureImageUnits                = 32;
    DefaultTBuiltInResource.maxCombinedTextureImageUnits              = 80;
    DefaultTBuiltInResource.maxTextureImageUnits                      = 32;
    DefaultTBuiltInResource.maxFragmentUniformComponents              = 4096;
    DefaultTBuiltInResource.maxDrawBuffers                            = 32;
    DefaultTBuiltInResource.maxVertexUniformVectors                   = 128;
    DefaultTBuiltInResource.maxVaryingVectors                         = 8;
    DefaultTBuiltInResource.maxFragmentUniformVectors                 = 16;
    DefaultTBuiltInResource.maxVertexOutputVectors                    = 16;
    DefaultTBuiltInResource.maxFragmentInputVectors                   = 15;
    DefaultTBuiltInResource.minProgramTexelOffset                     = -8;
    DefaultTBuiltInResource.maxProgramTexelOffset                     = 7;
    DefaultTBuiltInResource.maxClipDistances                          = 8;
    DefaultTBuiltInResource.maxComputeWorkGroupCountX                 = 65535;
    DefaultTBuiltInResource.maxComputeWorkGroupCountY                 = 65535;
    DefaultTBuiltInResource.maxComputeWorkGroupCountZ                 = 65535;
    DefaultTBuiltInResource.maxComputeWorkGroupSizeX                  = 1024;
    DefaultTBuiltInResource.maxComputeWorkGroupSizeY                  = 1024;
    DefaultTBuiltInResource.maxComputeWorkGroupSizeZ                  = 64;
    DefaultTBuiltInResource.maxComputeUniformComponents               = 1024;
    DefaultTBuiltInResource.maxComputeTextureImageUnits               = 16;
    DefaultTBuiltInResource.maxComputeImageUniforms                   = 8;
    DefaultTBuiltInResource.maxComputeAtomicCounters                  = 8;
    DefaultTBuiltInResource.maxComputeAtomicCounterBuffers            = 1;
    DefaultTBuiltInResource.maxVaryingComponents                      = 60;
    DefaultTBuiltInResource.maxVertexOutputComponents                 = 64;
    DefaultTBuiltInResource.maxGeometryInputComponents                = 64;
    DefaultTBuiltInResource.maxGeometryOutputComponents               = 128;
    DefaultTBuiltInResource.maxFragmentInputComponents                = 128;
    DefaultTBuiltInResource.maxImageUnits                             = 8;
    DefaultTBuiltInResource.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    DefaultTBuiltInResource.maxCombinedShaderOutputResources          = 8;
    DefaultTBuiltInResource.maxImageSamples                           = 0;
    DefaultTBuiltInResource.maxVertexImageUniforms                    = 0;
    DefaultTBuiltInResource.maxTessControlImageUniforms               = 0;
    DefaultTBuiltInResource.maxTessEvaluationImageUniforms            = 0;
    DefaultTBuiltInResource.maxGeometryImageUniforms                  = 0;
    DefaultTBuiltInResource.maxFragmentImageUniforms                  = 8;
    DefaultTBuiltInResource.maxCombinedImageUniforms                  = 8;
    DefaultTBuiltInResource.maxGeometryTextureImageUnits              = 16;
    DefaultTBuiltInResource.maxGeometryOutputVertices                 = 256;
    DefaultTBuiltInResource.maxGeometryTotalOutputComponents          = 1024;
    DefaultTBuiltInResource.maxGeometryUniformComponents              = 1024;
    DefaultTBuiltInResource.maxGeometryVaryingComponents              = 64;
    DefaultTBuiltInResource.maxTessControlInputComponents             = 128;
    DefaultTBuiltInResource.maxTessControlOutputComponents            = 128;
    DefaultTBuiltInResource.maxTessControlTextureImageUnits           = 16;
    DefaultTBuiltInResource.maxTessControlUniformComponents           = 1024;
    DefaultTBuiltInResource.maxTessControlTotalOutputComponents       = 4096;
    DefaultTBuiltInResource.maxTessEvaluationInputComponents          = 128;
    DefaultTBuiltInResource.maxTessEvaluationOutputComponents         = 128;
    DefaultTBuiltInResource.maxTessEvaluationTextureImageUnits        = 16;
    DefaultTBuiltInResource.maxTessEvaluationUniformComponents        = 1024;
    DefaultTBuiltInResource.maxTessPatchComponents                    = 120;
    DefaultTBuiltInResource.maxPatchVertices                          = 32;
    DefaultTBuiltInResource.maxTessGenLevel                           = 64;
    DefaultTBuiltInResource.maxViewports                              = 16;
    DefaultTBuiltInResource.maxVertexAtomicCounters                   = 0;
    DefaultTBuiltInResource.maxTessControlAtomicCounters              = 0;
    DefaultTBuiltInResource.maxTessEvaluationAtomicCounters           = 0;
    DefaultTBuiltInResource.maxGeometryAtomicCounters                 = 0;
    DefaultTBuiltInResource.maxFragmentAtomicCounters                 = 8;
    DefaultTBuiltInResource.maxCombinedAtomicCounters                 = 8;
    DefaultTBuiltInResource.maxAtomicCounterBindings                  = 1;
    DefaultTBuiltInResource.maxVertexAtomicCounterBuffers             = 0;
    DefaultTBuiltInResource.maxTessControlAtomicCounterBuffers        = 0;
    DefaultTBuiltInResource.maxTessEvaluationAtomicCounterBuffers     = 0;
    DefaultTBuiltInResource.maxGeometryAtomicCounterBuffers           = 0;
    DefaultTBuiltInResource.maxFragmentAtomicCounterBuffers           = 1;
    DefaultTBuiltInResource.maxCombinedAtomicCounterBuffers           = 1;
    DefaultTBuiltInResource.maxAtomicCounterBufferSize                = 16384;
    DefaultTBuiltInResource.maxTransformFeedbackBuffers               = 4;
    DefaultTBuiltInResource.maxTransformFeedbackInterleavedComponents = 64;
    DefaultTBuiltInResource.maxCullDistances                          = 8;
    DefaultTBuiltInResource.maxCombinedClipAndCullDistances           = 8;
    DefaultTBuiltInResource.maxSamples                                = 4;
    DefaultTBuiltInResource.maxMeshOutputVerticesNV                   = 256;
    DefaultTBuiltInResource.maxMeshOutputPrimitivesNV                 = 512;
    DefaultTBuiltInResource.maxMeshWorkGroupSizeX_NV                  = 32;
    DefaultTBuiltInResource.maxMeshWorkGroupSizeY_NV                  = 1;
    DefaultTBuiltInResource.maxMeshWorkGroupSizeZ_NV                  = 1;
    DefaultTBuiltInResource.maxTaskWorkGroupSizeX_NV                  = 32;
    DefaultTBuiltInResource.maxTaskWorkGroupSizeY_NV                  = 1;
    DefaultTBuiltInResource.maxTaskWorkGroupSizeZ_NV                  = 1;
    DefaultTBuiltInResource.maxMeshViewCountNV                        = 4;

    DefaultTBuiltInResource.limits.nonInductiveForLoops                 = 1;
    DefaultTBuiltInResource.limits.whileLoops                           = 1;
    DefaultTBuiltInResource.limits.doWhileLoops                         = 1;
    DefaultTBuiltInResource.limits.generalUniformIndexing               = 1;
    DefaultTBuiltInResource.limits.generalAttributeMatrixVectorIndexing = 1;
    DefaultTBuiltInResource.limits.generalVaryingIndexing               = 1;
    DefaultTBuiltInResource.limits.generalSamplerIndexing               = 1;
    DefaultTBuiltInResource.limits.generalVariableIndexing              = 1;
    DefaultTBuiltInResource.limits.generalConstantMatrixVectorIndexing  = 1;

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
    const char* const         source_raw = source.data();
    int                       source_len = int(source.length());
    glslang::TProgram         program;
    std::vector<unsigned int> spir_v;
    spv::SpvBuildLogger       logger;

    shader.setStringsWithLengths(&source_raw, &source_len, 1);
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
      shader.setStrings(&PreprocessedCStr, 1);
#endif

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(&DefaultTBuiltInResource, 100, false, messages))
    {
      bfLogPrint("%.*s", source_len, source_raw);
      bfLogPush("Shader Parse Error:");
      bfLogError("Info Log       : %s", shader.getInfoLog());
      bfLogError("Info Debug Log : %s", shader.getInfoDebugLog());
      bfLogPop();

      return ShaderPipelineError::FAILED_TO_PARSE_SHADER;
    }

    program.addShader(&shader);

    if (!program.link(messages))
    {
      bfLogPrint("\n\n%.*s\n\n", source_len, source_raw);
      bfLogPush("Shader Link Error:");
      bfLogError("Info Log       : %s", shader.getInfoLog());
      bfLogError("Info Debug Log : %s", shader.getInfoDebugLog());
      bfLogPop();

      return ShaderPipelineError::FAILED_TO_LINK_SHADER;
    }

    glslang::SpvOptions spvOptions;
    spvOptions.disableOptimizer = false;
    spvOptions.optimizeSize     = true;

    GlslangToSpv(*program.getIntermediate(shader_type), spir_v, &logger, &spvOptions);

    result.resize(spir_v.size());
    std::copy(spir_v.begin(), spir_v.end(), result.begin());

    return ShaderPipelineError::NONE;
  }
}  // namespace bf

// glslang::OutputSpvHex(SpirV, "Output.cpp, "varName"); // Generates an array with the spv code with variable name being varName ;), Might Be useful later.
