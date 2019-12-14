rm basic_material.vert.spv
rm basic_material.frag.spv
rm imgui.vert.spv
rm imgui.frag.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V basic_material.vert.glsl -o basic_material.vert.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V basic_material.frag.glsl -o basic_material.frag.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V imgui.vert.glsl -o imgui.vert.spv
D:/Vulkan/1.1.106.0/Bin32/glslangValidator.exe -V imgui.frag.glsl -o imgui.frag.spv
pause
cls
 