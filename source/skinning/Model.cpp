////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#define CGLTF_IMPLEMENTATION

#include <cgltf.h>

#include "Model.hpp"

#include <stdexcept>
#include <fmt/core.h>

#include "FileUtil.hpp"
#include "GraphicsMath.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>


XM_ALIGNED_STRUCT(16) Vertex
{
	Vector4 Position;
	//Vector4 Color;
	Vector2 TexCoord;
};

#define GLTF_MODEL_LOAD_ATTRIBUTE(accessor, component_count, type, dest)                     \
  {                                                                                               \
    int n = 0;                                                                                    \
    type *buffer = (type *) accessor->buffer_view->buffer->data +                                 \
                   accessor->buffer_view->offset / sizeof (type) +                                \
                   accessor->offset / sizeof (type);                                              \
    for (unsigned int k = 0; k < accessor->count; k++)                                            \
      {                                                                                           \
        for (int l = 0; l < component_count; l++)                                                 \
          {                                                                                       \
            dest[component_count * k + l] = buffer[n + l];                                        \
          }                                                                                       \
        n += (int) (accessor->stride / sizeof (type));                                            \
      }                                                                                           \
  }


static int32_t primitive_get_accessor_for_type(const cgltf_primitive* primitive,
	cgltf_attribute_type attribute_type)
{
	for (int32_t i = 1; i < primitive->attributes_count; i++)
	{
		const cgltf_attribute* attribute = &primitive->attributes[i];
		if (attribute->type == attribute_type)
		{
			return i;
		}
	}

	return -1;
}

static void
mesh_calculate_vertex_index_count(const cgltf_mesh* mesh,
	uint32_t* vertex_count,
	uint32_t* uv_count,
	uint32_t* index_count)
{
	*vertex_count = 0;
	*uv_count = 0;
	*index_count = 0;

	for (int32_t i = 0; i < mesh->primitives_count; i++)
	{
		const cgltf_primitive* primitive = &mesh->primitives[i];
		*index_count += primitive->indices->count;

		for (int32_t j = 0; j < primitive->attributes_count; j++)
		{
			const cgltf_attribute* attribute = &primitive->attributes[j];
			if (attribute->type == cgltf_attribute_type_position)
			{
				*vertex_count += attribute->data->count;
			}
			else if (attribute->type == cgltf_attribute_type_texcoord)
			{
				*uv_count += attribute->data->count;
			}
		}
	}
}

Model::Model(const std::string& fileName, MTL::Device* device)
{
	auto filePath = FileUtil::PathForResource(fileName);
	NS::String* nsFilePath = NS::String::string(filePath.c_str(), NS::UTF8StringEncoding);
	NS::Data* data = NS::Data::data(nsFilePath);
	nsFilePath->release();
	if (data == nullptr)
	{
		throw std::runtime_error(fmt::format("Failed to load file: {}", fileName));
	}

	cgltf_options options = {};
	cgltf_data* modelData = nullptr;
	cgltf_result result = cgltf_parse(&options, data->bytes(), data->length(), &modelData);
	if (result != cgltf_result_success)
	{
		throw std::runtime_error(fmt::format("Failed to parse GLTF file: {}", fileName));
	}

	result = cgltf_load_buffers(&options, modelData, nullptr);
	if (result != cgltf_result_success)
	{
		throw std::runtime_error("Failed to load GLTF buffers");
	}


	for (int32_t i = 0; i < modelData->meshes_count; i++)
	{
		const cgltf_mesh* mesh = &modelData->meshes[i];

		uint32_t numVertices = 0;
		uint32_t numTexCoords = 0;
		uint32_t numIndices = 0;
		mesh_calculate_vertex_index_count(mesh, &numVertices, &numTexCoords,
			&numIndices);

		auto positions = new float[numVertices * 3];
		auto texcoords = new float[numTexCoords * 2];
		std::vector<uint16_t> indices(numIndices);

		/*
		 * Access the primitive attributes to get accessor indices
		 */
		for (int32_t j = 0; j < mesh->primitives_count; j++)
		{
			const cgltf_primitive* primitive = &mesh->primitives[j];
			if (primitive->type != cgltf_primitive_type_triangles)
				continue;

			const cgltf_accessor* accessor = nullptr;
			for (int32_t attr = 0; attr < primitive->attributes_count; attr++)
			{
				const cgltf_attribute* attribute = &primitive->attributes[attr];
				accessor = attribute->data;
				if (attribute->type == cgltf_attribute_type_position)
				{
					assert(accessor->type == cgltf_type_vec3 &&
						   accessor->component_type == cgltf_component_type_r_32f);
					GLTF_MODEL_LOAD_ATTRIBUTE(accessor, 3, float, positions);
				}
				else if (attribute->type == cgltf_attribute_type_normal)
				{
				}
				else if (attribute->type == cgltf_attribute_type_texcoord)
				{

					assert(accessor->type == cgltf_type_vec2 &&
						   accessor->component_type == cgltf_component_type_r_32f);
					GLTF_MODEL_LOAD_ATTRIBUTE(accessor, 2, float, texcoords);
				}
			}

			if (primitive->indices != nullptr)
			{
				accessor = primitive->indices;
				assert(accessor->type == cgltf_type_scalar &&
					   accessor->component_type == cgltf_component_type_r_16u);
				GLTF_MODEL_LOAD_ATTRIBUTE(accessor, 1, uint16_t, indices);
			}
		}

		std::vector<Vertex> vertices(numVertices);
		int32_t iVertex = 0;
		int32_t iTexCoord = 0;
		for (uint32_t j = 0; j < numVertices; j++)
		{
			const int32_t v0 = iVertex;
			const int32_t v1 = iVertex + 1;
			const int32_t v2 = iVertex + 2;

			const int32_t uv0 = iTexCoord;
			const int32_t uv1 = iTexCoord + 1;

			vertices[j].Position = Vector4(positions[v0],
				positions[v1], positions[v2], 1.0f);

			vertices[j].TexCoord = Vector2(texcoords[uv0],
				texcoords[uv1]);

			iVertex += 3;
			iTexCoord += 2;
		}
		delete[] positions;
		delete[] texcoords;

		// TODO: Create Vertex and Index MTLBuffer

		VertexBuffer = NS::TransferPtr(
			device->newBuffer(vertices.data(),
				sizeof(Vertex) * vertices.size(),
				MTL::ResourceCPUCacheModeDefaultCache));
		VertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

		IndexBuffer = NS::TransferPtr(device->newBuffer(indices.data(),
			sizeof(uint16_t) *
			indices.size(),
			MTL::ResourceOptionCPUCacheModeDefault));
		IndexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));


//		const VkDeviceSize vbuffer_size = sizeof(struct Vertex) * vertex_count;
//		GKVulkanBuffer* staging_vertex_buffer =
//			gk_vulkan_buffer_new(priv->context, vbuffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//				VMA_MEMORY_USAGE_CPU_ONLY);
//
//		void* vertex_data;
//		gk_vulkan_buffer_map(staging_vertex_buffer, &vertex_data);
//		memcpy(vertex_data, vertices, vbuffer_size);
//		gk_vulkan_buffer_unmap(staging_vertex_buffer);
//
//		priv->meshes[i].vertex_buffer = gk_vulkan_buffer_new(priv->context, vbuffer_size,
//			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
//			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//			VMA_MEMORY_USAGE_GPU_ONLY);
//		gk_vulkan_buffer_copy(priv->meshes[i].vertex_buffer, staging_vertex_buffer, command_pool);
//		g_clear_object(&staging_vertex_buffer);
//
//		const VkDeviceSize index_buffer_size = sizeof(uint16_t) * priv->meshes[i].index_count;
//		GKVulkanBuffer* staging_index_buffer =
//			gk_vulkan_buffer_new(priv->context, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//				VMA_MEMORY_USAGE_CPU_ONLY);
//
//		void* index_data;
//		gk_vulkan_buffer_map(staging_index_buffer, &index_data);
//		memcpy(index_data, indices, index_buffer_size);
//		gk_vulkan_buffer_unmap(staging_index_buffer);
//
//		priv->meshes[i].index_buffer = gk_vulkan_buffer_new(priv->context, index_buffer_size,
//			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
//			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
//			VMA_MEMORY_USAGE_GPU_ONLY);
//		gk_vulkan_buffer_copy(priv->meshes[i].index_buffer, staging_index_buffer, command_pool);
//		g_clear_object(&staging_index_buffer);
	}

	/*
	 * Load texture images, this assumes the equivalent KTX image is available
	 * TODO: Support multiple texture images
	 */

//	auto numTextures = modelData->textures_count;
//	for (cgltf_size i = 0; i < numTextures; i++)
//	{
//		auto texture = modelData->textures[i];
//
//		printf("Test\n");
//	}

	auto numImages = modelData->images_count;
	for (cgltf_size i = 0; i < numImages; i++)
	{
		auto image = modelData->images[i];

		printf("Test\n");
	}

	auto* imageTypes = (NS::Array*)CGImageSourceCopyTypeIdentifiers();
	const auto numImageTypes = imageTypes->count();
	for (size_t i = 0; i < numImageTypes; i++)
	{
		printf("%s\n", imageTypes->object(i)->description()->utf8String());
	}
	//priv->texture = gk_vulkan_texture_new(priv->context, "Helmet");
}


void Model::Render(MTL::RenderCommandEncoder* commandEncoder)
{
	commandEncoder->setVertexBuffer(VertexBuffer.get(), 0, 0);
	commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
		IndexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16,
		IndexBuffer.get(), 0, 3);
}