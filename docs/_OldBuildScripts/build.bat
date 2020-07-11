del bin\vm.exe
del build\gcc\bifrost_vm_build.o
del build\gcc\bifrost_array_t.o
del build\gcc\bifrost_dynamic_string.o
del build\gcc\bifrost_hash_map.o
del build\gcc\main.o
del build\gcc\main_cpp.o

rem gcc --version
rem bin\vm.exe fake_file.bts
rem bin\vm.exe
rem -Wuseless-cast^ This is for C++/ObjC++
rem cls

set CC_OPTIONS=-I../bifrost-core/bifrost_core^
 -Iinclude^
 -O0^
 -ggdb^
 -Wnonnull^
 -std=c99^
 -pedantic^
 -Wall^
 -Wextra^
 -Werror^
 -Wduplicated-cond^
 -Wduplicated-branches^
 -Wlogical-op^
 -Wnull-dereference^
 -Wjump-misses-init^
 -Wdouble-promotion^
 -Wshadow^
 -Winline^
 -c

set CPPC_OPTIONS=-I../bifrost-core/bifrost_core^
 -Iinclude^
 -O0^
 -ggdb^
 -Wnonnull^
 -std=c++17^
 -pedantic^
 -Wall^
 -Wextra^
 -Werror^
 -Wduplicated-cond^
 -Wduplicated-branches^
 -Wlogical-op^
 -Wnull-dereference^
 -Wdouble-promotion^
 -Wshadow^
 -Winline^
 -c

rem g++ %CPPC_OPTIONS% main.cpp -o build/gcc/main_cpp.o || pause

gcc^
 %CC_OPTIONS%^
 main.c^
 -o build/gcc/main.o || pause

 gcc^
 %CC_OPTIONS%^
 src/bifrost/script/bifrost_vm_build.c^
 -o build/gcc/bifrost_vm_build.o || pause

gcc^
 %CC_OPTIONS%^
 src/bifrost/data_structures/bifrost_array_t.c^
 -o build/gcc/bifrost_array_t.o || pause

 gcc^
 %CC_OPTIONS%^
 src/bifrost/data_structures/bifrost_dynamic_string.c^
 -o build/gcc/bifrost_dynamic_string.o || pause

 gcc^
 %CC_OPTIONS%^
 src/bifrost/data_structures/bifrost_hash_map.c^
 -o build/gcc/bifrost_hash_map.o || pause

pushd build
pushd gcc
gcc main.o bifrost_vm_build.o bifrost_array_t.o bifrost_dynamic_string.o bifrost_hash_map.o -o ../../bin/vm.exe || pause
rem g++ main_cpp.o bifrost_vm_build.o bifrost_array_t.o bifrost_dynamic_string.o bifrost_hash_map.o -o ../../bin/vm.exe || pause
popd
popd

bin\vm.exe test_script.bts
rem bin\vm.exe assets/scripts/test_module.bts
