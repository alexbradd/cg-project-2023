# Simple ENGine

The Simple Engine (definitely not other words starting with S qualifying the
quality of a project) is a Vulkan-based game engine written from scratch.

Obviously I do not want to create the new Unity or Unreal (5 credits are too
little even for this), these are the goals I want to achieve with this engine,
plus those that have been actually implemented:

- [x] Load models via OBJ
- [x] Support for multiple scenes
- [x] Parse scene representations from files
- [x] Implement a system where the user can extend an object's behaviour with
      arbitrary scripts (in CPP) (like Unity's MonoBehaviour)
- [ ] System to compose different vertex/fragment shaders into different object
      shaders
- [ ] System to create different materials by instantiating object shaders
- [ ] Loading textures
- [ ] Global illumination with a single directional light

Things that I want to do if I have time left before the deadline:

- [ ] Proper event system
- [ ] Global shader definition file to avoid copy-pasting
- [ ] Some multithreading

## Some documentation

### Scene format

Scenes are defined inside of YAML (with `.yaml` extension) files stored in the
scene folder (specified by application at engine start). A scene is composed of:

- A name, which is equal to the filename without the `.yaml` extension
- A list of object shaders, each with a name two shader stages (one for vertex
  and one for fragment) and a list of texture types to pass to the fragment
  shader
- A list of shader instances, each of which contains a name and a list of
  texture paths
- A light
- A list of Entities

The first scene that the engine will load is `default.yml`.

An entity is very similar to a Unity GameObject: it is an object in the scene
that has a name, a position and a list of components. Components define various
functionalities that the entity will have (such as being a camera, having
graphics or any user defined ones). For more info see the section on components.

Example scene:

```yaml
Shaders:
  - name: simple
    vert: simple_vert
    frag: simple_frag
    textureTypes: [2d]

ShaderInstances:
  - name: test
    textures: [diffuse.png]

Light:
  color: [1.0, 1.0, 1.0, 1.0]
  angle: 45

Entities:
  - name: "cam"
    transform:
      position: [0.0, 0.0, 10.0]
    components:
      - id: "Camera"
        main: true
  - name: "other"
    transform:
      position: [0.0, 5.0, 10.0]
    components:
      - id: "MyAwesomeScript"
        a_parameter: 10
        another_parameter: false
```

#### Footguns

Yea, I am on a deadline and stuff has a bit of jank.

1. On scene load, everything will be done in a certain order:

   1. Shaders are loaded and created
   2. Instances are loaded and created
   3. Entities are created sequentially as defined

   Meaning that the order in which we define things (especially Entities) needs
   to be taken into account.

2. Components are read and attached to entities sequentially in the same order
   as they are defined

   Keep an eye on which component searches for which during initialization. If
   you have circular dependencies, tough luck, find a way to remove them.

3. Texture order is important since it is the set order the shader will receive
   them in

### Component system

Each component inherits from the `BaseComponent` class. Most likely, users will
want to inherit from `ScriptComponent` however, since it provides useful hooks
into the update loop.

Each component will need to:

1. Declare an ID using the `DECLARE_COMPONENT_ID("...")` macro
2. If it wants to be parsed from the scene YAML:
   1. Extend `ConfigParsableComponent<...>`
   2. Define the `createFromConfig` or use the `DECLARE_CREATE_FROM_CONFIG` and
      `DEFINE_CREATE_FROM_CONFIG` macros

Scripts that inherit from `ScriptComponent` will have the following hooks
available:

- Component constructor: run on component creation
  - Potential footgun: due to implementation details on creation the component
    will not yet be attached to the entity (i.e. it will not appear in the
    entities component list). This shouldn't be a problem unless for some reason
    one queries itself on creation. In that case, don't.
- `onEnable`/`onDisable`: called when the script is disable/enabled via the
  aptly named `enable()`/`disable()` methods
- `onEarlyUpdate`: called at the earliest time during the drawing of the frame
  if the component is enabled. Use if your code needs to run before every
  script's `onUpdate`.
- `onUpdate`: called in the middle of the drawing loop just before the frame
  graphics are drawn if the component is enabled. Should be the main event to use.
- `onLateUpdate`: called after the frame has been drawn if the component is
  enabled. Use if your code needs to run after every script's `onUpdate`.
- Component destructor: runs on component removal, be it for entity destruction
  or scene destruction.

### Shader format

The global uniform buffers are in the first descriptor set and the bindings are
defined as follows:

0. Transformation-to-clip-space parameters (bound to the vertex stage):
   - `mat4` projection matrix
   - `mat4` view matrix
   - Model matrix is passed as a push constant for performance reasons
1. Lighting parameters (bound to the fragment stage):
   - `vec3` light direction
   - `vec4` light color
   - `vec3` camera position

#### Vertex shader

The vertex descriptor passed by the engine to the vertex shader is the
following:

0. Vertex position
1. Vertex normal
2. Vertex color
3. Vertex UV coordinates

The vertex stage must output (aside from `gl_Position`) the following parameters:

0. `vec3` fragment world position
1. `vec3` fragment normal
2. `vec2` fragment UV coordinate

#### Fragment shader

The fragment shader takes as input what the vertex stage feeds it and needs to
output a `vec4` containing the fragment output color.

Fragment shader parameters (only textures of various dimensions) are passed in
different sets as defined in the scene YAML.
