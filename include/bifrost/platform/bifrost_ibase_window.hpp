#ifndef BIFROST_IBASE_WINDOW_HPP
#define BIFROST_IBASE_WINDOW_HPP

#include "bifrost/data_structures/bifrost_ring_buffer.hpp"
#include "bifrost/event/bifrost_window_event.hpp"
#include "bifrost/utility/bifrost_function_view.hpp"  // FunctionView

namespace bifrost
{
  class IBaseWindow
  {
   private:
    FixedRingBuffer<Event, 64>           m_EventBuffer;
    FunctionView<void(const FileEvent&)> m_FileDropCallback;

   public:
    virtual bool open(const char* title, int width = 1280, int height = 720) = 0;
    virtual void close()                                                     = 0;
    virtual bool wantsToClose()                                              = 0;

    FunctionView<void(const FileEvent&)>& onFileDrop()
    {
      return m_FileDropCallback;
    }

    Event getNextEvent()
    {
      return m_EventBuffer.pop();
    }

    [[nodiscard]] bool hasNextEvent() const
    {
      return !m_EventBuffer.isEmpty();
    }

    template<typename T>
    void pushEvent(EventType type, const T& evt_data, std::uint8_t flags = Event::FLAGS_DEFAULT)
    {
      m_EventBuffer.push(Event(type, this, flags, evt_data));
    }

   protected:
    IBaseWindow()          = default;
    virtual ~IBaseWindow() = default;
  };
}  // namespace bifrost

#endif /* BIFROST_IBASE_WINDOW_HPP */
