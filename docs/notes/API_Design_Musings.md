Need to have a good tweening lib. Easy to use with built in ways to tween: ("position", "scale", "rotation")
With a callback: OnComplete(DestroyPopup)

[https://assetstore.unity.com/packages/tools/animation/leantween-3595]

Ex:

startButton.onClick(
  [](const MouseEvent& evt)
  {
    Tween.doStartScreenAnimation(
      ...
    );

    evt.target.removeEventListener(evt.self/currentListener);
  }
);

To have animations be able to be undone after an interaction (press) we should always be animating towards a goal.
To "Undo" the goal is the starting value.


-------------------------------------------------------------------------------------
-------------------------------- Scripted Particles ---------------------------------
-------------------------------------------------------------------------------------

To boot strap the first version an actual scripting language can be used with binding functions
with the sole purpose of emitting a simple particle bytecode.

-------------------------------------------------------------------------------------
-------------------------------- Transform Hierarchy -------------------------------- 
-------------------------------------------------------------------------------------

* This will be an immediate update type of system.
* Systems that update the transform need to happen before any reads.
* With this immediate design we can have a Gameplay API for batch updating transforms?
    - If updating multiple objects that do not share an ancestor then multithread??

Things that don't need speed.
- Looping through Children.

Things that need to just work.
- Mesh Renderer, Particles needs to read a component's world_transform matrix.
- Physics, AI needs the current position, rotation and scale of the object.
- Transform needs to get the latest updates from the physics component.

Things that could be made fast.
- Physics only needs to sync if the Transform was messed with this frame.
- Transforms need to read from physics any updates it has.

Min Spec of Systems:
Audio          -> postition: Vec3;
In-Game GUI    -> postition: Vec3;
Cam Target     -> postition: Vec3;
Mesh Rendering -> obj-world: Matrix4x4;
Lights         -> postition: Vec3, scale: Vec3, obj-world: Matrix4x4;
Sprites        -> obj-world: Matrix4x4;
Particles      -> postition: Vec3, scale: Vec3, rotation: Vec3, obj-world: Matrix4x4;
Physics        -> postition: Vec3, scale: Vec3, rotation: Vec3;
Text Rendering -> obj-world: Matrix4x4;
Gameplay       -> postition: Vec3, scale: Vec3, rotation: Vec3;

-------------------------------------------------------------------------------------
---------------------------------- Prefabs and Meta --------------------------------- 
-------------------------------------------------------------------------------------

One major problem with the old prefab system is the inability to be able to override specific properties.
This is because from the editor's POV it has no context on what is being edited. To give the editor more
context the API for inspecting variables need to reflect (kind of a joke here) the file structure.

If this is automatic as it 'should' be then how would custom controls work?

Tide Engine Custom Control List:

* Sprite/'Scale To Texture Aspect Ratio'
* All bitmasks that had labels on them.
* Sprite/'mask layers'
* Sprite/'mask layers'/'<add layer>'
* Collider/'edit collider mode'
* ParticleEmitter/'Restart Emitter'
* ParticleEmitter/'Export Particle Fx'
* ParticleEmitter/'Import Particle Fx'
* ParticleEmitter/'Animation Properties'/'animation'
* Animation/'time left from frame'
* Animation/'current animation'
* Audio/name sound table
* Literally anything that dynamically shows and hides.

** Most of these can probably be solved by having 'type tags' IE more wrapper types with sematic meaning **

But how do you solve:
  * Custom Buttons
  * Extra custom text.

Looks like this needs all the info at data bindting time aka meta needs a lot more info.

Where Meta System Is Used in the Tide Engine:
  > Timeline Animation System
    - Just needed a list of all the members and what type it is.
    - Also needed to set and get ofc.
    - Must be Gotten By <T>
  > Serialization
    - Get, Set, IsProperty, IsRef, Name. Must be Gotten By <T>
    - Lots of specialization based off of T.
  > Automatic binding of Functions.
    - Would be nice to have Functions aswell.
    - Get, Set, Name.

Meta system Should have been more specialized:

IMetaField<T> {
  using type:   T;
  is_writeable: bool;
  is_function:  bool;
  is_writable:  bool;
  is_readable:  bool;
  is_class:     bool;
  is_field:     bool;
  is_pointer:   bool;
  is_enum:      bool;
  name:         string;

  // Extra Info:
  is_serialized: bool;
};

// Should be all fundamental types...
using BasicType = Variant<void*, int, String, float, etc...>;

meta::begin<T>();
meta::class_info();
meta::field();
meta::read_only();
meta::property();
meta::function();
meta::const_function();

Maybe for the editor have a:

inspector.push("COMPONENT_NAME", ...flags...);

inspector.addAfter("field_name", []()
{
  if (ImGui::Button("..."))
  {

  }
});
// Also: "inspector.addBefore".

inspector.onChange<T>("field_name", [](const T& old_value, const T& new_value)
{

});

inspector.bitfieldNames("field_name", BITFIELD_NAMES_ARRAY);
inspector.enumNames("field_name", ENUM_NAMES_ARRAY);
inspector.combo<String>("field_names", OPTIONS_ARRAY);
// custom would be an implicit "inspector::hide".
inspector.custom("field_name", [](const auto& raw_member)
{

});
inspector.hide("field_name");

inspector.onAnyChange([](const Array<String>& field_path, const Variant<...>& data)
{

});
inspector.pop();


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

                                      RUNTIME META OUTLINE                                         

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

BaseObject:
  Class            : ClassInfo*
  PropertyOverides : HashTable<String, GenericValue>

GenericValue : Variant / Any

BaseMetaInfo
  Name           : StringView
  // Path           : StringView
    
ClassInfo : BaseMetaInfo
  BaseClasses    : Array<ClassInfo>
  Members        : Array<MemberInfo>
  Properties     : Array<PropertyInfo>
  Methods        : Array<MethodInfo>
  Instanciate()  : GenericValue
  FindProperty() : PropertyInfo
  FindMember()   : MemberInfo
  FindMethod()   : MethodInfo
  Size()         : size_t
  Alignment()    : size_t

MemberInfo : BaseMetaInfo
  Get()             : GenericValue
  Set(GenericValue) : Void
  Size              : size_t 
  Offset            : size_t 

PropertyInfo : BaseMetaInfo
  Get()             : GenericValue
  Set(GenericValue) : Void

ArrayPropertyInfo : PropertyInfo
  Elements : Array<PropertyInfo> 

MethodInfo : BaseMetaInfo
  Invoke(GenericValue...) : GenericValue
  IsStatic()              : Bool

EnumInfo : BaseMetaInfo
  FindElement() : EnumElementInfo
  Instanciate() : StringView
  ToElement()   : EnumElementInfo
    
EnumElementInfo : BaseMetaInfo
  Value() : size_t

UnionInfo : ClassInfo

------------------------
----- Event System -----
------------------------

EventHandler evt_handler = entity->addEventListener(
  Event::ON_ENTITY_DELETE,
  [](const EntityDeleteEvent& evt)
  {
    ... event code here ...
  }
);

entity->removeEventListener(evt_handler);

EventManager {
  // User Facing API
  EventHandler addEventListener(EventType, IListener);
  // This overload will subscribe the listener to all event types?
  EventHandler addEventListener(IListener);
  bool         hasEventListener(EventType, EventHandler);
  void         removeEventListener(EventHandler);

  // Event Dispatcher API
  void processEvents();
  void queueEvent(const Event*, int delay);
  void dispatchEvent(const Event*);
};

// Better Words?
//   'subscribe' and 'unsubscribe'
//   'register'  and 'unregister'
//   'add'       and 'remove'
//   'connect'   and 'disconnect'

The event system can store a contiguous list of EventData,
  uint8_t header;
  char    event_data[...];

All events should be trivially destuctable. (otherwise we need to keep track of a dtor for all events regardless of if it's needed)
String Data can be stored inline to the EventBuffer.

There needs to be a way to signify the event has been captured / cancelled.
This feature must be independent of Gamestate / Behavior order in some way (EventHanlder layers???).

If we limit the amount of events to 32 or less then the type can be a flag adn event handlers can just "filter"
out what it listens to. Otherwise we need a lot of lists of each type of listener / checking each listener for the type it wants.
Maybe multiple lists would be a good thing? Instead of per object event managers we do a per system event management.
You find out if an entity dies from the Scene rather than the object itself? (or both???)

Should this system be thread friendly? I can't imagine why you would use events and threads at the same time.

---------------------------
----- Behavior System -----
---------------------------

The behavior system should NOT rely on Ctors and Dtors.

Rather Behaviors must have a ctor that takes in nothing.

Like Unity we should have:

[https://docs.unity3d.com/Manual/ExecutionOrder.html]
> Awake  - The first time the object is made active (called before Start).
> Start  - Called the first frame that this Behavior is active.
> Update - Called every frame on an active behavior.

The separation between Awake and Start is that with Awake you do some initializaton but you cannot assume other Behaviors have been initialized.
Then in Start you can assume all 'Awake's have been called so that you can do some inter object communication.

* Unity Requires that script files have a class with the same name as the file otherwise there is a rejection of adding it to an object.

----- Action List Design -----

The message board is not needed since the state could have been stored in the behavior itself.
Also maybe it should have a more immediate API like the RenderGraph / FrameGraph, this would
allow more dynamic behaviors since the actions are created each frame can be put into if statements.
This is retarded because then sense of timing will be very off.

----- Scripting Language Regstry -----

Rather than an int maybe this should be an opaque handle that is really an int.
typedef int bfVMRegistrySlot;

// Psuedo Codes
int reg_slot = vm.registerMakeSlot();
vm.registryLoadFromStack(reg_slot, slot_idx);
vm.stackLoadFromRegistry(slot_idx, reg_slot);
vm.registryDestoySlot(reg_slot);


----- GameObject Lifetime -----

How Unity does it is that they have a C# reference wrapper class that just contains 
a pointer to the native object. You must still explicitly destroy Objects (ex: Object.Destroy(gam_obj))
but then what you have left is an empty husk of a reference.

// [https://blogs.unity3d.com/2014/05/16/custom-operator-should-we-keep-it/]
// [https://jacx.net/2015/11/20/dont-use-equals-null-on-unity-objects.html]

---------------------------
-----    Graphics     -----
---------------------------

Overdraw Optimizations:
- Opaque objects should be front to back sorted.
  - Opaque UI elements being drawn frist has big wins if it takes up lots of screen.
- Sprites an trade off between a tight sprite mesh or a full quad.
  - This is a trade off between vertex count and overdraw.
- Translucent objects must use painter's algo to be correct. after opaque pass.

Transparency Handling:
- Alpha test in shader. `if (clr.a < 0.5) discard;`
- Draw Opqaue then transparent in painter algo order and pray a pathological case doesnt happen.
- Split polys and d more work than worth to get it right 80% of the time.

2D Graphics
- 2D can be though of as layers rather than depth.
  - Sorting by shader / texture helps less since layer order takes priority.
  - So just minimize the number of textures used (spritesheets)
- Simple rect culling can be used, because of this everything on screen
  will mostlikely contribute to the final image.
- For a hierarchy of widgets, object draw order can be reordered with its siblings
  only if it doesnt overlap anyone.

- When Batches Are Split
  - Clip Rect Change
  - Texture Change

- When a separate Composite Layer is needed.
  - parent has alpha that must propage to children.
  - Any "filter" effect
    - Drop Shadow
    - Guassian Blur
    - Glow
    - Bevel
    - Gradient Glow
    - Gradient Bevel
    - Color Adjust
  - Any blending mode other than the _default_ source-over.




Editor Idea:
"Inline Assets" - To make it less intrusive to add sharable properities to
                  field make it so editing of the properties can be inline
                  with that field and later extracted out.
                  This allows for the Designer to not lose there workflow
                  and work locally.

                  Less disruptive workflow while still allowing data driven workflow.
                  if an asset is dropped over this then just clear it.

                  Even asset refs should be editable here too?

                  EnemyStats: [Inline Asset] (*)
                                              [ Export Properties... ]
                                              [ Clear                ]
                    ... Display Stats here ...

[https://www.youtube.com/watch?v=raQ3iHhE_Kk&feature=youtu.be&ab_channel=Unity]



For example, style guides for Python projects often specify either 79 or 120 characters vs. Git commit messages should be no longer than 50 characters.



Windows Threads Pools expose a default thread pool to which work - including io requests - can be submitted. This is used by Microsoft’s STL implementation.
Apple platforms have Grand Central Dispatch, which works similarly but has a far cooler name.


