#include <seng/application.hpp>
#include <seng/components/mesh_renderer.hpp>
#include <seng/components/toggle.hpp>
#include <seng/components/transform.hpp>
#include <seng/hook.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/scene/entity.hpp>
#include <seng/scene/scene.hpp>

#include <yaml-cpp/yaml.h>

#include <memory>
#include <string>
#include <utility>

using namespace seng;
using namespace std::placeholders;

MeshRenderer::MeshRenderer(Entity& e,
                           std::string mesh,
                           std::string material,
                           bool enabled) :
    ToggleComponent(e, enabled)
{
  m_meshName = std::move(mesh);
  m_matName = std::move(material);

  m_tok = e.scene().onShaderInstanceDraw(m_matName).insert(
      std::bind(&MeshRenderer::render, this, _1));
}

MeshRenderer::~MeshRenderer()
{
  entity->scene().onShaderInstanceDraw(m_matName).remove(m_tok);
}

void MeshRenderer::shaderInstanceName(std::string name)
{
  entity->scene().onShaderInstanceDraw(m_matName).remove(m_tok);
  m_matName = std::move(name);
  m_tok = entity->scene().onShaderInstanceDraw(m_matName).insert(
      std::bind(&MeshRenderer::render, this, _1));
}

void MeshRenderer::render(const rendering::CommandBuffer& cmd) const
{
  // If it is not disabled
  if (!enabled()) return;

  // Get the corresponding mesh
  auto& meshName = m_meshName;
  auto& mesh = entity->application().renderer()->requestMesh(meshName);
  if (mesh.vertices().empty()) return;
  if (!mesh.synced()) mesh.sync();

  // Update the models transform
  auto& instance =
      entity->application().renderer()->shaders().objectShaderInstances().at(m_matName);
  instance.updateModelState(cmd, entity->transform()->toMat4());

  // Bind the vertex/index buffers
  cmd.buffer().bindVertexBuffers(0, *(*mesh.vertexBuffer()).buffer(), {0});
  cmd.buffer().bindIndexBuffer(*(*mesh.indexBuffer()).buffer(), 0,
                               vk::IndexType::eUint32);

  // Draw it it
  cmd.buffer().drawIndexed(mesh.indices().size(), 1, 0, 0, 0);
}

DEFINE_CREATE_FROM_CONFIG(MeshRenderer, entity, node)
{
  std::string mesh;
  std::string mat;
  bool enabled = true;

  if (node["model"] && node["model"].IsScalar()) mesh = node["model"].as<std::string>();
  if (node["instance"] && node["instance"].IsScalar())
    mat = node["instance"].as<std::string>();
  if (node["enabled"] && node["enabled"].IsScalar()) enabled = node["enabled"].as<bool>();
  return std::make_unique<MeshRenderer>(entity, mesh, mat, enabled);
}
