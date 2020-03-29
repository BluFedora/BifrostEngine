
rm imgui.vert.spv
rm imgui.frag.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V imgui.vert.glsl -o imgui.vert.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V imgui.frag.glsl -o imgui.frag.spv

del shaders\standard\compiled\*
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V shaders/standard/processed/gbuffer.frag.glsl -o shaders/standard/compiled/gbuffer.frag.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V shaders/standard/processed/gbuffer.vert.glsl -o shaders/standard/compiled/gbuffer.vert.spv
rem D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V shaders/standard/proccessed/normal_encode.glsl -o shaders/standard/compiled/normal_encode.spv
rem D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V shaders/standard/proccessed/std_vertex_input.def.glsl -o shaders/standard/compiled/std_vertex_input.def.spv

pause
cls
 