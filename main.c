#include "bifrost/bifrost_vm.h" /* All of the VM */
// TODO: REMOVE.
#include "src/bifrost/script/bifrost_vm_debug.h"
// TODO: REMOVE
#include "src/bifrost/script/bifrost_vm_obj.h"

#include <assert.h> /* assert                    */
#include <bifrost/data_structures/bifrost_array_t.h>
#include <stdio.h>   /* printf                    */
#include <stdlib.h>  /* Malloc, Free              */
#include <string.h>  // strlen

static char* load_from_file(const char* const filename, long* out_size)
{
  FILE* f      = fopen(filename, "rb");  // NOLINT(android-cloexec-fopen)
  char* buffer = NULL;

  if (f)
  {
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    buffer = malloc(file_size + 1);
    fread(buffer, sizeof(char), file_size, f);
    fclose(f);

    buffer[file_size] = '\0';

    *out_size = file_size;
  }

  return buffer;
}

static size_t     g_NumAllocations = 0u;
static size_t     g_NumFrees       = 0u;
static bfMemoryFn g_OldAlloc       = NULL;

static void  userErrorFn(BifrostVM* vm, BifrostVMError err, int line_no, const char* message);
static void  userPrintFn(BifrostVM* vm, const char* message);
static void  userModuleFn(BifrostVM* vm, const char* from, const char* module, BifrostVMModuleLookUp* out);
static void* userMemoryFn(void* user_data, void* ptr, size_t old_size, size_t size, size_t alignment);
static void  usage(const char* program_name);

// TODO(SR): REMOVE ME
static void nativeFunctionTest(BifrostVM* vm, int32_t num_args)
{
  assert(num_args == 2);

  const bfVMNumberT num0 = bfVM_stackReadNumber(vm, 0);
  const bfVMNumberT num1 = bfVM_stackReadNumber(vm, 1);

  bfVM_stackResize(vm, 2);
  bfVM_moduleLoad(vm, 0, "main");
  bfVM_stackLoadVariable(vm, 0, 0, "fibbonacci");
  bfVM_stackSetNumber(vm, 1, num0);

  bfVM_call(vm, 0, 1, 1);

  bfVM_stackSetNumber(vm, 0, bfVM_stackReadNumber(vm, 1) * num1);
}

// TODO(SR): REMOVE ME
static void nativeFunctionMathPrint(BifrostVM* vm, int32_t num_args)
{
  assert(num_args == 0);
  (void)vm;
  printf("This is from the math module\n");
}

// TODO(SR): REMOVE ME
static void userClassMath_mult(BifrostVM* vm, int32_t num_args)
{
  (void)num_args;

  const bfVMNumberT num0 = bfVM_stackReadNumber(vm, 0);
  const bfVMNumberT num1 = bfVM_stackReadNumber(vm, 1);

  bfVM_stackSetNumber(vm, 0, num0 * num1);
}

#if __EMSCRIPTEN__
#include <bifrost/platform/bifrost_platform_gl.h>
// TODO(SR): The API should define what header to include.
// On desktop we can use GLAD for Win, Mac, Nix.
#include "bifrost/render/bifrost_video_api.h"

#include <emscripten.h>        // EM_ASM
#include <emscripten/html5.h>  //

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE s_GlCtx;

void updateFromWebGL()
{
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

extern void basicDrawing(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE);

#endif

#define BIFROST_VM_MODULE_MEMORY 1

#if BIFROST_VM_MODULE_MEMORY

void bfRegisterModuleMemory(BifrostVM* vm);

#endif

int main(int argc, const char* argv[])
{
#if __EMSCRIPTEN__
  EmscriptenWebGLContextAttributes ctx_attribs;
  emscripten_webgl_init_context_attributes(&ctx_attribs);
  ctx_attribs.alpha                        = bfFalse;
  ctx_attribs.depth                        = bfTrue;
  ctx_attribs.stencil                      = bfTrue;
  ctx_attribs.antialias                    = bfTrue;
  ctx_attribs.premultipliedAlpha           = bfTrue;
  ctx_attribs.preserveDrawingBuffer        = bfFalse;
  ctx_attribs.powerPreference              = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;  // EM_WEBGL_POWER_PREFERENCE_LOW_POWER or EM_WEBGL_POWER_PREFERENCE_DEFAULT
  ctx_attribs.failIfMajorPerformanceCaveat = bfFalse;
  ctx_attribs.majorVersion                 = 2;
  ctx_attribs.minorVersion                 = 0;
  ctx_attribs.enableExtensionsByDefault    = bfTrue;
  ctx_attribs.explicitSwapControl          = bfFalse;
  ctx_attribs.renderViaOffscreenBackBuffer = bfFalse;
  // ctx_attribs.proxyContextToMainThread = bfFalse; // default

  s_GlCtx = emscripten_webgl_create_context("#kanvas", &ctx_attribs);

  if (!s_GlCtx)
  {
    printf("Failed to create context\n");
    return -5;
  }

  emscripten_webgl_make_context_current(s_GlCtx);

  basicDrawing(s_GlCtx);

  argc    = 2;
  argv[1] = "test_script.bts";

  const ShaderProgramCreateParams shader_params =
   {
    .parent = NULL,
   };

  bfShaderProgram shader = BifrostShaderProgram_new(&shader_params);
  BifrostShaderProgram_loadFile(shader, BST_VERTEX, "assets/shaders/gles3/basic.vs.glsl");
  BifrostShaderProgram_loadFile(shader, BST_FRAGMENT, "assets/shaders/gles3/basic.fs.glsl");
  BifrostShaderprogram_compile(shader);
  // const int positionAttributeLocation = glGetAttribLocation(shader);

  BifrostShaderProgram_delete(shader);
  //EM_ASM(alert("Inline Javascript from C++!"););
#endif

  if (argc == 1)
  {
    usage(argv[0]);
  }
  else if (argc >= 2)
  {
    /*
      Configuration
    */
    BifrostVMParams vm_params;
    bfVMParams_init(&vm_params);
    vm_params.error_fn           = &userErrorFn;
    vm_params.print_fn           = &userPrintFn;
    vm_params.module_fn          = &userModuleFn;
    g_OldAlloc                   = vm_params.memory_fn;
    vm_params.memory_fn          = &userMemoryFn;
    vm_params.min_heap_size      = 200;
    vm_params.heap_size          = 500;
    vm_params.heap_growth_factor = 0.3f;

    /*
      Initialization
    */
    BifrostVM* vm = bfVM_new(&vm_params);

    bfVM_stackResize(vm, 1);
    bfVM_moduleMake(vm, 0, "std:math");
    bfVM_moduleBindNativeFn(vm, 0, "math_print", &nativeFunctionMathPrint, 0);

    bfRegisterModuleMemory(vm);

    bfVM_moduleMake(vm, 0, "std:array");
    /*
    typedef struct
    {
      bfVMValue* data;

    } StdArray;

    static const BifrostMethodBind s_ArrayClassMethods[] = {
     {"ctor", &userClassFile_ctor, -1},
     {"size", &userClassFile_print, -1},
     {"capacity", &userClassFile_print, -1},
     {"push", &userClassFile_print, -1},
     {"insert", &userClassFile_close, -1},
     {"removeAt", &userClassFile_close, -1},
     {"pop", &userClassFile_close, -1},
     {"concat", &userClassFile_close, -1},
     {"clone", &userClassFile_close, -1},
     {"copy", &userClassFile_close, -1},
     {"resize", &userClassFile_close, -1},
     {"reserve", &userClassFile_close, -1},
     {"sortedFindFrom", &userClassFile_close, -1},
     {"sortedFind", &userClassFile_close, -1},
     {"findFrom", &userClassFile_close, -1},
     {"find", &userClassFile_close, -1},
     {"[]", &userClassFile_close, -1},
     {"[]=", &userClassFile_close, -1},
     {"sort", &userClassFile_close, -1},
     {"clear", &userClassFile_close, -1},
     {"front", &userClassFile_close, -1},
     {"back", &userClassFile_close, -1},
     {"dtor", &userClassFile_close, -1},
     {NULL, NULL, 0},
    };

    BifrostVMClassBind array_clz = {
     .name            = "Array",
     .extra_data_size = sizeof(StdArray),
     .methods         = s_ArrayClassMethods};

    bfVM_moduleBindClass(vm, 0, &array_clz);
 */
    /*
      Running Code
    */
    long        source_size;
    char* const source = load_from_file(argv[1], &source_size);

    if (source && source_size)
    {
      const BifrostVMError err = bfVM_execInModule(vm, "main", source, (size_t)source_size);
      free(source);

      if (err != BIFROST_VM_ERROR_NONE)
      {
        printf("ERROR FROM MAIN!\n");
      }
      else
      {
        printf("### Calling GC ###\n");
        bfVM_gc(vm);
        printf("### GC Done    ###\n");

        bfVM_stackResize(vm, 2);
        bfVM_moduleLoad(vm, 0, "main");
        bfVM_stackLoadVariable(vm, 0, 0, "fibbonacci");
        bfVM_stackSetNumber(vm, 1, 9.0);

        bfVM_call(vm, 0, 1, 1);

        bfVMNumberT result = bfVM_stackReadNumber(vm, 1);

        printf("VM Result0: %f\n", result);

        bfVM_stackResize(vm, 3);
        bfVM_moduleLoad(vm, 0, "main");
        bfVM_moduleBindNativeFn(vm, 0, "facAndMult", &nativeFunctionTest, 2);
        bfVM_stackLoadVariable(vm, 0, 0, "facAndMult");
        bfVM_stackSetNumber(vm, 1, 9.0);
        bfVM_stackSetNumber(vm, 2, 3.0);

        bfVM_call(vm, 0, 1, 2);

        result = bfVM_stackReadNumber(vm, 1);

        printf("VM Result1: %f\n", result);

        printf("<--- Class Registration: --->\n");

        bfVM_stackResize(vm, 1);
        bfVM_moduleLoad(vm, 0, "main");

        static const BifrostMethodBind s_TestClassMethods[] = {
         {"mult", &userClassMath_mult, 2},
         {NULL, NULL, 0},
        };

        BifrostVMClassBind test_clz = {
         .name            = "Math",
         .extra_data_size = 0u,
         .methods         = s_TestClassMethods};

        bfVM_moduleBindClass(vm, 0, &test_clz);

        printf("-----------------------------\n");

        bfVM_stackLoadVariable(vm, 0, 0, "testNative");

        if (bfVM_call(vm, 0, 0, 0))
        {
          printf("There was an error running 'testNative'\n");
        }
      }
    }
    else
    {
      printf("Could not load file %s\n", argv[1]);
    }

    /*
      Destruction
    */
    bfVM_delete(vm);
  }
  else
  {
    printf("Invalid number of arguments passed (%i)\n", argc);
    return -1;
  }

  printf("----------------------------------------\n");
  printf("VM Memory Stats:\n\t %zu allocations\n\t %zu frees\n", g_NumAllocations, g_NumFrees);
  printf("----------------------------------------\n");

  printf("Returned from main.\n");
  return 0;
}

static void usage(const char* program_name)
{
  printf("usage: %s <filename>\n", program_name);
}

static void userErrorFn(struct BifrostVM_t* vm, BifrostVMError err, int line_no, const char* message)
{
  (void)vm;
  (void)line_no;
  if (err == BIFROST_VM_ERROR_STACK_TRACE_BEGIN || err == BIFROST_VM_ERROR_STACK_TRACE_END)
  {
    printf("### ------------ ERROR ------------ ###\n");
  }
  else
  {
    printf("%s", message);
  }
}

static void userPrintFn(BifrostVM* vm, const char* message)
{
  (void)vm;
  printf("%s\n", message);
}

static void userModuleFn(BifrostVM* vm, const char* from, const char* module, BifrostVMModuleLookUp* out)
{
  (void)vm;
  (void)from;

  long  file_size = 0;
  char* src       = load_from_file(module, &file_size);

  if (src && file_size)
  {
    out->source     = src;
    out->source_len = (size_t)file_size;
    ++g_NumAllocations;
  }
}

static void* userMemoryFn(void* user_data, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
  if (old_size == 0u && new_size != 0u)
  {
    ++g_NumAllocations;
  }
  else if (old_size != 0u && new_size == 0u)
  {
    ++g_NumFrees;
  }

  return g_OldAlloc(user_data, ptr, old_size, new_size, alignment);
}

/*
void DEBUG_DUMP_VARIABLES(const void* vars, uint32_t size, uint32_t indent)
{
  typedef struct
  {
    const char* name;
    uint64_t    value;

  } DEBUG_SYMBOL;

  const DEBUG_SYMBOL* symbols = vars;

  for (uint32_t i = 0; i < size; ++i)
  {
    ConstBifrostString name  = symbols[i].name;
    bfVMValue          value = symbols[i].value;

    for (uint32_t j = 0; j < indent; ++j)
    {
      printf("  ");
    }

    if (value != VAL_NULL)
    {
      printf("DEBUG_DUMP[%u, %.*s] = ", i, (int)String_length(name), name);
    }
    else
    {
      printf("DEBUG_DUMP[%u, %s] = ", i, name);
    }

    if (IS_BOOL(value))
    {
      printf("BOOL<%s>", value == VAL_TRUE ? "true" : "false");
    }

    if (IS_NUMBER(value))
    {
      printf("NUMBER<%f>", bfVmValue_asNumber(value));
    }

    if (IS_POINTER(value))
    {
      BifrostObj* obj = AS_POINTER(value);

      printf("OBJECT<");

      switch (obj->type)
      {
        case BIFROST_VM_OBJ_MODULE: printf("module"); break;
        case BIFROST_VM_OBJ_CLASS:
        {
          printf("class\n");
          BifrostObjClass* clz = (BifrostObjClass*)obj;
          printf("#field_initializers : %zu\n", Array_size(&clz->field_initializers));
          DEBUG_DUMP_VARIABLES(clz->field_initializers, Array_size(&clz->field_initializers), indent + 1);
          printf("#symbols : %zu\n", Array_size(&clz->symbols));
          DEBUG_DUMP_VARIABLES(clz->symbols, Array_size(&clz->symbols), indent + 1);

          for (uint32_t j = 0; j < indent; ++j)
          {
            printf("  ");
          }
          break;
        }
        case BIFROST_VM_OBJ_INSTANCE: printf("instance"); break;
        case BIFROST_VM_OBJ_FUNCTION:
        {
          printf("function\n");

          BifrostObjFn* fn = (BifrostObjFn*)obj;

          for (uint32_t j = 0; j < indent; ++j)
          {
            printf("  ");
          }

          printf("stack size = %u\n", (unsigned)fn->needed_stack_space);

          for (uint32_t j = 0; j < indent; ++j)
          {
            printf("  ");
          }

          const size_t num_k = Array_size(&fn->constants);
          printf("Konstants: (%zu)\n", num_k);

          for (size_t k = 0; k < num_k; ++k)
          {
            bfVMValue k_value = fn->constants[k];

            for (uint32_t j = 0; j < indent + 1; ++j)
            {
              printf("  ");
            }

            char buffer[255];
            bfDbgValueToString(k_value, buffer, sizeof(buffer));
            printf("[%u] = %s\n", i, buffer);
          }

          bfDbgDisassembleInstructions(indent + 1, fn->instructions, Array_size(&fn->instructions));

          for (uint32_t j = 0; j < indent; ++j)
          {
            printf("  ");
          }
          break;
        }
        case BIFROST_VM_OBJ_NATIVE_FN: printf("native_fn"); break;
        case BIFROST_VM_OBJ_STRING:
        {
          BifrostObjStr* str = (BifrostObjStr*)obj;

          printf("string\n");

          for (uint32_t j = 0; j < indent + 1; ++j)
          {
            printf("  ");
          }
          printf("value(%.*s)(%zu)\n", (int)String_length(str->value), str->value, String_length(str->value));

          for (uint32_t j = 0; j < indent; ++j)
          {
            printf("  ");
          }
          break;
        }
      }

      printf(">");
    }

    if (value == VAL_NULL)
    {
      printf("NULL<>");
    }

    printf("\n");
  }
}
*/

typedef struct
{
  bfValueHandle fn; /*!< Function to call. */

} bfClosure;

void bfCoreClosure_ctor(BifrostVM* vm, int32_t num_args)
{
  assert(num_args == 2);

  bfClosure* const self = bfVM_stackReadInstance(vm, 0);
  self->fn              = bfVM_stackMakeHandle(vm, 1);
}

void bfCoreClosure_call(BifrostVM* vm, int32_t num_args)
{
  bfClosure* const self = bfVM_stackReadInstance(vm, 0);

  const size_t arity = bfVM_handleGetArity(self->fn);
  bfVM_stackResize(vm, arity + 1);
  bfVM_stackLoadHandle(vm, arity, self->fn);

  bfVM_call(vm, arity, 0, num_args);
}

static void bfCoreClosure_finalizer(BifrostVM* vm, void* instance)
{
  const bfClosure* const self = instance;
  bfVM_stackDestroyHandle(vm, self->fn);
}

void bfCoreMemory_gc(BifrostVM* vm, int32_t num_args)
{
  assert(num_args == 0);
  bfVM_gc(vm);
}

void bfRegisterModuleMemory(BifrostVM* vm)
{
  bfVM_stackResize(vm, 1);
  bfVM_moduleMake(vm, 0, "std:memory");
  bfVM_moduleBindNativeFn(vm, 0, "gc", &bfCoreMemory_gc, 0);

  bfVM_moduleMake(vm, 0, "std:functional");

  static const BifrostMethodBind s_ClosureClassMethods[] =
   {
    {"ctor", &bfCoreClosure_ctor, 2},
    {"call", &bfCoreClosure_call, -1},
    {NULL, NULL, 0},
   };

  BifrostVMClassBind closure_clz =
   {
    .name            = "Closure",
    .extra_data_size = sizeof(bfClosure),
    .methods         = s_ClosureClassMethods,
    .finalizer       = bfCoreClosure_finalizer,
   };

  bfVM_moduleBindClass(vm, 0, &closure_clz);
}
