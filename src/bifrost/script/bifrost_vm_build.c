// TODO(SR):
//   I need to fix the ability to compile a non unity build.

#include "bifrost_vm_parser.c"
#include "bifrost_vm_api.c"
#include "bifrost_vm_function_builder.c"
#include "bifrost_vm_lexer.c"
#include "bifrost_vm_value.c"
#include "bifrost_vm_gc.c"
#include "bifrost_vm_obj.c" // TODO(SR): This must be below GC
#include "bifrost_vm_debug.c"