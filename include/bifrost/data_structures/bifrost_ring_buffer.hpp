#ifndef BIFROST_RING_BUFFER_HPP
#define BIFROST_RING_BUFFER_HPP

#include <type_traits> /* aligned_storage */

namespace bifrost
{
  namespace detail
  {
    template<typename T>
    struct RingBufferImpl
    {
      T*     buffer;
      size_t head;
      size_t tail;

      RingBufferImpl(T* storage) :
        buffer{storage},
        head{0},
        tail{0}
      {
      }

      [[nodiscard]] size_t size() const { return tail - head; }
      [[nodiscard]] bool   isEmpty() const { return head == tail; }

      template<typename FWrap>
      bool push(const T& element, FWrap&& wrap)
      {
        if (wrap(tail + 1) == head)
        {
          // NOTE(Shareef):
          //   Since this is used for the event system having the latest event
          //   is a better policy than keeping the old events.
          pop(wrap);
          // return false;
        }

        new (buffer + tail) T(element);
        tail         = wrap(tail + 1);
        return true;
      }

      template<typename F, typename FWrap>
      void forEach(F&& fn, FWrap&& wrap)
      {
        for (int i = head; i != tail; i = wrap(i + 1))
        {
          fn(i, buffer[i]);
        }
      }

      template<typename FWrap>
      T pop(FWrap&& wrap)
      {
        if (isEmpty())
        {
          // TODO(Shareef): Throw a 'real' exception.
          throw "";
        }

        T element = std::move(buffer[head]);
        buffer[head].~T();
        head       = wrap(head + 1);
        return element;
      }
    };
  }  // namespace detail

  template<typename T, size_t N>
  class FixedRingBuffer final : private detail::RingBufferImpl<T>
  {
    static_assert(N != 0, "A buffer of 0 size is not valid.");

    using storage_t = std::aligned_storage_t<sizeof(T) * N, alignof(T)>;

   private:
    storage_t m_Storage;

   public:
    FixedRingBuffer() :
      detail::RingBufferImpl<T>(reinterpret_cast<T*>(&m_Storage)),
      m_Storage{}
    {
    }

    using detail::RingBufferImpl<T>::size;
    using detail::RingBufferImpl<T>::isEmpty;

    bool push(const T& element)
    {
      return detail::RingBufferImpl<T>::push(element, wrapImpl);
    }

    T pop()
    {
      return detail::RingBufferImpl<T>::pop(wrapImpl);
    }

   private:
    static size_t wrapImpl(size_t number)
    {
      if constexpr ((N & N - 1) == 0)
      {
        return number & (N - 1);
      }
      else
      {
        return number % N;
      }
    }
  };
  /* TODO(Shareef): Allocators need to be defined before using this class.
  template<typename T>
  class RingBuffer : private detail::RingBufferImpl<T>
  {
   private:
    char*  m_Storage;
    size_t m_Capacity;

   public:
    RingBuffer(size_t initial_size) :
      detail::RingBufferImpl<T>(reinterpret_cast<T*>(m_Storage = new char[sizeof(T) * initial_size])),
      m_Storage{m_Storage},
      m_Capacity{initial_size}
    {
    }

    ~RingBuffer()
    {
      delete[] m_Storage;
    }

    using detail::RingBufferImpl<T>::size;
    using detail::RingBufferImpl<T>::isEmpty;

    bool push(const T& element)
    {
      return detail::RingBufferImpl<T>::push(element, [this](size_t number) { return number % m_Capacity; });
    }

    T& pop()
    {
      return detail::RingBufferImpl<T>::pop([this](size_t number) { return number % m_Capacity; });
    }
  };
  */
}  // namespace bifrost

#endif /* BIFROST_RING_BUFFER_HPP */