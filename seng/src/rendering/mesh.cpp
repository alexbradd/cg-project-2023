#include <seng/rendering/buffer.hpp>
#include <seng/rendering/mesh.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>

#include <tiny_obj_loader.h>
#include <glm/geometric.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>

using namespace seng::rendering;
using namespace std;

static constexpr vk::BufferUsageFlags vertexBufferUsage =
    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags indexBufferUsage =
    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;

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

Mesh::Mesh(const Renderer &renderer,
           const std::vector<Vertex> &vertices,
           const std::vector<uint32_t> &indices) :
    m_vertices(renderer.device(), vertexBufferUsage, vertices.size() * sizeof(Vertex)),
    m_indices(renderer.device(), indexBufferUsage, indices.size() * sizeof(uint32_t)),
    m_count(indices.size())
{
  const auto &device = renderer.device();
  const auto &cmdPool = renderer.commandPool();
  const auto &queue = device.graphicsQueue();

  uploadTo(device, cmdPool, queue, this->m_vertices, vertices.size() * sizeof(Vertex), 0,
           vertices.data());
  uploadTo(device, cmdPool, queue, this->m_indices, indices.size() * sizeof(uint32_t), 0,
           indices.data());
}

Mesh Mesh::loadFromDisk(const Renderer &renderer,
                        const std::string &assetPath,
                        std::string name)
{
  std::string modelPath{filesystem::path{assetPath} / filesystem::path{name + ".obj"}};
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  // Lifted from vulkan-tutorial.com's "Loading models" chapter with minor adaptations
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
    throw std::runtime_error(err);
  }

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

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }
  return Mesh(renderer, std::move(vertices), std::move(indices));
}
