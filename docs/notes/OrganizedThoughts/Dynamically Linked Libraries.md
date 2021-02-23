# DLLs

dlls are reference counted.

`cl /c MySource.cpp`  Generates Obj File (MySource.obj)
`link MySource.obj /DLL /NOENTRY /EXPORT:FunctionName /EXPORT:RenamedFunctionName=FancyFunctionName /EXPORT:OnlyUseWithGetProcAddress,PRIVATE` (Generates The Library (MySource.dll And MySource.lib) file with the specified exported symbol)

Whatever.def (use with `/DEF:Whatever.def`)
```  
LIBRARY liName
EXPORTS
  FunctionName
  RenamedFunctionName=FancyFunctionName
  OnlyUseWithGetProcAddress PRIVATE
```

```
__declspec(dllexport)
__declspec(dllimport)

or

#pragma comment(linker, "/export:FunctionName")
```

Runtime (These are defined in KERNEL32.dll):
  LoadLibraryEx
  GetProcAddress
  FreeLibrary

References:
  (Everything You Ever Wanted to Know about DLLs)[https://www.youtube.com/watch?v=JPQWQfDhICA&ab_channel=CppCon]
