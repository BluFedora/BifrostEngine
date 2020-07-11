rem emcc -I../bifrost-core/bifrost_core --preload-file test_script.bts main.c -o vm.html
rem https://lyceum-allotments.github.io/2016/06/emscripten-and-sdl2-tutorial-part-5-move-owl/
rem emcc --show-ports

rem  --preload-file test_script.bts^
rem  --preload-file ../assets/scripts/test_script.bscript^

call emcc^
  --js-library ../lib/bifrost_webgl.js^
  -s "EXPORTED_FUNCTIONS=['_main']"^
  -s USE_WEBGL2=1^
  -s FULL_ES3=1^
  -s WASM=0^
  -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR^
  -s ASSERTIONS=2^
  -I../include^
  -Iinclude^
  src/bifrost_vm_build.c^
  ../src/bifrost/data_structures/bifrost_array_t.c^
  ../src/bifrost/data_structures/bifrost_dynamic_string.c^
  ../src/bifrost/data_structures/bifrost_hash_map.c^
  -o main_bitcode.bc

call em++^
  -std=c++17^
  --js-library ../lib/bifrost_webgl.js^
  -s "EXPORTED_FUNCTIONS=['_main']"^
  --preload-file test_script.bscript^
  -s USE_WEBGL2=1^
  -s FULL_ES3=1^
  -s WASM=0^
  -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR^
  -s ASSERTIONS=2^
  -I../include^
  -Iinclude^
  vm_command_line.cpp^
  main_bitcode.bc^
  -o vm_js.html

del main_bitcode.bc
