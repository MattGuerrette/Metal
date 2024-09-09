
#pragma once

#include <fmt/core.h>

#include <Metal/Metal.hpp>

template <typename VertexType>
class StaticGeometry
{
public:
    explicit StaticGeometry(
        MTL::Device* device, std::vector<VertexType> vertices, std::vector<uint16_t> indices)
    {
        m_vertexBuffer = NS::TransferPtr(device->newBuffer(vertices.data(),
            sizeof(VertexType) * vertices.size(), MTL::ResourceOptionCPUCacheModeDefault));
        m_indexBuffer = NS::TransferPtr(device->newBuffer(indices.data(),
            sizeof(uint16_t) * indices.size(), MTL::ResourceOptionCPUCacheModeDefault));
    }

    [[nodiscard]] MTL::Buffer* vertexBuffer() const
    {
        return m_vertexBuffer.get();
    }

    [[nodiscard]] MTL::Buffer* indexBuffer() const
    {
        return m_indexBuffer.get();
    }

private:
    NS::SharedPtr<MTL::Buffer> m_vertexBuffer;
    NS::SharedPtr<MTL::Buffer> m_indexBuffer;
};