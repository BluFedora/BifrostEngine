# Animation Runtime

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
 
## Runtime Data Structures

// Level Order Array of Nodes.
Nodes[XXX] = {
  Root, 
  RootChild0, RootChild1, RootChild2, RootChild3
 RootChild0Child0, RootChild2Child0
}

Node
 |
 |--> [FirstChild + NumChildren]

Skeleton
  InvBindPos

```c
enum InterpolationType
  LINEAR
  STEP
  CUBICSPLINE
 ```
 
Animation
  Channels
    Tracks
