#include "bifrost/platform/bifrost_platform.h"

#include <stdlib.h> /* realloc */

bfPlatformInitParams g_BifrostPlatform;

void* bfPlatformDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data)
{
  (void)user_data;
  (void)old_size;

  /*
    NOTE(Shareef):
      "if new_size is zero, the behavior is implementation defined
      (null pointer may be returned in which case the old memory block may or may not be freed),
      or some non-null pointer may be returned that may
      not be used to access storage."
  */
  if (new_size == 0u)
  {
    free(ptr);
    ptr = NULL;
  }
  else
  {
    void* const new_ptr = realloc(ptr, new_size);

    if (!new_ptr)
    {
      /*
        NOTE(Shareef):
          As to not leak memory, realloc says:
            "If there is not enough memory,
            the old memory block is not freed and null pointer is returned."
      */
      free(ptr);
    }

    ptr = new_ptr;
  }

  return ptr;
}

void* bfPlatformAlloc(size_t size)
{
  return g_BifrostPlatform.allocator(NULL, 0u, size, g_BifrostPlatform.user_data);
}

void* bfPlatformRealloc(void* ptr, size_t old_size, size_t new_size)
{
  return g_BifrostPlatform.allocator(ptr, old_size, new_size, g_BifrostPlatform.user_data);
}

void bfPlatformFree(void* ptr, size_t old_size)
{
  (void)g_BifrostPlatform.allocator(ptr, old_size, 0u, g_BifrostPlatform.user_data);
}

void bfPlatformDoMainLoop(BifrostWindow* main_window)
{
  while (!bfWindow_wantsToClose(main_window))
  {
    bfPlatformPumpEvents();

    if (main_window->frame_fn)
    {
      main_window->frame_fn(main_window);
    }
  }
}
