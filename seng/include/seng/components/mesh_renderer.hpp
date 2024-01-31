#pragma once

#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/toggle.hpp>
#include <seng/hook.hpp>

#include <string>
#include <utility>

namespace seng {
class Entity;

namespace rendering {
class CommandBuffer;
}

/**
 * The MeshRenderer component is the glue that binds meshes to materials (or
 * shader instances). On the shaderInstanceDraw scene hook, it fetches from the
 * cache (or loads if necessary) the mesh and renders it.
 */
class MeshRenderer : public ToggleComponent,
                     public ConfigParsableComponent<MeshRenderer> {
 public:
  /**
   * Creates a new MeshRenderer component tieing the mesh with the given path with
   * the material with the given name.
   */
  MeshRenderer(Entity& entity,
               std::string mesh,
               std::string material,
               bool enabled = true);
  MeshRenderer(const MeshRenderer&) = delete;
  MeshRenderer(MeshRenderer&&) = delete;
  ~MeshRenderer();

  MeshRenderer& operator=(const MeshRenderer&) = delete;
  MeshRenderer& operator=(MeshRenderer&&) = delete;

  DECLARE_COMPONENT_ID("MeshRenderer");
  DECLARE_CREATE_FROM_CONFIG();

  // Accessors
  const std::string& meshName() const { return m_meshName; }
  const std::string& shaderInstanceName() const { return m_matName; }

  // Setters
  void meshName(std::string name) { m_meshName = std::move(name); }
  void shaderInstanceName(std::string name);

  /// Render the mesh
  void render(const rendering::CommandBuffer& cmd) const;

 private:
  std::string m_meshName;
  std::string m_matName;
  HookToken<const rendering::CommandBuffer&> m_tok;
};

// FIXME: for some reason, the template doesn't get instantiated. Force it with
// this until we provide a macro + documentation
template class ConfigParsableComponent<MeshRenderer>;

}  // namespace seng
