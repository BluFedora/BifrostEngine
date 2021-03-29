# Editor UI


Featured Wanted for Nice Editor UX:

  - Tooltips for the fields.
  - Disable certain elements when
  - Being able to separeate content into headers.
  - Seprate content weith extra spaces / separators.

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
  bool                is_flags;
};
```
