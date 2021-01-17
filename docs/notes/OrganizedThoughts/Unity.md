# Unity

## `ScriptableObject`s

`ScriptableObject`s are a way to create a custom asset type in Unity that can be
serialized to disc.

They can be saved as:

- Standalone assets
- SubAssets of Other ScriptableObjects.
- Local To A Scene



```cs
ScriptableObject.CreateInstance<ScriptableObjectType>();
```

```cs

```

Gotchas

- The subAsset with the name the same as the asset turns into main asset.
- ScriptableObject must be defined in a file with the same name as the class.

References:
  [All the places a ScriptableObject is stored](https://blog.eyas.sh/2020/09/where-scriptableobjects-live/)
