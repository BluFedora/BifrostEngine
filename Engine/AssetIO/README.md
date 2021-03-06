# Asset System Outline



## Document Structure

The asset system is built off of a `Document` model to allow for expressing referential relationships between assets.

```mermaid
classDiagram
  class DocumentHeader {
    version:    u32;
    references: ResourceRef[];
  }
  
  class Document {
    header: DocumentHeader;
    resources: Resource[];
  }
  
  class ResourceRefType {
    <<enumeration>>
    EXTERNAL_REF
    INTERNAL_REF
  }
  
  class ResourceRef {
    type:   ResourceRefType;
    ref_id: UUID;
  }
  
  class Resource {
    uuid:    UUID;
    name:    String;
    type:    u32;
    version: u32;
    data:    ...;
  }
  
  Document       *-- DocumentHeader: Contains a
  Document       o-- Resource: Has an Array of
  DocumentHeader *-- ResourceRef: Has Arrays of
  ResourceRef    *-- ResourceRefType: Has
```

A document is a collection of resources 


## ISerializer



## Asset Status

```cpp
enum AssetStatus
{
  UNLOADED, // (RefCount == 0)                            : Asset is not loaded.
  LOADING,  // (RefCount != 0) && (Flags & IsLoaded) == 0 : Asset loading on background thread.
  LOADED,   // (RefCount != 0) && (Flags & IsLoaded) != 0 : Asset is fully loaded.
};
```

## Built-in Asset Layout

```cpp
struct BaseAsset;

struct ScriptAsset : public BaseAsset
{
  bfValueHandle module_handle;
};

struct TextureAsset : public BaseAsset
{
  bfTextureHandle texture_handle;
};

struct ShaderModuleAsset : public BaseAsset
{
  bfShaderModuleHandle shader_module_handle;
};

struct ShaderProgramAsset : public BaseAsset
{
  bfShaderProgramHandle shader_program_handle;
};

struct MaterialAsset : public BaseAsset
{
  TextureAssetHandle albedo;
  TextureAssetHandle normal;
  TextureAssetHandle metallic;
  TextureAssetHandle roughness;
  TextureAssetHandle ambient_occlusion;
};

struct Animation2DAsset : public BaseAsset
{
  bfSpritesheet* anim_2D_spritesheet
};

struct Animation3DAsset : public BaseAsset
{
  struct Channel { ... };

  IMemoryManager&                 memory;
  double                          duration;
  double                          ticks_per_second;
  std::uint8_t                    num_channels;
  Channel*                        channels;
  HashTable<String, std::uint8_t> name_to_channel;
};

struct ModelAsset : public BaseAsset
{
  struct Mesh       { ... };
  struct Node       { ... };
  struct NodeIDBone { ... };

  Array<MaterialHandle> embedded_materials;
  Array<Mesh>           meshes;
  Array<Node>           nodes;
  Array<NodeIDBone>     bone_to_node;
  bfBufferHandle        standard_vertex_buffer;
  bfBufferHandle        vertex_bone_buffer;
  bfBufferHandle        index_buffer;
  Matrix4x4f            global_inv_transform;
};

struct TypefaceAsset : public BaseAsset
{
  Typeface*               typeface;
  HashTable<float, Font*> font_cache;
};

struct SoundFxAsset : public BaseAsset
{
  ???
};
```

## Asset Interface

- `void load(BaseAsset* self, Engine& engine)`
- `void reload(BaseAsset* self, Engine& engine)`
- `void unload(BaseAsset* self, Engine& engine)`
- `void saveContent(BaseAsset* self, Engine& engine, ISerializer& serializer)`
- `void saveMeta(BaseAsset* self, Engine& engine, ISerializer& serializer)`

## Asset Map

`BaseAsset` contains a `bfUUID m_UUID` which
is a copy of the _key_ in the Map. So this can be a specialized map
that uses the BaseAsset itself.

```cpp
struct AssetMap
{
  static const int k_MaxProbe = 16;

  // Typedefs for clarity of these type's purpose.
  using StringRangeInAsset = StringRange;
  using HashIndex          = std::size_t;
  using PathToIndex        = HashMap<StringRangeInAsset, HashIndex>;

  struct Node
  {
    static BaseAsset s_TombstoneSentinel = {...};

    BaseAsset* value;

    bool is_active()    const { return value != nullptr && !is_tombstone(); }
    bool is_tombstone() const { return value == &s_TombstoneSentinel;       }
    bool is_inactive()  const { return value == nullptr;                    }
  };

  Node*       assets;          // Ordered / hashed based on bfUUID.
  std::size_t num_assets;      // Must always be a power of 2.
  std::size_t num_assets_mask; // (`num_assets` - 1)
  PathToIndex path_to_asset;   // Path string stored in the `BaseAsset*`

  //
  // Looking up by string path uses `path_to_asset` then searches from the found index.
  // Looking up by UUID directly uses `BaseAsset*`.
  //
  
  template<typename TKey, typename FCmp>
  BaseAsset* find_impl(HashIndex start_index, TKey&& key, FCmp&& cmp)
  {
    const HashIndex end_index = start_index + k_MaxProbe;

    while (start_index < end_index)
    {
      const HashIndex actual_index = start_index & num_assets_mask;

      if (assets[actual_index].is_inactive()) 
      {
        return nullptr; 
      }

      if (assets[actual_index].is_active() && cmp(key, assets[actual_index].value))
      {
        return assets[actual_index].value;
      }
      
      ++start_index;
    }

    return nullptr;
  }
}
```

## File Extension Registration

```cpp
assets.registerFileExtensions(
  {".png", ".jpg", ".jpeg", ...},
  &AssetCreationCallback
);
```

## References / 'Prior Art'

- [Godot's Scene Format](https://docs.godotengine.org/en/stable/development/file_formats/tscn.html)
- [Unity Scene Format](https://docs.unity3d.com/Manual/FormatDescription.html)
- [Unity Serialization Details](https://www.programmersought.com/article/4566102035/)

## Tools To Consider Using

  - [Jinja](https://jinja.palletsprojects.com/en/2.11.x/) String _template_ library for python.
  - [ANTLR](https://www.antlr.org/) Parser generator with support for Java, C#, Python, Js, Go, C++, Swift, PHP and DART. 
  - [Message Pack](https://msgpack.org/index.html)
