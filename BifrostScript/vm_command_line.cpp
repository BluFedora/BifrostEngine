/******************************************************************************/
/*!
 * @file   vm_command_line.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Command Line Interface for the Virtual Machine.
 * 
 * @version 0.0.1
 * @date    2020-02-17
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#include "include/bifrost/script/bifrost_vm.hpp"

#include <cstdio>    // printf, fopen, flocse, ftell, fseek, fread, malloc
#include <iostream>  // std::cin

static void errorHandler(BifrostVM* vm, BifrostVMError err, int line_no, const char* message) noexcept;
static void printHandler(BifrostVM* vm, const char* message) noexcept;
static void moduleHandler(BifrostVM* vm, const char* from, const char* module, BifrostVMModuleLookUp* out) noexcept;
static void waitForInput() noexcept;

int main(int argc, const char* argv[])
{
  if (argc != 2)
  {
    std::printf("usage %s <file-name>\n", argv[0]);
    waitForInput();
    return 0;
  }

  const char* const file_name = argv[1];

  bifrost::VMParams params{};
  params.error_fn  = &errorHandler;
  params.print_fn  = &printHandler;
  params.module_fn = &moduleHandler;

  bifrost::VM vm{params};

  BifrostVMModuleLookUp load_file;

  moduleHandler(vm, nullptr, file_name, &load_file);

  if (!load_file.source || load_file.source_len == 0)
  {
    std::printf("failed to load '%s'\n", file_name);
    return 1;
  }

  vm.stackResize(1);
  vm.moduleLoad(0, BIFROST_VM_STD_MODULE_ALL);

  const BifrostVMError err = vm.execInModule(nullptr, load_file.source, load_file.source_len);

  if (err)
  {
    errorHandler(vm, err, 0, vm.errorString());
  }

  std::free(const_cast<char*>(load_file.source));
  waitForInput();
  return 0;
}

static void errorHandler(BifrostVM* /*vm*/, BifrostVMError err, int line_no, const char* message) noexcept
{
  const char* err_type_str;

  switch (err)
  {
    case BIFROST_VM_ERROR_OUT_OF_MEMORY:
      err_type_str = "OOM";
      break;
    case BIFROST_VM_ERROR_RUNTIME:
      err_type_str = "Runtime";
      break;
    case BIFROST_VM_ERROR_LEXER:
      err_type_str = "Lexer";
      break;
    case BIFROST_VM_ERROR_COMPILE:
      err_type_str = "Compiler";
      break;
    case BIFROST_VM_ERROR_FUNCTION_ARITY_MISMATCH:
      err_type_str = "Function Arity Mismatch";
      break;
    case BIFROST_VM_ERROR_MODULE_ALREADY_DEFINED:
      err_type_str = "Module Already Exists";
      break;
    case BIFROST_VM_ERROR_MODULE_NOT_FOUND:
      err_type_str = "Missing Module";
      break;
    case BIFROST_VM_ERROR_INVALID_OP_ON_TYPE:
      err_type_str = "Invalid Type";
      break;
    case BIFROST_VM_ERROR_INVALID_ARGUMENT:
      err_type_str = "Invalid Arg";
      break;
    case BIFROST_VM_ERROR_STACK_TRACE_BEGIN:
      err_type_str = "Trace Bgn";
      break;
    case BIFROST_VM_ERROR_STACK_TRACE:
      err_type_str = "STACK";
      break;
    case BIFROST_VM_ERROR_STACK_TRACE_END:
      err_type_str = "Trace End";
      break;
    case BIFROST_VM_ERROR_NONE:
    default:
      err_type_str = "none";
      break;
  }

  std::printf("%s Error[Line %i]: %s\n", err_type_str, line_no, message);
}

static void printHandler(BifrostVM* /*vm*/, const char* message) noexcept
{
  std::printf("%s\n", message);
}

static void moduleHandler(BifrostVM* /*vm*/, const char* /*from*/, const char* module, BifrostVMModuleLookUp* out) noexcept
{
  FILE* const file      = std::fopen(module, "rb");  // NOLINT(android-cloexec-fopen)
  char*       buffer    = nullptr;
  long        file_size = 0u;

  if (file)
  {
    std::fseek(file, 0, SEEK_END);
    file_size = std::ftell(file);

    if (file_size != -1L)
    {
      std::fseek(file, 0, SEEK_SET);  //same as std::rewind(f);
      buffer = static_cast<char*>(std::malloc(sizeof(char) * (std::size_t(file_size) + 1)));

      if (buffer)
      {
        std::fread(buffer, sizeof(char), file_size, file);
        buffer[file_size] = '\0';
      }
      else
      {
        file_size = 0;
      }
    }
    else
    {
      file_size = 0;
    }

    std::fclose(file);
  }

  out->source     = buffer;
  out->source_len = file_size;
}

static void waitForInput() noexcept
{
  try
  {
    std::cin.ignore();
  }
  catch (const std::ios::failure&)
  {
  }
}
