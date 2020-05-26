/*!
 * @file   bifrost_vm_build.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief   
 *   This is the 'unity build' file that gets compiled for this library.
 *   TODO(SR): I need to fix the ability to compile a non unity build.
 *
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */

#include "bifrost_vm_parser.c"

#include "bifrost_vm_api.c"

#include "bifrost_vm_function_builder.c"

#include "bifrost_vm_lexer.c"

#include "bifrost_vm_value.c"

#include "bifrost_vm_gc.c"

#include "bifrost_vm_obj.c"  // TODO(SR): This must go under "bifrost_vm_gc.c".

#include "bifrost_vm_debug.c"
