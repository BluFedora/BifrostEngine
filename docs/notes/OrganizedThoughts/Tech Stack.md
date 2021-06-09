# Reefy Tech Stack

## Edit Object Model

Featured Wanted for Nice Editor UX:
  - Tooltips for the fields.
  - Disable certain elements at will
  - Being able to separate content into headers.
  - Separate content with extra spaces / separators.


```cpp
struct IObjectEditor
{
  virtual void get(void* dst_ptr)                      = 0;
  virtual void set(void* dst_ptr, const void* src_ptr) = 0;
  
  // Or Alternatively
  virtual bool edit(void* dst_ptr) = 0;
};

struct ObjectModelField;
using OnEditFn = void (*)(ObjectModelField* self, const void* old_value);

struct ObjectProperty
{
   string_range   name;
   string_range   ui_name; // Optional
   string_range   tooltip;
   IObjectEditor* editor;
   OnEditFn       on_edit;
   TypeNameHash   type;
};
```

```cpp
struct DropdownInfo
{
  StringRange name;
  T           value;
};

struct UIPropertyAttributes
{
  // If this can be edited. (All)
  bool        is_enabled;
  bool        is_readonly;
  StringRange tooltip;
  
  // Constrain the value to this range. (Int, Float, Vectors, Quaternions)
  T min_value, max_value;

  // String
  bool is_multiline;

  // Float
  bool is_slider;

  // Int / String
  const DropdownInfo* dropdown_infos;
  size_t              num_dropdown_infos;
 
  // If the dropdown items should be interpreted as flags.
  bool                is_dropdown_mut_exclusive;
};
```

## Binary Object Schema

```

```