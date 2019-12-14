#ifndef BIFROST_WINDOW_GLFW_HPP
#define BIFROST_WINDOW_GLFW_HPP

#include "bifrost_ibase_window.hpp"

#include <glfw/glfw3.h>

namespace bifrost
{
  class WindowGLFW : public IBaseWindow
  {
   private:
    GLFWwindow* m_WindowHandle;

   public:
    WindowGLFW();
    bool open(const char* title, int width = 1280, int height = 720) override;
    void close() override;
    bool wantsToClose() override;

    [[nodiscard]] GLFWwindow* handle() const { return m_WindowHandle; }

    ~WindowGLFW() override;
  };

  namespace glfw
  {
    using ControllerEventCallback = FunctionView<void(const ControllerEvent&)>;
    using ErrorCallback = FunctionView<void(int error_code, const char* message)>;
  }

  bool startupGLFW(glfw::ControllerEventCallback* onControllerEvent, glfw::ErrorCallback* onGLFWError);
  void shutdownGLFW();
}  // namespace bifrost

#endif /* BIFROST_WINDOW_GLFW_HPP */
