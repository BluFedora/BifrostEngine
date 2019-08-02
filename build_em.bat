rem emcc -I../bifrost-core/bifrost_core --preload-file test_script.bts main.c -o vm.html
rem https://lyceum-allotments.github.io/2016/06/emscripten-and-sdl2-tutorial-part-5-move-owl/
rem emcc --show-ports
emcc^
  --js-library lib/bifrost_webgl.js^
  --preload-file test_script.bts^
  --preload-file assets^
  -s "EXPORTED_FUNCTIONS=['_main', '_updateFromWebGL', '_getContext']"^
  -s^
  USE_WEBGL2=1^
  -s FULL_ES3=1^
  -s WASM=0^
  -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR^
  -s ASSERTIONS=2^
  -I../bifrost-core/bifrost_core^
  -Iinclude^
  main.c^
  src/bifrost/script/bifrost_vm_build.c^
  ../bifrost-core/bifrost_core/bifrost/data_structures/bifrost_array_t.c^
  ../bifrost-core/bifrost_core/bifrost/data_structures/bifrost_dynamic_string.c^
  ../bifrost-core/bifrost_core/bifrost/data_structures/bifrost_hash_map.c^
  -DBIFROST_MALLOC(size,align)="malloc((size))"^
  -DBIFROST_REALLOC(ptr,new_size,align)="realloc((ptr), new_size)"^
  -DBIFROST_FREE(ptr)="free((ptr))"^
  src/bifrost/render/opengl/bifrost_opengl_es3.cpp^
  src/bifrost/render/bifrost_shader_common.c^
  -o bin/vm_js.js
