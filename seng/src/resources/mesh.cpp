#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/resources/mesh.hpp>

#include <tiny_obj_loader.h>
#include <glm/geometric.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

using namespace seng;
using namespace seng::rendering;
using namespace std;

static constexpr vk::BufferUsageFlags vertexBufferUsage =
    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags indexBufferUsage =
    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;

Mesh::Mesh(const Renderer &renderer) :
    m_renderer(std::addressof(renderer)),
    m_vertices(),
    m_indices(),
    m_vbo(nullopt),
    m_ibo(nullopt)
{
}

Mesh::Mesh(const Renderer &renderer,
           std::vector<Vertex> vertices,
           std::vector<uint32_t> indices) :
    m_renderer(std::addressof(renderer)),
    m_vertices(std::move(vertices)),
    m_indices(std::move(indices)),
    m_vbo(nullopt),
    m_ibo(nullopt)
{
}

static void uploadTo(const Device &device,
                     const vk::raii::CommandPool &pool,
                     const vk::raii::Queue &queue,
                     const Buffer &to,
                     vk::DeviceSize size,
                     vk::DeviceSize offset,
                     const void *data)
{
  vk::MemoryPropertyFlags hostVisible = vk::MemoryPropertyFlagBits::eHostCoherent |
                                        vk::MemoryPropertyFlagBits::eHostVisible;

  Buffer temp(device, vk::BufferUsageFlagBits::eTransferSrc, size, hostVisible, true);
  temp.load(data, 0, size, {});
  temp.copy(to, {0, offset, size}, pool, queue);
}

void Mesh::sync()
{
  if (m_vertices.size() == 0 || m_indices.size() == 0) {
    seng::log::warning("No data to sync... aborting");
    return;
  }
  seng::log::dbg("Uploading mesh to device");

  if (!m_vbo.has_value())
    m_vbo = Buffer(m_renderer->device(), vertexBufferUsage,
                   m_vertices.size() * sizeof(Vertex));
  if (!m_ibo.has_value())
    m_ibo = Buffer(m_renderer->device(), indexBufferUsage,
                   m_indices.size() * sizeof(uint32_t));

  const auto &device = m_renderer->device();
  const auto &cmdPool = m_renderer->commandPool();
  const auto &queue = device.graphicsQueue();

  uploadTo(device, cmdPool, queue, *m_vbo, m_vertices.size() * sizeof(Vertex), 0,
           m_vertices.data());
  uploadTo(device, cmdPool, queue, *m_ibo, m_indices.size() * sizeof(uint32_t), 0,
           m_indices.data());
}

Mesh Mesh::loadFromDisk(const Renderer &renderer,
                        const std::string &assetPath,
                        const std::string &name)
{
  std::string modelPath{filesystem::path{assetPath} / filesystem::path{name}};
  if (!filesystem::exists(modelPath)) {
    seng::log::error("Could not locate {}, returning empty mesh", name);
    return Mesh(renderer);
  }

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  // Lifted from vulkan-tutorial.com's "Loading models" chapter with minor adaptations
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
    seng::log::error("Could not load {}, returning empty mesh", name);
    return Mesh(renderer);
  }
  seng::log::dbg("Loaded mesh {} from disk", name);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::unordered_map<Vertex, uint32_t> uniqueVertices{};
  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                       attrib.normals[3 * index.normal_index + 1],
                       attrib.normals[3 * index.normal_index + 2]};
      vertex.normal = glm::normalize(vertex.normal);

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      vertex.tangent = {0.0f, 0.0f, 0.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }

  seng::log::dbg("Calculating tangent vectors");
  // Adapted from: https://ogldev.org/www/tutorial26/tutorial26.html
  for (unsigned int i = 0; i < indices.size(); i += 3) {
    Vertex &v0 = vertices[indices[i]];
    Vertex &v1 = vertices[indices[i + 1]];
    Vertex &v2 = vertices[indices[i + 2]];

    glm::vec3 edge1 = v1.pos - v0.pos;
    glm::vec3 Edge2 = v2.pos - v0.pos;

    float deltaU1 = v1.texCoord.x - v0.texCoord.x;
    float deltaV1 = v1.texCoord.y - v0.texCoord.y;
    float deltaU2 = v2.texCoord.x - v0.texCoord.x;
    float deltaV2 = v2.texCoord.y - v0.texCoord.y;

    float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

    glm::vec3 tangent;
    glm::vec3 bitangent;

    tangent.x = f * (deltaV2 * edge1.x - deltaV1 * Edge2.x);
    tangent.y = f * (deltaV2 * edge1.y - deltaV1 * Edge2.y);
    tangent.z = f * (deltaV2 * edge1.z - deltaV1 * Edge2.z);

    bitangent.x = f * (-deltaU2 * edge1.x + deltaU1 * Edge2.x);
    bitangent.y = f * (-deltaU2 * edge1.y + deltaU1 * Edge2.y);
    bitangent.z = f * (-deltaU2 * edge1.z + deltaU1 * Edge2.z);

    v0.tangent += tangent;
    v1.tangent += tangent;
    v2.tangent += tangent;
  }

  for (unsigned int i = 0; i < vertices.size(); i++) {
    glm::normalize(vertices[i].tangent);
  }

  return Mesh(renderer, std::move(vertices), std::move(indices));
}
