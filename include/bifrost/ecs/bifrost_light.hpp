/*!
* @file   bifrost_light.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-27
*
* @copyright Copyright (c) 2020
*/
#ifndef BIFROST_LIGHT_HPP
#define BIFROST_LIGHT_HPP

#include "bifrost/bifrost_math.h"         /* bfColor4f */
#include "bifrost/math/bifrost_rect2.hpp" /* Vector3f */
#include "bifrost_base_component.hpp"     /* BaseComponent       */

namespace bifrost
{
  static constexpr float k_PI       = 3.14159265359f;
  static constexpr float k_RadToDeg = 57.2957795131f;
  static constexpr float k_DegToRad = 0.01745329251f;
  static constexpr float k_Epsilon  = 1.0e-4f;

  enum class LightType
  {
    DIRECTIONAL,
    POINT,
    SPOT,
  };

  class Light final : public Component<Light>
  {
    friend class StandardRenderer;

   private:
    struct LightGPUDataCache final
    {
      float inv_light_radius_pow2;  // (1.0 / radius)^2
      float spot_scale;             // 1.0 / max(cos(inner_angle) - cos(outer_angle), k_Epsilon)
      float spot_offset;            // -cos(outer_angle) * spot_scale
      bool  is_dirty;
    };

   private:
    LightType         m_Type;            //!< The type of light to render.
    bfColor4f         m_ColorIntensity;  //!< For DIRECTIONAL, POINT, and SPOT, Alpha is Intensity and must be >= 0.0f.
    Vector3f          m_Direction;       //!< For DIRECTIONAL and SPOT.
    float             m_Radius;          //!< For POINT and SPOT, must be positive.
    float             m_InnerAngleRad;   //!< For SPOT, must be less than [Light::m_OuterAngleRad].
    float             m_OuterAngleRad;   //!< For SPOT, must be greater than [Light::m_InnerAngleRad].
    LightGPUDataCache m_GPUCache;        //!< For POINT and SPOT, A cache of soem calulations needed for shading.

   public:
    explicit Light(Entity& owner) :
      Base(owner),
      m_Type{LightType::POINT},
      m_ColorIntensity{1.0f, 1.0f, 1.0f, 5.0f},
      m_Direction{1.0f, 0.0f, 0.0f},
      m_Radius{2.0f},
      m_InnerAngleRad{k_PI * 0.5f},
      m_OuterAngleRad{k_PI},
      m_GPUCache{0.0f, 0.0f, 0.0f, true}
    {
    }

    LightType        type() const { return m_Type; }
    const bfColor4f& colorIntensity() const { return m_ColorIntensity; }
    const Vector3f&  direction() const { return m_Direction; }
    float            radius() const { return m_Radius; }
    float            innerAngleRad() const { return m_InnerAngleRad; }
    float            outerAngleRad() const { return m_OuterAngleRad; }
    float            innerAngleDeg() const { return m_InnerAngleRad * k_RadToDeg; }
    float            outerAngleDeg() const { return m_OuterAngleRad * k_RadToDeg; }

    void setColor(const bfColor4f& value)
    {
      m_ColorIntensity = value;
    }

    void setRadius(float value)
    {
      m_Radius            = value;
      m_GPUCache.is_dirty = true;
    }
  };
}  // namespace bifrost

#endif /* BIFROST_LIGHT_HPP */
