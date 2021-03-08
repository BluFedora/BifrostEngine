# Unique IDs

This is a small collection of ideas on handling generating unique IDs for objects depending on the requirements of the system. The methods are roughly ordered least to greatest in expense / runtime speed but with stronger guarantees. 

## Monotonically Incrementing Integer

This is a simple scheme for generating ID when you have a single authority over the generation of IDs. All you do is increment an integer and return the old value. If you want to save the ID across fine just save the last number you generated and load that back up. 

```cpp
struct IDGen
{
  std::uintXX_t m_NextID = 0u;

  std::uintXX_t Get()
  {
    return ++m_NextID;
  }
};
```

There is one problem, if you delete many objects you end up with holes which may be undesirable. With 64bit IDs this may not be a problem as you will have to create and delete billions of objects to even get to the end of the ID space.

### Reusing IDs through pooling

This solves the ID holes problem by storing a stack of reusable IDs that gets used up before incrementing the integer. The stack will need to be saved to the file as well if you need serialization support.

```cpp
struct IDGen
{
  std::uintXX_t        m_NextID      = 0u;
  Stack<std::uintXX_t> m_ReusableIDs = {};

  std::uintXX_t Get()
  {
    if (!m_ReusableIDs.empty()) {
      return m_ReusableIDs.pop();
    }
  
    return ++m_NextID;
  }
  
  void Kill(std::uintXX_t id)
  {
    m_ReusableIDs.push(id);
  }
};
```

Reusing IDs has the problem that if you reuse and ID and there is an external object that refers to that ID then the external object will never know (aka ABA / use after free type of problem(s)).

## Handles (Index + Generation)

Handles are a way to somewhat handle the problem up reusing an ID but having external objects old IDs not work for the new object. This works by stealing a few bits of the id to store a _generational_ index of how many times the ID has been used. Each time a certain ID is given out then the generation should be incremented / changed in some meaningful way. When looking up the ID compare both the index and the generation and if either mismatches tell the caller the object they are looking up does not exist anymore.

```cpp
struct HandleID
{
  std::uintXX_t generation : k_BitsForGeneration; 
  std::uintXX_t id         : k_BitsForIndex; 
};
```

The generation will overflow and wrap around at some point depending on how much this is an issues you can change the number of bits it needs.

## UUID / GUID

UUIDs are a very heavy weight solution but are needed if you want to be able to generate a unique ID across processes / computers that may want to generate an ID at the same time. These tend to be 128 bit values when store compactly expanding out to a 36 byte string (in 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' format).

```cpp
using UUIDStr = SmallString<36 + 1 /* + 1 for nul terminator */>;

// This is what should get stored by the runtime.
struct UUID
{
  char bytes[16];
  
  // Min API Needed To Be Useful:
  
  static UUID    Gen();
  static UUID    FromString(const UUIDStr& str);
  static UUIDStr ToString(const UUID& uuid);
};
```

Operating systems have API for generating UUIDs that should be used but depending on the requirements generating multiple ransom values and putting them together into a 128 bit value should work out fine.
