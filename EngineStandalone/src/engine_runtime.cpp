#define WIN32_LEAN_AND_MEAN

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
 
#include <d3dx12.h> // D3D12 extension library.

#include <wrl.h>
using namespace Microsoft::WRL;

// Dx12 Notes:
/*
  When using runtime compiled HLSL shaders using any of the D3DCompiler functions, do not forget to link against the d3dcompiler.lib library and copy the D3dcompiler_47.dll to the same folder as the binary executable when distributing your project.
  A redistributable version of the D3dcompiler_47.dll file can be found in the Windows 10 SDK installation folder at C:\Program Files (x86)\Windows Kits\10\Redist\D3D\.
*/

extern "C" int main(int argc, char* argv[])
{
  return 0;
}
