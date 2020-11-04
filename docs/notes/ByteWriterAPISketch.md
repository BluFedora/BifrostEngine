# Generic Buffer IO Writer Interface Sketch

## References:
- [AMD GPUOpen Driver](https://github.com/GPUOpen-Drivers/pal/blob/477c8e78bc4f8c7f8b4cd312e708935b0e04b1cc/shared/gpuopen/inc/util/ddByteWriter.h#L40-L65)
- [Buffer-centric IO Article](https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/)

---

```cpp
enum class Result
{
  Success,
  AllocationFailure,
  EndOfStream,
  ...etc...
};

// End of Stream Indicated by 'bytes == nullptr && num_bytes == 0u'.
using WriteFn = Result (*)(void* user_data, const void* bytes, size_t num_bytes);

struct ByteWriter : IByteWriter
{
  void*   user_data;
  WriteFn callback;
  Result  last_result;

  // API //

  // Must always be called with a non null pointer and a non zero size.
  Result write(const void* bytes, size_t num_bytes) override
  {
    if (last_result == Result::Success) 
    {
      // We don't care about the preconditions if we previously had an error.
      assert(bytes     && "write must be called with a valid set of bytes.");
      assert(num_bytes && "write must be called with more than 0 bytes.");

      last_result = callback(user_data, bytes, num_bytes);
    }

    return last_result;
  }

  Result end() override
  {
    if (last_result == Result::Success) 
    {
      callback(user_data, nullptr, 0u);
    }

    // Reset the status of last_result.
    return std::exchange(last_result, Result::Success);
  }

  ~ByteWriter() override = default;

  // Initialization Functions //

  static ByteWriter fromVector(Vector<uint8_t>* buffer)
  {
    return ByteWriter{
      buffer,
      [](void* user_data, const void* bytes, size_t num_bytes) -> Result 
      {
        Vector<uint8_t>* const buffer = static_cast<Vector<uint8_t>*>(user_data);

        try 
        {
          if (!(bytes == nullptr && num_bytes == 0u)) // Not End of Stream
          {
            const uint8_t* const typed_buffer = static_cast<const uint8_t*>(buffer);
            
            buffer->insert(buffer->end(), typed_buffer, typed_buffer + num_bytes);
          }
          else
          {
            // End of Stream, No work needed for Vector.
          }

          return Result::Success;
        }
        catch (const std::bad_alloc&) 
        {
          return Result::AllocationFailure;
        }
      },
      Result::Success,
    };
  }

  static ByteWriter fromCallback(WriteFn callback, void* user_data = nullptr)
  {
    return ByteWriter{
      user_data,
      callback,
      Result::Success,
    };
  }
};

struct ByteReader
{
  const uint8_t* buffer_start; // Start of buffer.
  const uint8_t* cursor;       // Invariant: buffer_start <= cursor <= buffer_end.
  const uint8_t* buffer_end;   // End of Buffer + 1, Should not be read from.
  Result         last_result;  // Initialized to `Result::Success`.

  size_t bufferSize() const { return buffer_end - buffer_start; }

  //  Pre-condition : cursor == buffer_end.
  // Post-condition : cursor == buffer_start, cursor < buffer_end.
  //         Return : last_result.
  Result (*refill)(ByteReader* self);
};

static Result refill_null(ByteReader* self)
{
  static const uint8_t s_ZeroBuffer[16] = {0};

  self->buffer_start = s_ZeroBuffer;
  self->cursor       = s_ZeroBuffer;
  self->buffer_end   = s_ZeroBuffer + sizeof(s_ZeroBuffer);

  return self->last_result;
}

static Result set_failure_state(ByteReader* self, Result err)
{
  self->last_result = err;
  self->refill      = &refill_null;

  return self->refill(self);
}

static ByteReader from_memory(const uint8_t* buffer, size_t buffer_size)
{
  self->buffer_start = buffer;
  self->cursor       = buffer;
  self->buffer_end   = buffer + buffer_size;
  self->last_result  = Result::Success;

  self->refill = [](ByteReader* self) -> Result
  {
    // A memory stream cannot be refilled.
    return set_failure_state(self, Result::EndOfStream);
  };
}
```
