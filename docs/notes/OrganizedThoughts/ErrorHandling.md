# Error Handling

## Out Of Memory (OOM)

As a way to handle out of memory errors if you cannot recover but still want to
be able to at least log a message you can allocate an _emergency buffer_ and free
that buffer when the error happens. Otherwise any operation you do such as 
logging out to thw console will crash since clearly you have ran out of memory 
for you process.

```cpp
char* g_EmergencyHeap = new char[16_kb];

try {
  // ... your program here ... //
}
catch (const std::bad_alloc& err) { // `std::bad_alloc` is defined in `<new>`
  delete[] g_EmergencyHeap;

  std::cerr << err.what() << "(OOM Error)" << std::endl;
  std::cin.get(); // Pause on error message.
}
```
[Adapted from This StackOverflow answer](https://stackoverflow.com/a/13327733)

There is not much you can do to handle out of memory. Just try to allocate 
upfront and work in discrete parts that do not allow memory allocation failure 
in the middle of a certain operation of your software.

This advice mostly applies to desktop software. If you are on an embedded device
just DON'T run out of memory :).

References:
  [Handling out-of-memory conditions in C by Eli Bendersky](https://eli.thegreenplace.net/2009/10/30/handling-out-of-memory-conditions-in-c)
