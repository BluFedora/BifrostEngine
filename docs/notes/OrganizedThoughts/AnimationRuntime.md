# Animation Runtime

#engine #animation
 
 ## General Information About Animation Systems
 
 Most animations do not use translation and scaling tracks. Rotations tend to be used the most, scaling tens to be for cartoon-y animations (and it is used then the scale is probably uniform) while translation is usually only for root motion. This makes it a good idea to omit these tracks for less runtime calculations and memory savings.
 
## Runtime Data Structures

#shared-data

This data is immutable after loading from file.
```c
struct Bone:
  node_idx:           u32;
  bone_offset_matrix: Matrix4x4; // Bone => Mesh space in bind pose.

struct Node:
  name:         String;
  transform:    Matrix4x4; // Transform relative to parent.
  bone_idx:     u8;
  first_child:  u32;
  num_children: u32;

// Nodes Are stored Level Order:
//   this allows for combact representation of the parent
//   child relationship by only storing an index to the
//   firsdt child then the number of children.
//
// Nodes[] = 
// {
//                         Root, 
//      RootChild0, RootChild1, RootChild2, RootChild3
//   RootChild0Child0,       RootChild2Child0
// }
//
struct AnimationSkeleton:
  global_inverse_bind_pose: Matrix4x4;
  nodes:                    []Nodes;
  bones:                    []Bones;
```


#shared-data 
```c
// Contains a list of channels
struct Animation:
  Channels
    Tracks
```

## Animator Controller

The animator controller contains a graph of state an model can be in

#runtime #shared-data
```c
struct Pair<Skeleton, Animation>:
  bone_to_channel_idx: u8[k_MaxBones];
  
struct AnimationStateNode:

struct AnimationStateTransition:
```

### Parameter Buffers

This manages passing dynamically changing data into the animator controller at runtime.

#runtime #edit-time #shared-data
```c
enum ParameterType(u16):
  FLOAT32
  INT32
  BOOLEAN
  TRIGGER

struct ParameterInfo:
  offset: u16;
  type:   ParameterType;
  
// Needed per animator controller asset.
struct AnimationParameterBufferDescriptor:
  name_to_byte_offset: HashMap<String, ParameterInfo>;  //!< Allows for looking up parameters by name.
  template_buffer:     AnimationParameterBuffer; //!< Contains the default data when making a new instance of this type of animator
```

#runtime #instance-data
```c
// Needed per instance of each object that uses this animator controller. 
struct AnimationParameterBuffer:
  data_size: size_t;
  data:      byte[data_size];
```


## Frame Data

The runtime is built off of immutable data that cannot be resized while the simulation is running.

```c
struct AnimationFrame<N:u32>:
  data: float32[N];

using AnimationFrameScalar     = AnimationFrame<1>;
using AnimationFrameVector     = AnimationFrame<3>;
using AnimationFrameQuaternion = AnimationFrame<4>;

struct AnimationTrack<TFrameType>
  num_frames: u32;
  times:      float[num_frames];
  frames:     TFrameType[num_frames];

bitfield TracksInUse:
  HAS_ROTATION_TRACK   = (1u << 0)
  HAS_SCALE_X_TRACK    = (1u << 1)
  HAS_SCALE_Y_TRACK    = (1u << 2)
  HAS_SCALE_Z_TRACK    = (1u << 3)
  HAS_POSITION_X_TRACK = (1u << 4)
  HAS_POSITION_Y_TRACK = (1u << 5)
  HAS_POSITION_Z_TRACK = (1u << 6)

// `track` is set when the appropriate bit in `TracksInUse` is set.
union OptionalTrack<T>:
  track:    AnimationTrack<T>;
  constant: T;

// Each animated bone will get it's own animation channel.
struct SkinnedMeshAnimationChannel:
  rot_track: OptionalTrack<AnimationFrameQuaternion>;
  scl_track: OptionalTrack<AnimationFrameScalar>[3];
  pos_track: OptionalTrack<AnimationFrameScalar>[3];
  track_set: TracksInUse;
```

```c
enum InterpolationType:
  LINEAR // Listed first so that it gets the value 0 as it is the most common case.
  STEP
  CUBICSPLINE

// This should be the result of the main animation routines.
struct STRTransform:
  scale:    Vector3;
  rotation: Quaternion;
  position: Vector3;
  
struct Pose
  modified_bones: u64[2]; // Bitfield with which bones were modified.
  num_bones:      u8;
  bones_poses:    STRTransform[num_bones];
```
More info on: [[Interpolation]]

## R & D

- Curve Simplification
- Quaternion (nlerp vs slerp)
- SSE / SIMD
- Multithreading (Job System)
- Variable Update Rate (Dist to cam, priority)
  - Dont update every bone (choose important ones)
  - Dont update every frame
  - Animation LODs
  - Frustum Culled
- Only storing xyz of quaternion.

## An Imported Model Has

- A List of Meshes
  - Each mesh has a list of bones
  - Each mesh has either one material or less.
  - List of vertices
- A List of Embedded Materials
  - Each Material has a list of properties and texture types.
  - Each texture type can have multiple textures.


## Assimp Data

- There may be more Nodes than Bones.
- Each mesh has an array of bones.
- Each mesh vertex needs some bone data (weight:float, bone_index:int)
- Each animation 'track' has which bone is affects + all the frame datas.
- A bone is associated with one aiNode, but an aiNode may not have a bone.
- Per Mesh Bone Data
  - Mat4x4 OffsetMatrix;
  - Node*  NodeToChange;
  - Per Vertex Data
    - Uint8 BoneIndex
    - Float BoneWeight

- Model Specific Data
  - Mat4x4       GlobalInverseTransform = scene->mRootNode->mTransformation.Inverse()
  - Node[]       Nodes;
  - BoneInput[]  BonesOffset

- Model Instance Data
  - BoneOutput[] BonesFinalTransformation

- Node
  - Mat4x4 Transform;
  - Int    FirstChild;
  - Int    NumChildren;
  - Int!   BoneIndex;

- Anim Specific Data
  - String      Name
  - AnimTrack[] Tracks
  
- AnimTrack
  - String NodeName;
  - Vec3[] TranslationKeys;
  - Quat[] RotationKeys;
  - Vec3[] ScaleKeys;

- BoneInput
  - Mat4x4 MeshToBoneSpace;

- BoneOutput
  - Mat4x4 FinalTransformation;

- <Model, Anim> Specific Data
  - AnimTrack*[] BoneToTrack
 
 
 ## GLTF 2.0
 
 animations.sampler.input = `accessor` for time data
 animations.sampler.output = `accessor` for keyframe data

## References

[Quaternion Space Optimization](http://peyman-mass.blogspot.com/2013/05/some-important-features-of-unit.html)

