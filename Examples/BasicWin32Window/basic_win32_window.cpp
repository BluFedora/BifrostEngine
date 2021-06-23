#ifndef UNICODE
#define UNICODE
#endif

#include "bf/bf_api_types.h"

#include <Windows.h>

#include <stdio.h>

union Pixel
{
  color4u  color;
  color32h bgra;
};

struct Bitmap
{
  BITMAPINFO bitmapInfo = {};
  Pixel*     memory     = nullptr;

  Pixel& operator()(int x, int y)
  {
    return memory[x + y * bitmapInfo.bmiHeader.biWidth];
  }

  void safeWrite(int x, int y, Pixel pix)
  {
    if (x >= 0 && x < bitmapInfo.bmiHeader.biWidth &&
        y >= 0 && y < bitmapInfo.bmiHeader.biHeight)
    {
      memory[x + y * bitmapInfo.bmiHeader.biWidth] = pix;
    }
  }
};

static Bitmap g_Bitmap = {};
static int    g_Offset = 0;

static void Bitmap_Resize(Bitmap& result, int width, int height)
{
  BITMAPINFO& bitmapInfo = result.bitmapInfo;

  bitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFO);
  bitmapInfo.bmiHeader.biWidth         = width;
  bitmapInfo.bmiHeader.biHeight        = height;
  bitmapInfo.bmiHeader.biPlanes        = 1;
  bitmapInfo.bmiHeader.biBitCount      = 32;
  bitmapInfo.bmiHeader.biCompression   = BI_RGB;
  bitmapInfo.bmiHeader.biSizeImage     = 0;
  bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
  bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
  bitmapInfo.bmiHeader.biClrUsed       = 0;
  bitmapInfo.bmiHeader.biClrImportant  = 0;
  // bitmapInfo.bmiColors[0].rgbBlue;
  // bitmapInfo.bmiColors[0].rgbGreen;
  // bitmapInfo.bmiColors[0].rgbRed;
  // bitmapInfo.bmiColors[0].rgbReserved;

  void* const old_memory = result.memory;

  result.memory = (Pixel*)realloc(old_memory, width * height * sizeof(Pixel));

  if (!result.memory)
  {
    free(old_memory);
  }
}

static void WriteToHDC(HDC hdc, rect2i screen, Bitmap* bitmap)
{
  for (int y = 0; y < bitmap->bitmapInfo.bmiHeader.biHeight; ++y)
  {
    for (int x = 0; x < bitmap->bitmapInfo.bmiHeader.biWidth; ++x)
    {
      uint8_t b = uint8_t(x + g_Offset) * 8;
      uint8_t g = uint8_t(y + g_Offset) * 8;
      uint8_t r = 255;
      uint8_t a = 0;

      (*bitmap)(x, y).bgra = color32h_makeBGRA(r, g, b, a);
    }
  }

  for (int y = 0; y < 10; ++y)
  {
    for (int x = 0; x < 10; ++x)
    {
      bitmap->safeWrite(x + 5, y + 5, Pixel{0, 0, 0, 0});
    }
  }

  StretchDIBits(
   hdc,

   // Destination
   screen.min.x,
   screen.min.y,
   screen.max.x - screen.min.x,
   screen.max.y - screen.min.y,

   // Source
   0,
   0,
   bitmap->bitmapInfo.bmiHeader.biWidth,
   bitmap->bitmapInfo.bmiHeader.biHeight,

   bitmap->memory,
   &bitmap->bitmapInfo,

   DIB_RGB_COLORS,
   SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  const wchar_t k_MainWindowClassName[] = TEXT("MainWindowClass");

  WNDCLASS windowClass;
  windowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpfnWndProc   = &WindowProc;
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = 0;
  windowClass.hInstance     = hInstance;
  windowClass.hIcon         = NULL;
  windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = NULL;
  windowClass.lpszMenuName  = NULL;
  windowClass.lpszClassName = k_MainWindowClassName;

  const ATOM classAtom = RegisterClass(&windowClass);

  if (classAtom == 0)
  {
    const DWORD errorCode = GetLastError();

    LPTSTR lpBuffer;

    const DWORD numCharacters = FormatMessage(
     FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
     NULL,
     errorCode,
     LANG_SYSTEM_DEFAULT,  // Or `LANG_USER_DEFAULT`
     (LPTSTR)&lpBuffer,
     0u,
     NULL);

    if (numCharacters != 0)
    {
      OutputDebugString(TEXT("ERROR: "));
      OutputDebugString(lpBuffer);

      LocalFree(lpBuffer);
    }
    else
    {
      const DWORD errorCodeFmtMessage = GetLastError();

      return errorCodeFmtMessage;
    }

    return 1;
  }

  void* windowUserData = NULL;

  const HWND windowHandle = CreateWindowEx(
   0u,
   k_MainWindowClassName,
   TEXT("My New Fancy Window 💇🏻‍"),
   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
   CW_USEDEFAULT,
   CW_USEDEFAULT,
   CW_USEDEFAULT,
   CW_USEDEFAULT,
   NULL,
   NULL,
   hInstance,
   windowUserData);

  if (windowHandle == NULL)
  {
    return 2;
  }

  ShowWindow(windowHandle, nCmdShow);
  UpdateWindow(windowHandle);

  bool is_running = true;

  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  float frequencyFactor = 1.0f / freq.QuadPart;

  LARGE_INTEGER lastTime;
  QueryPerformanceCounter(&lastTime);

  while (is_running)
  {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    const float dt_seconds = float(now.QuadPart - lastTime.QuadPart) * frequencyFactor;
    lastTime               = now;

    char buffer[100];
    sprintf(buffer, "DT(%f), FPS(%f)\n", dt_seconds, 1.0f / dt_seconds);
    OutputDebugStringA(buffer);

    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
    {
      if (msg.message == WM_QUIT)
      {
        is_running = false;
        break;
      }

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (GetKeyState(VK_SPACE) & (1 << 15))
    {
      g_Offset += 2;
    }

    HDC hdc = GetDC(windowHandle);

    RECT client_rect;
    GetClientRect(windowHandle, &client_rect);

    rect2i screen;
    screen.min.x = client_rect.left;
    screen.min.y = client_rect.top;
    screen.max.x = client_rect.right;
    screen.max.y = client_rect.bottom;

    WriteToHDC(hdc, screen, &g_Bitmap);

    // Sleep(8);
  }

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_SIZE:
    {
      const int width  = LOWORD(lParam);
      const int height = HIWORD(lParam);

      Bitmap_Resize(g_Bitmap, width, height);

      HDC  hdc = GetDC(hwnd);
      RECT client_rect;
      GetClientRect(hwnd, &client_rect);

      rect2i screen;
      screen.min.x = client_rect.left;
      screen.min.y = client_rect.top;
      screen.max.x = client_rect.right;
      screen.max.y = client_rect.bottom;

      WriteToHDC(hdc, screen, &g_Bitmap);

      break;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC         hdc = BeginPaint(hwnd, &ps);

      RECT client_rect;
      GetClientRect(hwnd, &client_rect);

      rect2i screen;
      screen.min.x = client_rect.left;
      screen.min.y = client_rect.top;
      screen.max.x = client_rect.right;
      screen.max.y = client_rect.bottom;

      WriteToHDC(hdc, screen, &g_Bitmap);

      EndPaint(hwnd, &ps);
      break;
    }
    case WM_CLOSE:
    {
      DestroyWindow(hwnd);

      // If I want to cancel the close return 0.

      break;
    }
    case WM_DESTROY:
    {
      PostQuitMessage(0);
      break;
    }
    default:
    {
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
  }

  return 0;
}
