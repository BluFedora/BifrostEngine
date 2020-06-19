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
#ifndef BIFROST_IECS_SYSTEM_HPP
#define BIFROST_IECS_SYSTEM_HPP

class Engine;

namespace bifrost
{
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
    virtual void onFrameBegin(Engine& engine, float dt)   { (void)engine; (void)dt;    }
    virtual void onFrameUpdate(Engine& engine, float dt)  { (void)engine; (void)dt;    }
    virtual void onFrameEnd(Engine& engine, float dt)     { (void)engine; (void)dt;    }
    virtual void onFrameDraw(Engine& engine, float alpha) { (void)engine; (void)alpha; }
    // clang-format on

   public:
    virtual ~IECSSystem() = default;
  };
}  // namespace bifrost

#endif /* BIFROST_IECS_SYSTEM_HPP */
