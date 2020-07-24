/******************************************************************************/
/*!
* @file   bifrost_rect2.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Contains Utilities for 2D rectangle math.
*
* @version 0.0.1
* @date    2020-03-17
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bifrost/math/bifrost_rect2.hpp"

namespace bifrost
{
  Rect2i rect::aspectRatioDrawRegion(std::uint32_t aspect_w, std::uint32_t aspect_h, std::uint32_t window_w, std::uint32_t window_h)
  {
    Rect2i result = {};

    if (aspect_w > 0 && aspect_h > 0 && window_w > 0 && window_h > 0)
    {
      const float optimal_w = float(window_h) * (float(aspect_w) / float(aspect_h));
      const float optimal_h = float(window_w) * (float(aspect_h) / float(aspect_w));

      if (optimal_w > float(window_w))
      {
        result.setRight(int(window_w));
        result.setTop(int(0.5f * (window_h - optimal_h)));
        result.setBottom(int(result.top() + optimal_h));
      }
      else
      {
        result.setBottom(int(window_h));
        result.setLeft(int(0.5f * (window_w - optimal_w)));
        result.setRight(int(result.left() + optimal_w));
      }
    }

    return result;
  }
}  // namespace bifrost
