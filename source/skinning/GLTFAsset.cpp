////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "GLTFAsset.hpp"

#include <filesystem>

#include "File.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <imgui.h>

#include <fmt/format.h>

#include <SDL3/SDL_storage.h>

#include "GraphicsMath.hpp"

struct PrimitiveInfo
{
    uint32_t numVertices;
    uint32_t numTexCoords;
    uint32_t numNormals;
    uint32_t numColors;
    uint32_t numTangents;
    uint32_t numJoints;
    uint32_t numWeights;
    uint32_t numIndices;
};

namespace
{
    void draw_joint(const cgltf_node* joint)
    {
        if (joint->children == nullptr)
        {
            ImGui::BulletText("%s", joint->name ? joint->name : "JOINT");
        }
        else
        {
            if (ImGui::TreeNode(fmt::format("{}", joint->name ? joint->name : "JOINT").c_str()))
            {
                for (size_t i = 0; i < joint->children_count; ++i)
                {
                    draw_joint(joint->children[i]);
                }
                ImGui::TreePop();
            }
        }
    }
    constexpr int ComponentCount(cgltf_type type)
    {
        switch (type)
        {
        case cgltf_type_scalar:
            return 1;
        case cgltf_type_vec2:
            return 2;
        case cgltf_type_vec3:
            return 3;
        case cgltf_type_vec4:
        case cgltf_type_mat2:
            return 4;
        case cgltf_type_mat3:
            return 9;
        case cgltf_type_mat4:
            return 16;
        case cgltf_type_invalid:
        case cgltf_type_max_enum:
            break;
        }

        return -1;
    }

    template <typename T>
    void LoadAttribute(const cgltf_accessor* accessor, T* dest)
    {
        int        numComponents = ComponentCount(accessor->type);
        cgltf_size n = 0;
        T*         buffer = (T*)accessor->buffer_view->buffer->data
            + accessor->buffer_view->offset / sizeof(T) + accessor->offset / sizeof(T);

        for (unsigned int k = 0; k < accessor->count; k++)
        {
            for (int l = 0; l < numComponents; l++)
            {
                dest[numComponents * k + l] = buffer[n + l];
            }
            n += accessor->stride / sizeof(T);
        }
    }

    void PrimitiveCalculateVertexIndexCount(const cgltf_primitive* primitive, PrimitiveInfo& info)
    {
        info = {};
        info.numIndices += primitive->indices->count;

        for (int32_t j = 0; j < primitive->attributes_count; j++)
        {
            const cgltf_attribute* attribute = &primitive->attributes[j];
            if (attribute->type == cgltf_attribute_type_position)
            {
                info.numVertices += attribute->data->count;
            }
            else if (attribute->type == cgltf_attribute_type_texcoord)
            {
                info.numTexCoords += attribute->data->count;
            }
            else if (attribute->type == cgltf_attribute_type_color)
            {
                info.numColors += attribute->data->count;
            }
            else if (attribute->type == cgltf_attribute_type_normal)
            {
                info.numNormals += attribute->data->count;
            }
            else if (attribute->type == cgltf_attribute_type_tangent)
            {
                info.numTangents += attribute->data->count;
            }
            else if (attribute->type == cgltf_attribute_type_joints)
            {
                info.numJoints += attribute->data->count;
            }
            else if (attribute->type == cgltf_attribute_type_weights)
            {
                info.numWeights += attribute->data->count;
            }
        }
    }
}

GLTFAsset::GLTFAsset(MTL::Device* device, const std::string& name)
    : m_device(device)
{
    cgltf_options         options = {};
    const auto            resourcePath = SDL_GetBasePath();
    std::filesystem::path path = resourcePath;
    path.append(name);

    init(path);
}

GLTFAsset::GLTFAsset(MTL::Device* device, const std::filesystem::path& filePath)
    : m_device(device)
{
    init(filePath);
}

void GLTFAsset::init(const std::filesystem::path& filePath)
{
    cgltf_data*   data = nullptr;
    cgltf_options options = {};
    if (cgltf_parse_file(&options, filePath.c_str(), &data) != cgltf_result_success)
    {
        throw std::runtime_error("Failed to parse GLTF asset");
    }

    m_data.reset(data);

    if (cgltf_load_buffers(&options, m_data.get(), filePath.c_str()) != cgltf_result_success)
    {
        throw std::runtime_error("Failed to load buffers");
    }

    for (uint32_t i = 0; i < m_data->meshes_count; ++i)
    {
        const cgltf_mesh* mesh = &m_data->meshes[i];
        m_meshes.emplace_back(m_device, mesh);
    }
}

std::vector<Matrix> GLTFAsset::boneMatricesForAnimation(int32_t animation) const
{
    std::vector<Matrix> result;

    const cgltf_skin* skin = &m_data->skins[0];
    for (uint32_t i = 0; i < skin->inverse_bind_matrices->count; ++i)
    {
        auto matrix = new float[64];
        LoadAttribute(&skin->inverse_bind_matrices[i], matrix);
        result.emplace_back(matrix);
        delete[] matrix;
    }

    return result;
}

std::vector<std::string> GLTFAsset::animations() const
{
    assert(m_data != nullptr);

    std::vector<std::string> result;
    for (uint32_t i = 0; i < m_data->animations_count; ++i)
    {
        const cgltf_animation* animation = &m_data->animations[i];
        if (animation->name)
        {
            auto node = animation->channels[0].target_node;
            result.emplace_back(animation->name);
        }
        else
        {
            // Generate animation name
            result.emplace_back(fmt::format("Animation {}", i));
        }
    }

    return result;
}

const cgltf_animation* GLTFAsset::getAnimation(int32_t index) const
{
    assert(m_data != nullptr);
    if (m_data->animations)
    {
        return &m_data->animations[index];
    }

    return nullptr;
}

void GLTFAsset::initDeviceResources()
{
    // TODO: Load device resources such as pipelines and GPU buffers
}

void GLTFAsset::drawUi()
{
    assert(m_data != nullptr);

    const cgltf_node* joint = m_data->skins[0].joints[0];
    draw_joint(joint);
}

void GLTFAsset::render(MTL::RenderCommandEncoder* commandEncoder)
{
    // TODO: Draw meshes using command encoder
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeBack);

    for (auto& mesh : m_meshes)
    {
        for (auto& primitive : mesh.primitives)
        {
            if (primitive.material)
            {
                commandEncoder->setFragmentTexture(primitive.material->baseColorTexture(), 0);
            }
            commandEncoder->setVertexBuffer(primitive.geometry->vertexBuffer(), 0, 0);
            commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
                primitive.geometry->indexBuffer()->length() / sizeof(uint16_t),
                MTL::IndexTypeUInt16, primitive.geometry->indexBuffer(), 0, 1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////

Primitive::Primitive(MTL::Device* device, const cgltf_primitive* prim)
{
    PrimitiveInfo info {};
    PrimitiveCalculateVertexIndexCount(prim, info);

    MTK::TextureLoader* loader = MTK::TextureLoader::alloc()->init(device);
    // TODO: Access material
    if (prim->material)
    {
        material = std::make_unique<Material>(loader, prim->material);
    }

    auto                  positions = new float[info.numVertices * 3];
    auto                  texcoords = new float[info.numTexCoords * 2];
    auto                  colors = new float[info.numColors * 4];
    auto                  normals = new float[info.numNormals * 3];
    auto                  joints = new uint16_t[info.numJoints * 4];
    auto                  weights = new float[info.numWeights * 4];
    std::vector<uint16_t> indices(info.numIndices);

    MTL::PrimitiveType primitiveType;
    switch (prim->type)
    {
    case cgltf_primitive_type_triangles:
        primitiveType = MTL::PrimitiveTypeTriangle;
        break;
    case cgltf_primitive_type_line_strip:
        primitiveType = MTL::PrimitiveTypeLineStrip;
        break;
    case cgltf_primitive_type_lines:
        primitiveType = MTL::PrimitiveTypeLine;
        break;
    case cgltf_primitive_type_points:
        primitiveType = MTL::PrimitiveTypePoint;
        break;
    case cgltf_primitive_type_triangle_strip:
        primitiveType = MTL::PrimitiveTypeTriangleStrip;
        break;
    case cgltf_primitive_type_line_loop:
        break;
    case cgltf_primitive_type_triangle_fan:
        break;
    case cgltf_primitive_type_invalid:
        break;
    case cgltf_primitive_type_max_enum:
        break;
    }

    const cgltf_accessor* accessor = nullptr;
    for (int32_t attr = 0; attr < prim->attributes_count; attr++)
    {
        const cgltf_attribute* attribute = &prim->attributes[attr];
        accessor = attribute->data;
        if (attribute->type == cgltf_attribute_type_position)
        {
            assert(accessor->type == cgltf_type_vec3
                && accessor->component_type == cgltf_component_type_r_32f);
            LoadAttribute(accessor, positions);
        }
        else if (attribute->type == cgltf_attribute_type_normal)
        {
            assert(accessor->type == cgltf_type_vec3
                && accessor->component_type == cgltf_component_type_r_32f);
            LoadAttribute(accessor, normals);
        }
        else if (attribute->type == cgltf_attribute_type_texcoord)
        {
            assert(accessor->type == cgltf_type_vec2
                && accessor->component_type == cgltf_component_type_r_32f);
            LoadAttribute(accessor, texcoords);
        }
        else if (attribute->type == cgltf_attribute_type_color)
        {
            assert(accessor->type == cgltf_type_vec4
                && accessor->component_type == cgltf_component_type_r_32f);
            LoadAttribute(accessor, colors);
        }
        else if (attribute->type == cgltf_attribute_type_joints)
        {
            assert(accessor->type == cgltf_type_vec4
                && accessor->component_type == cgltf_component_type_r_16u);
            LoadAttribute(accessor, joints);
        }
        else if (attribute->type == cgltf_attribute_type_weights)
        {
            assert(accessor->type == cgltf_type_vec4
                && accessor->component_type == cgltf_component_type_r_32f);
            LoadAttribute(accessor, weights);
        }
    }

    if (prim->indices != nullptr)
    {
        accessor = prim->indices;
        LoadAttribute(accessor, &indices[0]);
    }

    std::vector<Vertex> vertices(info.numVertices);
    int32_t             iVertex = 0;
    int32_t             iTexCoord = 0;
    int32_t             iColor = 0;
    int32_t             iNormal = 0;
    int32_t             iJoint = 0;
    int32_t             iWeight = 0;
    for (uint32_t j = 0; j < info.numVertices; j++)
    {
        const int32_t v0 = iVertex;
        const int32_t v1 = iVertex + 1;
        const int32_t v2 = iVertex + 2;

        const int32_t uv0 = iTexCoord;
        const int32_t uv1 = iTexCoord + 1;

        const int32_t c0 = iColor;
        const int32_t c1 = iColor + 1;
        const int32_t c2 = iColor + 2;
        const int32_t c3 = iColor + 3;

        const int32_t n0 = iNormal;
        const int32_t n1 = iNormal + 1;
        const int32_t n2 = iNormal + 2;

        const int32_t j0 = iJoint;
        const int32_t j1 = iJoint + 1;
        const int32_t j2 = iJoint + 2;
        const int32_t j3 = iJoint + 3;

        const int32_t w0 = iWeight;
        const int32_t w1 = iWeight + 1;
        const int32_t w2 = iWeight + 2;
        const int32_t w3 = iWeight + 3;

        vertices[j].position = Vector4(positions[v0], positions[v1], positions[v2], 1.0f);

        if (info.numColors > 0)
        {
            vertices[j].color = Vector4(colors[c0], colors[c1], colors[c2], 1.0f);
        }
        else
        {

            vertices[j].color = Vector4(DirectX::Colors::Red.f);
        }

        // vertices[j].normal = Vector3(normals[n0], normals[n1], normals[n2]);

        vertices[j].texcoord = Vector2(texcoords[uv0], texcoords[uv1]);

        vertices[j].joint = Vector4(joints[j0], joints[j1], joints[j2], joints[j3]);
        vertices[j].weight = Vector4(weights[w0], weights[w1], weights[w2], weights[w3]);

        iVertex += 3;
        iTexCoord += 2;
        iColor += 4;
        iJoint += 4;
        iWeight += 4;
    }
    delete[] positions;
    delete[] texcoords;
    delete[] colors;
    delete[] normals;
    delete[] joints;
    delete[] weights;

    geometry = std::make_unique<StaticGeometry<Vertex>>(device, vertices, indices);
}

////////////////////////////////////////////////////////////////////////////

Mesh::Mesh(MTL::Device* device, const cgltf_mesh* mesh)
{
    for (uint32_t i = 0; i < mesh->primitives_count; ++i)
    {
        const cgltf_primitive* prim = &mesh->primitives[i];
        primitives.emplace_back(device, prim);
    }
}

////////////////////////////////////////////////////////////////////////////

Material::Material(MTK::TextureLoader* textureLoader, const cgltf_material* material)
{
    if (material->has_pbr_metallic_roughness)
    {
        if (material->pbr_metallic_roughness.base_color_texture.texture)
        {
            const auto id = fmt::format("{}_baseColorTexture", material->name);

            const auto texture = material->pbr_metallic_roughness.base_color_texture.texture;
            if (texture->image != nullptr)
            {
                const auto image = texture->image;

                uint8_t* data
                    = (uint8_t*)image->buffer_view->buffer->data + image->buffer_view->offset;
                uint32_t length = image->buffer_view->size;

                const void*               keys[] = { MTK::TextureLoaderOptionTextureUsage,
                                  MTK::TextureLoaderOptionTextureStorageMode };
                NS::SharedPtr<NS::Number> usageMode
                    = NS::TransferPtr(NS::Number::number(MTL::TextureUsageShaderRead));
                NS::SharedPtr<NS::Number> storageMode
                    = NS::TransferPtr(NS::Number::number(MTL::StorageModePrivate));
                const void* values[]
                    = { (const void*)(usageMode.get()), (const void*)(storageMode.get()) };
                CFDictionaryRef textureLoaderOptions = CFDictionaryCreate(nullptr, keys, values, 2,
                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

                NS::Error* error = nullptr;
                m_baseColorTexture = NS::TransferPtr(textureLoader->newTexture(
                    NS::Data::data(data, length), (NS::Dictionary*)textureLoaderOptions, &error));
                CFRelease(textureLoaderOptions);
                if (error != nullptr)
                {
                    throw std::runtime_error("Failed to create texture from file");
                }
            }
            else if (texture->extensions_count != 0)
            {
                // Texture uses special extension type image
                for (int32_t i = 0; i < texture->extensions_count; i++)
                {
                    const auto extension = texture->extensions[i];
                    if (!std::strcmp(extension.name, "KHR_texture_basisu"))
                    {
                        printf("Texture uses Basis Universal Compression\n");
                    }
                }
            }
        }

        //        if (material->pbr_metallic_roughness.metallic_roughness_texture.texture)
        //        {
        //            const auto id = std::format("{}_MetallicRoughnessTexture", material->name);
        //
        //            const auto texture
        //                = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
        //            MetallicRoughnessTexture_ = LoadTexture(id, texture, assetManager);
        //        }
    }
}

MTL::Texture* Material::baseColorTexture() const
{
    return m_baseColorTexture.get();
}
