/******************************************************************************/
/*!
 * @file   bifrost_ref.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  All reflectable / serializable engine objects will inherit from this class.
 *
 * @version 0.0.1
 * @date    2020-06-09
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BIFROST_REF_HPP
#define BIFROST_REF_HPP

namespace bifrost
{
  class IBaseObject;

  class BaseRef
  {
   private:
    IBaseObject* m_Object;

   protected:
    explicit BaseRef(IBaseObject* object = nullptr) :
      m_Object{object}
    {
    }

   public:
    [[nodiscard]] IBaseObject* object() const { return m_Object; }

    void bind(IBaseObject* obj)
    {
      m_Object = obj;
    }
  };

  template<typename T>
  class Ref : public BaseRef
  {
   public:
    explicit Ref(T* object = nullptr) :
      BaseRef{(IBaseObject*)object}
    {
    }
  };
}  // namespace bifrost

#endif /* BIFROST_REF_HPP */
