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
#include "bifrost/script/bifrost_vm.hpp"

#include <cstdio>    // printf, fopen, flocse, ftell, fseek, fread, malloc
#include <iostream>  // cin

struct MemoryUsageTracker final
{
  std::size_t peak_usage;
  std::size_t current_usage;
};

static void  errorHandler(BifrostVM* vm, BifrostVMError err, int line_no, const char* message) noexcept;
static void  printHandler(BifrostVM* vm, const char* message) noexcept;
static void  moduleHandler(BifrostVM* vm, const char* from, const char* module, BifrostVMModuleLookUp* out) noexcept;
static void* memoryHandler(void* user_data, void* ptr, size_t old_size, size_t new_size) noexcept;
static void  waitForInput() noexcept;

int main(int argc, const char* argv[])
{
#ifndef __EMSCRIPTEN__
  if (argc != 2)
  {
    std::printf("There is an example script loaded at 'assets/scripts/test_script.bscript'\n");
    std::printf("usage %s <file-name>\n", argv[0]);
    waitForInput();
    return 0;
  }

  const char* const file_name = argv[1];
#else
  const char* const file_name = "test_script.bscript";
#endif

  MemoryUsageTracker mem_tracker{0, 0};

  bifrost::VMParams params{};
  params.error_fn  = &errorHandler;
  params.print_fn  = &printHandler;
  params.module_fn = &moduleHandler;
  params.memory_fn = &memoryHandler;
  params.user_data = &mem_tracker;

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
    waitForInput();
    return err;
  }

  memoryHandler(bfVM_userData(vm), const_cast<char*>(load_file.source), sizeof(char) * (load_file.source_len + 1u), 0u);

  std::printf("Memory Stats:\n");
  std::printf("\tPeak    Usage: %u (bytes)\n", unsigned(mem_tracker.peak_usage));
  std::printf("\tCurrent Usage: %u (bytes)\n", unsigned(mem_tracker.current_usage));

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

static void moduleHandler(BifrostVM* vm, const char* /*from*/, const char* module, BifrostVMModuleLookUp* out) noexcept
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
      std::fseek(file, 0, SEEK_SET);  //same as std::rewind(file);
      buffer = static_cast<char*>(memoryHandler(bfVM_userData(vm), nullptr, 0u, sizeof(char) * (std::size_t(file_size) + 1)));

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

static void* memoryHandler(void* user_data, void* ptr, size_t old_size, size_t new_size) noexcept
{
  //
  // These checks are largely redundant since it just reimplements
  // what 'realloc' already does, this is mostly for demonstrative
  // purposes on how to write your own memory allocator function.
  //

  MemoryUsageTracker* const mum_tracker = static_cast<MemoryUsageTracker*>(user_data);

  mum_tracker->current_usage -= old_size;
  mum_tracker->current_usage += new_size;

  if (mum_tracker->current_usage > mum_tracker->peak_usage)
  {
    mum_tracker->peak_usage = mum_tracker->current_usage;
  }

  if (old_size == 0u || ptr == nullptr)  // Both checks are not needed but just for illustration of both ways of checking for new allocation.
  {
    return new_size != 0 ? std::malloc(new_size) : nullptr;  // Returning nullptr for a new_size of 0 is not strictly required.
  }

  if (new_size == 0u)
  {
    std::free(ptr);
  }
  else
  {
    return std::realloc(ptr, new_size);
  }

  return nullptr;
}

static void waitForInput() noexcept
{
#ifndef __EMSCRIPTEN__
  try
  {
    std::cin.ignore();
  }
  catch (const std::ios::failure&)
  {
  }
#endif
}
