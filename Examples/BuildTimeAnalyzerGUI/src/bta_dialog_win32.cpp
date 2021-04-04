#include "bta_dialog.hpp"

#include <memory> /* unique_ptr */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <shobjidl.h>
#include <windows.h>

namespace bta
{
  bool dialogInit()
  {
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    return SUCCEEDED(hr);
  }

  static bool dialogOpenX(std::string &out, FILEOPENDIALOGOPTIONS options)
  {
    bool             result = false;
    IFileOpenDialog *ifile_open_dlog;

    const HRESULT hr_create_dialog = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&ifile_open_dlog));

    if (SUCCEEDED(hr_create_dialog))
    {
      if (options)
      {
        ifile_open_dlog->SetOptions(options);
      }

      const HRESULT hr_show_dialog = ifile_open_dlog->Show(nullptr);

      if (SUCCEEDED(hr_show_dialog))
      {
        IShellItem *  result_item;
        const HRESULT hr_grab_result = ifile_open_dlog->GetResult(&result_item);

        if (SUCCEEDED(hr_grab_result))
        {
          PWSTR         file_path;
          const HRESULT hr_get_name = result_item->GetDisplayName(SIGDN_FILESYSPATH, &file_path);

          if (SUCCEEDED(hr_get_name))
          {
            //
            // This code converts from WChar to Normal ASCII Chars.
            // Maybe Wide chars are the way to go but my knowledge on text encoding is
            // much too limited.
            // - Shareef R.
            //

            const std::size_t             file_path_length     = std::wcslen(file_path);
            const std::size_t             file_path_capacity   = file_path_length + 1;
            const std::size_t             temp_buffer_capacity = file_path_capacity * 2;
            const std::unique_ptr<char[]> temp_buffer          = std::make_unique<char[]>(temp_buffer_capacity);
            char *const                   temp_buffer_ptr      = temp_buffer.get();

            std::wcstombs(temp_buffer_ptr, file_path, temp_buffer_capacity);
            CoTaskMemFree(file_path);

            out = temp_buffer_ptr;

            result = true;
          }

          result_item->Release();
        }
      }

      ifile_open_dlog->Release();
    }

    return result;
  }

  bool dialogOpenFolder(std::string &out)
  {
    return dialogOpenX(out, FOS_NOCHANGEDIR | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_DONTADDTORECENT | FOS_FORCESHOWHIDDEN);
  }

  bool dialogOpenFile(std::string &out)
  {
    return dialogOpenX(out, FOS_NOCHANGEDIR | FOS_DONTADDTORECENT);
  }

  void dialogQuit()
  {
    CoUninitialize();
  }
}  // namespace bta
