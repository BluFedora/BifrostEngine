# Strings

Broadly speaking strings are a mistake and should be avoided. most problems can be solved by using a hashed string id or a fixed string buffer.

## String Intern Pool

A good scheme is to make all strings immutable and just store them in one big buffer and just check if the string already exists in the pool. The id just has be be an index into the big global buffer.

```cpp

std::uint32_t string_id1 = g_StringPool.Get("Hello Good Sir");
std::uint32_t string_id2 = g_StringPool.Get("Hello Good Sir");
std::uint32_t string_id3 = g_StringPool.Get("Hello World");

assert(string_id1 == string_id2);
assert(string_id1 != string_id3);

// or maybe This could be the id if you wanted some fancy pool allocation scheme.

struct StringID
{
  std::uint32_t pool_idx:    5;
  std::uint32_t pool_offset: 27;
};
```

## String Hashing

Using CRC-32 (or FNV-1a) as the hash algorithm for a unique id for strings works out pretty well. For handling collision just DONT and have the debug build check that there are no collisions or you can actually handle it by doing a real string compare after the hash compare passes.

[CRC-32 Info](https://www.gamedev.net/reference/articles/article1941.asp)
