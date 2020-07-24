/******************************************************************************/
/*!
* @file   bifrost_component_renderer.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-07-10
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_COMPONENT_RENDERER_HPP
#define BIFROST_COMPONENT_RENDERER_HPP

#include "bifrost/ecs/bifrost_iecs_system.hpp" /* IECSSystem                            */
#include "bifrost_standard_renderer.hpp"       /* TransientVertexBuffer, StandardVertex */

namespace bifrost
{
  class ComponentRenderer final : public IECSSystem
  {
    using VertexBuffer = TransientVertexBuffer<StandardVertex, 1024>;

   private:
    bfShaderModuleHandle  m_ShaderModules[2]   = {};       //!< [Sprite-Vertex, Sprite-Fragment]
    bfShaderProgramHandle m_ShaderProgram      = nullptr;  //!< Sprite Program
    VertexBuffer*         m_SpriteVertexBuffer = nullptr;

   public:
    void onInit(Engine& engine) override;
    void onFrameDraw(Engine& engine, CameraRender& camera, float alpha) override;
    void onDeinit(Engine& engine) override;
  };
}  // namespace bifrost

#endif /* BIFROST_COMPONENT_RENDERER_HPP */
