/******************************************************************************/
/*!
* @file   bifrost_iecs_system.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-XX-XX
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BF_IECS_SYSTEM_HPP
#define BF_IECS_SYSTEM_HPP

class Engine;

namespace bf
{
  struct CameraRender;

  class IECSSystem
  {
    friend class Engine;

   private:
    bool m_IsEnabled;

   public:
    bool isEnabled() const { return m_IsEnabled; }

   protected:
    IECSSystem();

  public: // TODO: This isn't right...
    // clang-format off
    virtual void onInit(Engine& engine) { (void)engine; }
    virtual void onFrameBegin(Engine& engine, float dt)   { (void)engine; (void)dt;    }
    virtual void onFrameUpdate(Engine& engine, float dt)  { (void)engine; (void)dt;    }
    virtual void onFrameEnd(Engine& engine, float dt)     { (void)engine; (void)dt;    }
    virtual void onFrameDraw(Engine& engine, CameraRender& camera, float alpha) { (void)engine; (void)camera; (void)alpha; }
    virtual void onDeinit(Engine& engine) { (void)engine; }
    // clang-format on

   public:
    virtual ~IECSSystem() = default;
  };
}  // namespace bifrost

#endif /* BF_IECS_SYSTEM_HPP */
