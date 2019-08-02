template<class T>
struct proxy_allocator
{
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T *       pointer;
  typedef const T * const_pointer;
  typedef T &       reference;
  typedef const T & const_reference;
  typedef T         value_type;

  template<class U>
  struct rebind
  {
    typedef proxy_allocator<U> other;
  };

  proxy_allocator() throw() {}
  proxy_allocator(const proxy_allocator &) throw() {}
  template<class U>
  proxy_allocator(const proxy_allocator<U> &) throw()
  {}
  ~proxy_allocator() throw() {}

  pointer       address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }

  pointer allocate(size_type s, void const * = 0)
  {
    return s ? reinterpret_cast<pointer>(yl_malloc(s * sizeof(T))) : 0;
  }

  void deallocate(pointer p, size_type)
  {
    yl_free(p);
  }

  size_type max_size() const throw()
  {
    return std::numeric_limits<size_t>::max() / sizeof(T);
  }

  void construct(pointer p, const T &val)
  {
    new (reinterpret_cast<void *>(p)) T(val);
  }

  void destroy(pointer p)
  {
    p->~T();
  }

  bool operator==(const proxy_allocator<T> &other) const
  {
    return true;
  }

  bool operator!=(const proxy_allocator<T> &other) const
  {
    return false;
  }
};