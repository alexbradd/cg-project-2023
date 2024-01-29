# Simple ENGine

The Simple Engine (definitely not other words starting with S qualifying the
quality of a project) is a Vulkan-based game engine written from scratch.

Obviously I do not want to create the new Unity or Unreal (5 credits are too
little even for this), these are the goals I want to achieve with this engine,
plus those that have been actually implemented:

- [x] Load models via OBJ (through `tinyobjloader`)
- [ ] Loading textures (through `stb_image.h`)
- [x] Support for multiple scenes
- [x] Parse scene representations from files
- [x] Implement a system where the user can extend an object's behaviour with
      arbitrary scripts (in C++) (like Unity's MonoBehaviour)
- [ ] System to compose different vertex/fragment shaders into different object
      shaders
- [ ] Parse shader definition from a file
- [ ] System to create different materials by instantiating object shaders
- [ ] Global illumination with a single directional light

Things that I want to do if I have time left before the deadline:

- [ ] Proper cache handling (dropping "cold" meshes/textures/etc...)
- [ ] Proper event system
- [ ] Shadow mapping
- [ ] Mipmapping
  - [ ] Make it configurable
- [ ] Multisampling
  - [ ] Make it configurable
- [ ] Proper object IDs, not just some static `unsigned long` that gets
      incremented every time
- [ ] Some multithreading

## Some documentation

### Scene format

Scenes are defined inside of YAML (with `.yaml` extension) files stored in the
scene folder (specified by application at engine start). A scene is composed of:

- A name, which is equal to the filename without the `.yaml` extension
- A light
- A list of Entities

The first scene that the engine will load is `default.yml`.

An entity is very similar to a Unity GameObject: it is an object in the scene
that has a name, a position and a list of components. Components define various
functionalities that the entity will have (such as being a camera, having
graphics or any user defined ones). For more info see the section on components.

Example scene:

```yaml
Light:
  ambient: [1.0, 0.8, 0.8, 1.0]
  color: [1.0, 1.0, 1.0, 1.0]
  direction: [0.70, -0.70, 0.0] # ~45 deg cw

Entities:
  - name: cam
    transform:
      position: [0.0, 0.0, 10.0]
    components:
      - id: Camera
        main: true
  - name: other
    transform:
      position: [0.0, 5.0, 10.0]
    components:
      - id: MyAwesomeScript
        a_parameter: 10
        another_parameter: false
      - id: MeshRenderer
        model: suzanne.obj
        material: test
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

### Shader system

Shaders and shader instances are defined in a YAML file specified at engine start.
The file contains two keys:

- `Shaders`: a list of object shaders, each with:
  - A name
  - Two shader stages (one for vertex and one for fragment)
  - A list of texture types to pass to the fragment stage
- `Instances`: a list of shader instances (basically materials), each of which
  contains:
  - A name
  - The name of the shader it instances
  - A list of texture image paths

Example shader configuration:

```yaml
Shaders:
  - name: simple
    vert: simple_vert
    frag: simple_frag
    textureTypes: [2d]
  - name: complex
    vert: simple_vert
    frag: complex_frag
    textureTypes: [1d, 2d, 2d, 2d, 2d, 3d]

Instances:
  - name: test
    instanceOf: simple
    textures: [diffuse.png]
```

Texture order in the object shader definition is very important since it will
define the layout of the descriptor set containing the samplers (more on that in
the relevant section). Same goes for the texture paths in material definition.

If the instance contains more textures than the declaration, the extra textures
will simply not be passed and warning will be printed, if it contains less an
error will be thrown.

The global uniform buffer is referenced by the first descriptor set (set 0)
with the following bindings:

0. Transformation-to-clip-space parameters (bound to the vertex stage), in order:
   - `mat4`: projection matrix
   - `mat4`: view matrix
1. Lighting parameters (bound to the fragment stage):
   - `vec3`: ambient color
   - `vec3`: light direction
   - `vec4`: light color
   - `vec3`: camera position

Some useful data is passed as push constants for performance reasons. In order,
these are:

- `mat4`: the model matrix
- `mat4`: reserved space for later use

These buffers will be bound for all shaders.

Below one can find what attributes each stage is expected to receive as input or
output, to ensure composability between various vertex and fragment stages. Not
adhering to these conventions is possible, however one must be mindful when
composing non-convention compliant stages with other ones.

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
2. `vec3` fragment color
3. `vec2` fragment UV coordinate

#### Fragment shader

The fragment shader takes as input what the vertex stage feeds it and needs to
output a `vec4` containing the fragment output color.

Textures are in descriptor set 1, in bindings ordered as defined in the shader
configuration file.

## Some comments on the engine as a whole

This project has been created as a final project form my uni course, and such
has been done under a lot of crunch due to other courses and deadlines.
Moreover, it was my first C++/Vulkan experience coming from a background of
C/Rust and Java and no prior graphics programming. This means that the code is
not of the finest quality: coding conventions are all over the place, RAII is
implemented in a "good enough" way, some APIs may be badly designed and
inflexible etc... . Still, as a "baby's first game engine" I think it turned out
pretty good. :)

Some thanks to the tutorials/projects I ~~stole~~ _took inspiration_ from:

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Guide](https://vkguide.dev/)
- [Vulkan Samples' framework](https://github.com/KhronosGroup/Vulkan-Samples),
  (or some architectural bits
- Unity, for some architectural bits like the GameObject/Component system
- [Kohi engine](https://kohiengine.com/) and the [youtube series](https://www.youtube.com/playlist?list=PLv8Ddw9K0JPg1BEO-RS-0MYs423cvLVtj)
  (the early bits mainly), for being a nice to follow video tutorial and for
  some renderer architecture bits
